package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

// UpdaterConfig holds persistent user preferences stored in bin/<platform>/updater.cfg
type UpdaterConfig struct {
	DownloadSymbols bool
}

func configPath(binDir string) string {
	return filepath.Join(binDir, "updater.cfg")
}

func loadConfig(binDir string) UpdaterConfig {
	cfg := UpdaterConfig{
		DownloadSymbols: false, // default off
	}

	b, err := os.ReadFile(configPath(binDir))
	if err != nil {
		return cfg
	}

	for _, line := range strings.Split(string(b), "\n") {
		line = strings.TrimSpace(line)
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		parts := strings.SplitN(line, "=", 2)
		if len(parts) != 2 {
			continue
		}
		key := strings.TrimSpace(parts[0])
		val := strings.TrimSpace(parts[1])

		switch key {
		case "symbols":
			cfg.DownloadSymbols, _ = strconv.ParseBool(val)
		}
	}
	return cfg
}

func saveConfig(binDir string, cfg UpdaterConfig) error {
	content := fmt.Sprintf(
		"# TF2 Vintage updater configuration\n"+
			"# Edit this file to change updater behaviour.\n"+
			"# Re-run tf2vintage-updater.exe --enable-symbols or --disable-symbols\n"+
			"# to toggle symbol downloads without editing manually.\n"+
			"\n"+
			"# Download debug symbols on each update (advanced users only).\n"+
			"# Symbols are used to get source-level stack traces from crash reports.\n"+
			"# Adds ~50-200 MB per update depending on build. Default: false\n"+
			"symbols=%v\n",
		cfg.DownloadSymbols,
	)
	return os.WriteFile(configPath(binDir), []byte(content), 0644)
}
