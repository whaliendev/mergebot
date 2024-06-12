package main

import (
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"time"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"gopkg.in/natefinch/lumberjack.v2"
)

const (
	MERGEBOT_CACHE_DIR = "/tmp/.mergebot"

	// The following constants are used for mergebot health check
	MERGEBOT_HOST            = "127.0.0.1"
	MERGEBOT_LISTEN_PORT     = 18080
	MERGEBOT_HEALTH_ENDPOINT = "/api/sa/health"
	INITIAL_DELAY_SECONDS    = 1 * time.Second

	PERIOD_SECONDS     = 1 * time.Second
	FAILURES_THRESHOLD = 3
	SUCCESS_THRESHOLD  = 1
	TIMEOUT_SECONDS    = 1 * time.Second

	CHECK_INTERVAL = 5 * time.Second
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

// CheckMergebotHealth checks the health of the mergebot service by sending a GET request to the health endpoint.
func CheckMergebotHealth(host string, port int) bool {
	healthEndpoint := fmt.Sprintf("http://%s:%d%s", host, port, MERGEBOT_HEALTH_ENDPOINT)
	ticker := time.NewTicker(PERIOD_SECONDS)
	defer ticker.Stop()

	failures := 0

	for range ticker.C {
		client := &http.Client{
			Timeout: TIMEOUT_SECONDS,
		}
		resp, err := client.Get(healthEndpoint)
	
		if err != nil {
			zap.S().Error("health check failed with error: ", err)
			failures++
			if failures >= FAILURES_THRESHOLD {
				return false
			}
			continue
		}
	
		defer resp.Body.Close()
	
		if resp.StatusCode != http.StatusOK {
			zap.S().Error("health check failed with status code: ", resp.StatusCode)
			failures++
			if failures >= FAILURES_THRESHOLD {
				return false
			}
			continue
		}
	
		body, err := io.ReadAll(resp.Body)
		if err != nil {
			zap.S().Error("failed to read response body: ", err)
			failures++
			if failures >= FAILURES_THRESHOLD {
				return false
			}
			continue
		}
	
		if string(body) != "OK" {
			zap.S().Error("health check failed with response body: ", string(body))
			failures++
			if failures >= FAILURES_THRESHOLD {
				return false
			}
		} else {
			// Successfully received "OK" response.
			return true
		}
	}
	return false
}

// At this point, we cannot easily implement the following functions.
// As replayPendingRequests need to collect all the params of these requests,
// while mergebot doesn't record these params.
// func cleanAndCollectPendingRequest() {

// }

// func replayPendingRequests() {

// }

type dirInfo struct {
	path       string
	accessTime time.Time
}

func markPendingRequestsAsFinished() {
	zap.S().Debug("begin to mark pending requests as finished")
	// Get all project directories
	projectDirs, err := os.ReadDir(MERGEBOT_CACHE_DIR)
	if err != nil {
		zap.S().Error("Failed to read base directory:", zap.Error(err))
	}

	var dirs []dirInfo
	for _, dir := range projectDirs {
		if dir.IsDir() {
			dirPath := filepath.Join(MERGEBOT_CACHE_DIR, dir.Name())
			stat, err := os.Stat(dirPath)
			if err != nil {
				zap.S().Error("Failed to get directory stats:", zap.String("directory", dirPath), zap.Error(err))
				continue
			}
			dirs = append(dirs, dirInfo{path: dirPath, accessTime: stat.ModTime()})
		}
	}

	// Sort directories by last access time in descending order
	sort.Slice(dirs, func(i, j int) bool {
		return dirs[i].accessTime.After(dirs[j].accessTime)
	})

	// Iterate through sorted directories
	for _, dir := range dirs {
		processProjectDir(dir.path)
	}
}

func processProjectDir(dirPath string) {
	// Iterate through subdirectories (merge scenarios)
	mergeScenarios, err := os.ReadDir(dirPath)
	if err != nil {
		zap.S().Error("Failed to read project directory:", zap.String("directory", dirPath), zap.Error(err))
	}

	for _, scenario := range mergeScenarios {
		if scenario.IsDir() {
			scenarioPath := filepath.Join(dirPath, scenario.Name())
			runningFilePath := filepath.Join(scenarioPath, "running")
			if _, err := os.Stat(runningFilePath); err == nil {
				// RUNNING file exists, remove it
				if err := os.Remove(runningFilePath); err != nil {
					zap.S().Error("Failed to remove RUNNING file:", zap.String("file", runningFilePath), zap.Error(err))
				} else {
					zap.S().Debug("Removed RUNNING file successfully", zap.String("file", runningFilePath))
				}
			}
		}
	}
}

func killByName(name string) {
	cmd := exec.Command("pkill", name)
	err := cmd.Run()
	if err != nil {
		if err == exec.ErrNotFound {
			fmt.Printf("process '%s' not found\n", name)
			zap.S().Fatalf("process '%s' not found", name)
		}
		// swallow the error intentionally, as the process may not exist
		zap.S().Info("failed to kill process", zap.Error(err))
	}
	zap.S().Info("killed process", zap.String("name", name))
}

func KillTCPListenersOnPort(port int) error {
	zap.S().Debugf("killing TCP listeners on port %d", port)
	lsofCmd := exec.Command("lsof", "-i", fmt.Sprintf("tcp:%d", port), "-t")
	output, err := lsofCmd.Output()
	if err != nil {
		zap.S().Errorf("failed to list TCP listeners on port %d: %v", port, err)
		return fmt.Errorf("failed to list TCP listeners on port %d: %v", port, err)
	}
	pidStrings := strings.Fields(string(output))
	if len(pidStrings) == 0 {
		zap.S().Infof("no TCP listeners found on port %d", port)
		return nil
	}
	zap.S().Infof("found TCP listeners on port %d: %v", port, pidStrings)

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

	err := killCmd.Run()
	if err != nil {
		if err == exec.ErrNotFound {
			fmt.Printf("process with PID %d not found\n", pid)
			zap.S().Fatalf("process with PID %d not found", pid)
		}
		// swallow the error intentionally, as the process may not exist
		zap.S().Infof("failed to kill process with PID %d: %v", pid, err)
	}
	zap.S().Infof("killed process with PID %d", pid)
}

func startMergebotService() error {
	wd, err := os.Getwd()
	if err != nil {
		zap.S().Error("failed to get current working directory", zap.Error(err))
		return err
	}

	dylibPath := filepath.Join(wd, "dylib")
	mergebotPath := filepath.Join(wd, "mergebot")
	mergebotCmd := fmt.Sprintf("LD_LIBRARY_PATH=%s:%s %s", wd, dylibPath, mergebotPath)
	zap.S().Debugln("mergebotCmd: ", mergebotCmd)
	cmd := exec.Command("sh", "-c", mergebotCmd)
	err = cmd.Start()
	if err != nil {
		zap.S().Error("failed to start mergebot", zap.Error(err))
		return err
	}

	return nil
}

func killMergebotService() {
	pgrepCmd := exec.Command("pgrep", "-x", "mergebot")
	output, err := pgrepCmd.Output()
	if err != nil {
		zap.S().Error("failed to find mergebot process", zap.Error(err))
		return
	}
	pidStrings := strings.Fields(string(output))
	for _, pidStr := range pidStrings {
		pid, err := strconv.Atoi(pidStr)
		if err != nil {
			zap.S().Errorf("Invalid PID '%s' found: %v\n", pidStr, err)
			continue
		}

		killChildProcesses(pid)
		killByPID(pid)
	}
}

func killChildProcesses(pid int) {
	// Use pkill to kill the process tree starting from a specific PID
	pkillCmd := exec.Command("pkill", "-P", strconv.Itoa(pid))
	err := pkillCmd.Run()
	if err != nil {
		if err == exec.ErrNotFound {
			fmt.Printf("process with PID %d not found\n", pid)
			zap.S().Fatalf("process with PID %d not found", pid)

		}
		// swallow the error intentionally, as the process may not exist
		zap.S().Infof("failed to kill child processes of PID %d: %v", pid, err)
	}
	zap.S().Infof("killed child processes of PID %d", pid)
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
		ticker.Reset(CHECK_INTERVAL)
		zap.S().Error("mergebot is unhealthy, taking corrective actions")

		// Start a goroutine to clean cache dir and collect pending requests
		go markPendingRequestsAsFinished()

		// 2.2.1 Kill mergebot and clangd
		killMergebotService()

		// 2.2.2 Kill all TCP services listening on MERGEBOT_LISTEN_PORT
		if err := KillTCPListenersOnPort(MERGEBOT_LISTEN_PORT); err != nil {
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
		time.Sleep(INITIAL_DELAY_SECONDS)
		if !CheckMergebotHealth(MERGEBOT_HOST, MERGEBOT_LISTEN_PORT) {
			zap.S().Error("mergebot is still unhealthy after restart")
			continue
		}

		// 2.2.4 Replay pending requests
		// replayPendingRequests()
	}
}
