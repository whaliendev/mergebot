package main

import (
	"bytes"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"os/signal"
	"path/filepath"
	"runtime"
	"sort"
	"strconv"
	"strings"
	"syscall"
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

	// k8s style readiness probe parameters
	PERIOD_SECONDS     = 1 * time.Second
	FAILURES_THRESHOLD = 3
	SUCCESS_THRESHOLD  = 1
	TIMEOUT_SECONDS    = 1 * time.Second

	// CHECK_INTERVAL is used to check the health of mergebot periodically
	CHECK_INTERVAL = 5 * time.Second

	// LOG_LENTH_THRESH is used to limit the length of mergebot logs written to husky log file
	LOG_LENTH_THRESH = 3000
)

func main() {
	// 0. fundamental check
	err := preliminaryCheck()
	if err != nil {
		panic(err)
	}

	// 1. prepare logger
	configureZapLogger()
	defer zap.L().Sync()
	zap.L().Info("Start husky")

	// Setting up signal handling
	signals := make(chan os.Signal, 1)
	// Notify this channel on SIGINT or SIGTERM
	signal.Notify(signals, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		for sig := range signals {
			if sig == syscall.SIGINT || sig == syscall.SIGTERM {
				// Perform your cleanup here, e.g.,
				// flushing logs or stopping any background tasks...
				zap.S().Info("Received signal", zap.String("signal", sig.String()), zap.String("action", "cleaning up and exitiing..."))
				fmt.Println("Received signal", sig.String(), "cleaning up and exiting...")
				zap.L().Sync()
				// After cleanup, exit
				os.Exit(0)
			}
		}
	}()

	// 2. loop to check mergebot health
	//   2.1 if mergebot is healthy, do nothing
	//   2.2 if mergebot is down or take too long to response:
	//      - start a goroutine to mark pending requests as finished
	//      2.2.1 kill mergebot and clangd
	//      2.2.2 kill all the tcp service listen on MERGEBOT_LISTEN_PORT.
	//      2.2.3 restart mergebot and clangd, and wait for mergebot to be healthy
	time.Sleep(3 * time.Second)
	ticker := time.NewTicker(CHECK_INTERVAL)
	defer ticker.Stop()
	// every new run, clean all previous pending requests
	go markPendingRequestsAsFinished()
	for range ticker.C {
		if checkMergebotHealth(MERGEBOT_HOST, MERGEBOT_LISTEN_PORT) {
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

		// Wait for mergebot to be healthy
		time.Sleep(INITIAL_DELAY_SECONDS)
		if !checkMergebotHealth(MERGEBOT_HOST, MERGEBOT_LISTEN_PORT) {
			zap.S().Error("mergebot is still unhealthy after restart")
		}

		// 2.2.4 Replay pending requests
		// replayPendingRequests()
	}
}

// At this point, we cannot easily implement the following functions.
// As replayPendingRequests need to collect all the params of these requests,
// while mergebot doesn't record these params.
// func cleanAndCollectPendingRequest() {

// }

// func replayPendingRequests() {

// }

func preliminaryCheck() error {
	//  Check 1: Verify that the OS is Unix-like
	if runtime.GOOS != "linux" {
		return fmt.Errorf("unsupported platform: %s", runtime.GOOS)
	}

	//  Check 2: Verify that mergebot binary exists and is executable
	mergebotPath := "./mergebot"
	info, err := os.Stat(mergebotPath)
	if os.IsNotExist(err) {
		return fmt.Errorf("you should put husky and mergebot in the same dir")
	}
	if info.Mode().Perm()&0111 == 0 {
		return fmt.Errorf("unexpected permission of mergebot binary")
	}

	// Check 3: Check the existence of kill, pkill, lsof, pgrep
	killExists, _, _ := CheckBinaryExists("kill")
	pkillExists, _, _ := CheckBinaryExists("pkill")
	lsofExists, _, _ := CheckBinaryExists("lsof")
	pgrepExists, _, _ := CheckBinaryExists("pgrep")
	if !killExists || !pkillExists || !lsofExists || !pgrepExists {
		return fmt.Errorf("binary executable needed by husky not installed, on ubuntu, you can install then by typing \n\n\nsudo apt-get install kill pkill lsof pgrep\n\n\n in the terminal")
	}
	return nil
}

func configureZapLogger() {
	logFilePath := filepath.Join(".", "husky.log")

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

func checkMergebotHealth(host string, port int) bool {
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
				// running file exists, remove it
				if err := os.Remove(runningFilePath); err != nil {
					zap.S().Error("Failed to remove running file:", zap.String("file", runningFilePath), zap.Error(err))
				} else {
					zap.S().Debug("Removed RUNNING file successfully", zap.String("file", runningFilePath))
				}
			}
		}
	}
}

func startMergebotService() error {
	// Get current working directory
	wd, err := os.Getwd()
	if err != nil {
		zap.S().Error("Failed to get current working directory", zap.Error(err))
		return err
	}

	// Set the dynamic library path and mergebot executable path
	dylibPath := filepath.Join(wd, "dylib")
	mergebotPath := filepath.Join(wd, "mergebot")

	// Prepare the command
	cmd := exec.Command(mergebotPath)
	cmd.Env = append(os.Environ(), fmt.Sprintf("LD_LIBRARY_PATH=%s:%s", wd, dylibPath))

	// Create buffers to capture standard output and standard error
	var stdoutBuf, stderrBuf bytes.Buffer
	cmd.Stdout = &stdoutBuf
	cmd.Stderr = &stderrBuf

	// Start the command
	err = cmd.Start()
	if err != nil {
		zap.S().Error("Failed to start mergebot", zap.Error(err))
		return err
	}

	// Wait for the process to exit and log its status
	go func() {
		err := cmd.Wait()
		stdout := stdoutBuf.String()
		stderr := stderrBuf.String()
		if len(stdout) >= LOG_LENTH_THRESH {
			stdout = stdout[:LOG_LENTH_THRESH]
		}
		if len(stderr) >= LOG_LENTH_THRESH {
			stderr = stderr[:LOG_LENTH_THRESH]
		}

		if err != nil {
			zap.S().Error("Mergebot exited with error", zap.Error(err), zap.String("stdout", stdout), zap.String("stderr", stderr))
		} else {
			zap.S().Info("Mergebot exited successfully", zap.String("stdout", stdout), zap.String("stderr", stderr))
		}
	}()

	zap.S().Debug("Mergebot started successfully")
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

		KillChildProcesses(pid)
		KillByPID(pid)
	}
}
