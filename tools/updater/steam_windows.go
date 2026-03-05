//go:build windows

package main

import (
	"strings"
	"fmt"
	"os/exec"
	"path/filepath"
	"time"

	"golang.org/x/sys/windows/registry"
)

func findSteamPathWindows() (string, error) {
	k, err := registry.OpenKey(registry.CURRENT_USER,
		`Software\Valve\Steam`, registry.QUERY_VALUE)
	if err != nil {
		return "", fmt.Errorf("Steam not found in registry — is Steam installed?")
	}
	defer k.Close()
	path, _, err := k.GetStringValue("SteamPath")
	if err != nil {
		return "", fmt.Errorf("could not read SteamPath from registry")
	}
	return filepath.FromSlash(path), nil
}

func findSourcemodsPathWindows() (string, error) {
	k, err := registry.OpenKey(registry.CURRENT_USER,
		`Software\Valve\Steam`, registry.QUERY_VALUE)
	if err != nil {
		return "", fmt.Errorf("Steam not found in registry")
	}
	defer k.Close()
	path, _, err := k.GetStringValue("SourceModInstallPath")
	if err != nil {
		// Fall back to deriving from SteamPath
		steamPath, serr := findSteamPathWindows()
		if serr != nil {
			return "", fmt.Errorf("could not find Sourcemods path")
		}
		return filepath.Join(steamPath, "steamapps", "sourcemods"), nil
	}
	return filepath.FromSlash(path), nil
}

func closeSteamWindows() error {
	cmd := exec.Command("taskkill", "/IM", "steam.exe", "/F")
	cmd.Run()
	time.Sleep(2 * time.Second)
	return nil
}

func relaunchSteamWindows(steamPath string) error {
	steamExe := filepath.Join(steamPath, "steam.exe")
	cmd := exec.Command(steamExe)
	return cmd.Start()
}

func isSteamRunningWindows() bool {
	out, err := exec.Command("tasklist", "/FI", "IMAGENAME eq steam.exe", "/NH").Output()
	if err != nil {
		return false
	}
	return strings.Contains(strings.ToLower(string(out)), "steam.exe")
}
