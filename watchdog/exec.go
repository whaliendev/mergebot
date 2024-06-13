package main

import (
	"fmt"
	"os/exec"
	"strconv"
	"strings"

	"go.uber.org/zap"
)

// CheckBinaryExists checks if a binary executable exists on the system.
// It returns true and the path to the executable if it exists, otherwise it returns false and an error.
func CheckBinaryExists(name string) (bool, string, error) {
	path, err := exec.LookPath(name)
	if err != nil {
		// The executable was not found
		return false, "", err
	}
	// The executable was found
	return true, path, nil
}

// KillChildProcesses kills all child processes of a given PID using the pkill command.
func KillChildProcesses(pid int) {
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
	} else {
		zap.S().Infof("killed child processes of PID %d", pid)
	}
}

// KillTCPListenersOnPort kills all TCP listeners on a specific port using lsof and kill commands.
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

		KillByPID(pid)
	}
	return nil
}

// KillByPID kills a process by its PID using the kill command.
func KillByPID(pid int) {
	killCmd := exec.Command("kill", "-9", strconv.Itoa(pid))

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
