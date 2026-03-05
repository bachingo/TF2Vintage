//go:build !windows

package main

import (
	"os"
	"syscall"
)

func isProcessRunning(pid int) bool {
	proc, err := os.FindProcess(pid)
	if err != nil {
		return false
	}
	// Signal 0 checks existence without sending a signal
	err = proc.Signal(syscall.Signal(0))
	return err == nil
}
