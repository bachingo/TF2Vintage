package main

import (
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"
)

// UpdaterConfig holds persistent user preferences stored in tf2vintage/updater.cfg.
// The file lives in the mod root so both platforms share a single path and it
// is never overwritten by a bin-only update.
type UpdaterConfig struct {
	DownloadSymbols bool
	CheckNightly    bool
}

func configPath(modDir string) string {
	return filepath.Join(modDir, "updater.cfg")
}

func loadConfig(modDir string) UpdaterConfig {
	cfg := UpdaterConfig{
		DownloadSymbols: false, // default off
		CheckNightly:    false, // default off
	}

	b, err := os.ReadFile(configPath(modDir))
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
		case "nightly":
			cfg.CheckNightly, _ = strconv.ParseBool(val)
		}
	}
	return cfg
}

func saveConfig(modDir string, cfg UpdaterConfig) error {
	content := fmt.Sprintf(
		"# TF2 Vintage updater configuration\n"+
			"# Edit this file to change updater behaviour.\n"+
			"\n"+
			"# Download debug symbols on each update (advanced users only).\n"+
			"# Symbols are used to get source-level stack traces from crash reports.\n"+
			"# Adds ~50-200 MB per update depending on build. Default: false\n"+
			"# Toggle: tf2vintage-updater --enable-symbols / --disable-symbols\n"+
			"symbols=%v\n"+
			"\n"+
			"# Check for nightly pre-releases in addition to stable releases.\n"+
			"# Nightlies are built every Tuesday and contain the latest binaries.\n"+
			"# Game assets (maps, materials, etc.) are only updated in stable releases.\n"+
			"# Toggle: tf2vintage-updater --enable-nightly / --disable-nightly\n"+
			"nightly=%v\n",
		cfg.DownloadSymbols,
		cfg.CheckNightly,
	)
	return os.WriteFile(configPath(modDir), []byte(content), 0644)
}
