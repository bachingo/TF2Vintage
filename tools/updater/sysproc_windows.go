//go:build windows

package main

import (
	"os/exec"
	"syscall"
)

func detachedProcess() *syscall.SysProcAttr {
	return &syscall.SysProcAttr{
		CreationFlags: syscall.CREATE_NEW_PROCESS_GROUP | 0x00000008, // DETACHED_PROCESS
	}
}

// newCmd wraps exec.Command for Steam launch on Windows.
func newCmd(name string, args ...string) *exec.Cmd {
	return exec.Command(name, args...)
}
