//go:build linux || darwin

package main

import "os/exec"

// newCmd returns an exec.Cmd for the given program and arguments.
// On Linux there is no console window to hide, so this is a plain exec.Command.
func newCmd(name string, args ...string) *exec.Cmd {
	return exec.Command(name, args...)
}

// termPauseIfStandalone blocks on stdin for standalone mode.
// On Linux, termPause() in helpers.go already handles this.
