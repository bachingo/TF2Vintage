//go:build linux || darwin

package main

import "os/exec"

// newCmd returns an exec.Cmd for the given program and arguments.
// On Linux there is no console window to hide, so this is a plain exec.Command.
func newCmd(name string, args ...string) *exec.Cmd {
	return exec.Command(name, args...)
}

// swapBinDir atomically replaces liveDir with stagingDir.
// On Linux/macOS the running exe is not locked at the filesystem level, so a
// plain atomicSwapDir (renaming two siblings) is sufficient. Both liveDir and
// stagingDir must be siblings (same parent directory) for the rename to succeed.
func swapBinDir(liveDir, stagingDir string) error {
	return atomicSwapDir(liveDir, stagingDir)
}

