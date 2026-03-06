//go:build windows

package main

import (
	"os/exec"
	"syscall"
)

// newCmd returns an exec.Cmd with HideWindow set so spawned processes don't
// flash a console window on Windows.
func newCmd(name string, args ...string) *exec.Cmd {
	cmd := exec.Command(name, args...)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	return cmd
}
