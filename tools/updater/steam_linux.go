//go:build linux

package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"time"
)

func isWindows() bool { return false }

// ── Steam path detection ──────────────────────────────────────────────────────

func findSteamPath() (string, error) {
	candidates := []string{
		filepath.Join(os.Getenv("HOME"), ".steam", "steam"),
		filepath.Join(os.Getenv("HOME"), ".local", "share", "Steam"),
	}
	for _, p := range candidates {
		if _, err := os.Stat(p); err == nil {
			return p, nil
		}
	}
	return "", fmt.Errorf("Steam installation not found")
}

func findSourcemodsPath() (string, error) {
	steam, err := findSteamPath()
	if err != nil {
		return "", err
	}
	return filepath.Join(steam, "steamapps", "sourcemods"), nil
}

// ── Steam process management ──────────────────────────────────────────────────

func closeSteam() error {
	cmd := exec.Command("pkill", "-x", "steam")
	cmd.Run() // ignore error — steam may not be running
	time.Sleep(2 * time.Second)
	return nil
}

func relaunchSteam(steamPath string) error {
	steamBin := filepath.Join(steamPath, "steam.sh")
	cmd := exec.Command(steamBin)
	cmd.Start()
	return nil
}

func isSteamRunning() bool {
	out, err := exec.Command("pgrep", "-x", "steam").Output()
	return err == nil && len(out) > 0
}
