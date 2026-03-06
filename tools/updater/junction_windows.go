//go:build windows

package main

import (
	"fmt"
	"os/exec"
	"syscall"
)

// createLink creates a directory junction from linkPath to targetPath on Windows.
func createLink(linkPath, targetPath string) error {
	return createJunctionWindows(linkPath, targetPath)
}

// createJunctionWindows creates a Windows directory junction using mklink /J.
func createJunctionWindows(linkPath, targetPath string) error {
	cmd := exec.Command("cmd", "/c", "mklink", "/J", linkPath, targetPath)
	cmd.SysProcAttr = &syscall.SysProcAttr{HideWindow: true}
	out, err := cmd.CombinedOutput()
	if err != nil {
		return fmt.Errorf("mklink /J failed: %v\n%s", err, out)
	}
	return nil
}
