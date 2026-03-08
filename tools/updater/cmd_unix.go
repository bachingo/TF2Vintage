//go:build linux || darwin

package main

import "os/exec"

// newCmd returns an exec.Cmd for the given program and arguments.
// On Linux there is no console window to hide, so this is a plain exec.Command.
func newCmd(name string, args ...string) *exec.Cmd {
	return exec.Command(name, args...)
}

// swapBinDir replaces liveBinDir with stagingBinDir.
// On Linux/macOS the running exe is unlocked at the filesystem level — the
// directory can be renamed freely even while the process is running — so a
// plain atomic swap is sufficient.
func swapBinDir(liveBinDir, stagingBinDir string) error {
	return atomicSwapDir(liveBinDir, stagingBinDir)
}

