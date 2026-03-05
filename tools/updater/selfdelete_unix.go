//go:build !windows

package main

import "os"

// scheduleDelete on Linux/Mac — the file can be unlinked immediately even
// while running since the inode stays alive until the process exits.
func scheduleDelete(path string) {
	os.Remove(path)
}
