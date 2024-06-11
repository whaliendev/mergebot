package main

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"time"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"gopkg.in/natefinch/lumberjack.v2"
)

const (
	MERGEBOT_CACHE_DIR       = "/tmp/.mergebot"
	MERGEBOT_HOST            = "127.0.0.1"
	MERGEBOT_LISTEN_PORT     = 18080
	MERGEBOT_HEALTH_ENDPOINT = "/api/sa/health"
	CHECK_INTERVAL           = 5 * time.Second
	UNRESPONSIVE_THRESHOLD   = 3 * time.Second
)

func preliminaryCheck() error {
	//  Check 1: Verify that the OS is Unix-like
	if runtime.GOOS != "linux" {
		return fmt.Errorf("unsupported platform: %s", runtime.GOOS)
	}

	//  Check 2: Verify that mergebot binary exists and is executable
	mergebotPath := "./mergebot"
	info, err := os.Stat(mergebotPath)
	if os.IsNotExist(err) {
		return fmt.Errorf("you should put watchdog and mergebot in the same dir")
	}
	if info.Mode().Perm()&0111 == 0 {
		return fmt.Errorf("unexpected permission of mergebot binary")
	}
	return nil
}

func configureZapLogger() {
	logFilePath := filepath.Join(".", "watchdog.log")

	// set up a rolling log writer
	lumberjackLogger := &lumberjack.Logger{
		Filename:   logFilePath,
		MaxSize:    10, // megabytes
		MaxBackups: 7,  // keep 7 backups
		MaxAge:     7,  // days
		Compress:   false,
	}

	writeSyncer := zapcore.AddSync(lumberjackLogger)

	// define encoder configuration
	encoderConfig := zap.NewProductionEncoderConfig()
	encoderConfig.EncodeTime = zapcore.ISO8601TimeEncoder
	core := zapcore.NewCore(
		zapcore.NewJSONEncoder(encoderConfig),
		writeSyncer,
		zapcore.DebugLevel,
	)

	logger := zap.New(core)
	zap.ReplaceGlobals(logger)
}

func CheckMergebotHealth(host string, port int) (healthy bool) {
	healthEndpoint := fmt.Sprintf("http://%s:%d%s", host, port, MERGEBOT_HEALTH_ENDPOINT)

	client := &http.Client{
		Timeout: UNRESPONSIVE_THRESHOLD,
	}

	resp, err := client.Get(healthEndpoint)
	if err != nil {
		zap.S().Error("health check failed with error: ", err)
		return false
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		zap.S().Error("health check failed with status code: ", resp.StatusCode)
		return false
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		zap.S().Error("failed to read response body: ", err)
		return false
	}

	if string(body) != "OK" {
		zap.S().Error("health check failed with response body: ", string(body))
		return false
	}

	return true
}

// At this point, we cannot easily implement the following functions.
// As replayPendingRequests need to collect all the params of these requests,
// while mergebot doesn't record these params.
// func cleanAndCollectPendingRequest() {

// }

// func replayPendingRequests() {

// }

func markPendingRequestsAsFinished() {

}

func killProcess(name string) {
	cmd := exec.Command("pkill", name)
	cmd.Run()
}

func killProcessWithParent(name string, parentName string) {

}

func killTCPListenersOnPort(port int) error {
	lsofCmd := exec.Command("lsof", "-i", fmt.Sprintf("tcp:%d", port), "-t")
	output, err := lsofCmd.Output()
	if err != nil {
		return fmt.Errorf("failed to list TCP listeners on port %d: %v", port, err)
	}
	pidStrings := strings.Fields(string(output))
	if len(pidStrings) == 0 {
		zap.S().Infof("no TCP listeners found on port %d", port)
		return nil
	}

	for _, pidStr := range pidStrings {
		pid, err := strconv.Atoi(pidStr)
		if err != nil {
			zap.S().Errorf("Invalid PID '%s' found: %v\n", pidStr, err)
			continue
		}

		killByPID(pid)
	}
	return nil
}

func killByPID(pid int) {
	killCmd := exec.Command("kill", "-9", strconv.Itoa(pid))
	killCmd.Run()
}

func startMergebotService() error {
	return nil
}

func main() {
	// 0. fundamental check
	err := preliminaryCheck()
	if err != nil {
		panic(err)
	}

	// 1. prepare logger
	configureZapLogger()
	defer zap.L().Sync()
	zap.L().Info("Start watchdog")

	// 2. loop to check mergebot health
	//   2.1 if mergebot is healthy, do nothing
	//   2.2 if mergebot is down or take too long to response:
	//      - start a goroutine to mark pending requests as finished
	//      2.2.1 kill mergebot and clangd
	//      2.2.2 kill all the tcp service listen on MERGEBOT_LISTEN_PORT.
	//      2.2.3 restart mergebot and clangd, and wait for mergebot to be healthy
	ticker := time.NewTicker(CHECK_INTERVAL)
	defer ticker.Stop()
	for range ticker.C {
		if CheckMergebotHealth(MERGEBOT_HOST, MERGEBOT_LISTEN_PORT) {
			continue // mergebot is healthy, do nothing
		}

		// Mergebot is down or unresponsive
		zap.S().Error("mergebot is unhealthy, taking corrective actions")

		// Start a goroutine to clean cache dir and collect pending requests
		go markPendingRequestsAsFinished()

		// 2.2.1 Kill mergebot and clangd
		killProcess("mergebot")
		killProcessWithParent("clangd", "mergebot")

		// 2.2.2 Kill all TCP services listening on MERGEBOT_LISTEN_PORT
		if err := killTCPListenersOnPort(MERGEBOT_LISTEN_PORT); err != nil {
			zap.S().Error("failed to kill TCP listeners on port", zap.Error(err))
		}

		// 2.2.3 Restart mergebot and clangd
		if err := startMergebotService(); err != nil {
			zap.L().Error("failed to restart mergebot", zap.Error(err))
		}
		// if err := startProcess("clangd"); err != nil {
		// 	zap.L().Error("Failed to restart clangd", zap.Error(err))
		// }

		// Wait for mergebot to be healthy
		time.Sleep(1 * time.Second)
		if !CheckMergebotHealth(MERGEBOT_HOST, MERGEBOT_LISTEN_PORT) {
			zap.S().Error("mergebot is still unhealthy after restart")
			continue
		}

		// 2.2.4 Replay pending requests
		// replayPendingRequests()
	}
}
