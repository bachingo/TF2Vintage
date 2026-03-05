package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

// acquireLock creates a lockfile in the same directory as the executable.
// Returns a release function and an error if another instance holds the lock.
func acquireLock(dir string) (release func(), err error) {
	lockPath := filepath.Join(dir, ".update.lock")

	// Check for existing lock
	if b, err := os.ReadFile(lockPath); err == nil {
		pid, _ := strconv.Atoi(strings.TrimSpace(string(b)))
		if pid > 0 && isProcessRunning(pid) {
			return nil, fmt.Errorf(
				"another instance of the updater is already running (PID %d)\n"+
					"If this is wrong, delete: %s", pid, lockPath)
		}
		// Stale lock from a crashed process — remove it
		os.Remove(lockPath)
	}

	if err := os.WriteFile(lockPath, []byte(strconv.Itoa(os.Getpid())), 0644); err != nil {
		// Can't write lock — warn but don't block (read-only filesystem edge case)
		termWarn("Could not create lockfile: %v", err)
		return func() {}, nil
	}

	return func() { os.Remove(lockPath) }, nil
}
