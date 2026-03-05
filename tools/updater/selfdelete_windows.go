//go:build windows

package main

import (
	"os"
	"os/exec"
)

// scheduleDelete removes the installer exe after the process exits.
// Uses a detached cmd.exe to delete the file once this process has released it.
func scheduleDelete(path string) {
	// Ping loops until our PID is gone, then deletes the file
	cmd := exec.Command("cmd.exe", "/C",
		"ping -n 3 127.0.0.1 >nul & del /F /Q \""+path+"\"")
	cmd.SysProcAttr = detachedProcess()
	cmd.Start()
}
