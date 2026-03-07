package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
)

// InstallState tracks progress for the GUI to display.
type InstallState struct {
	Status   string
	Progress float64 // 0.0–1.0
	Done     bool
	Err      error
}

type installNeeds struct {
	freshInstall bool
	repairBase   bool
	repairBin    bool
	sourcemods   string
	installDir   string
	reason       string
}

func runInstallMode() {
	startInstallUI(doInstall)
}

// doInstall is called by the GUI on a background goroutine.
// askAltPath blocks until the GUI responds with a path (empty = use default).
func doInstall(report func(InstallState), askAltPath func() string, askSymbols func() bool) {
	report(InstallState{Status: "Locating Steam..."})

	steamPath, err := findSteamPath()
	if err != nil {
		openURL("https://store.steampowered.com/about/")
		report(InstallState{Err: fmt.Errorf(
			"Steam could not be found on this computer.\n\n" +
				"If Steam is installed in a non-standard location, place the tf2vintage-updater " +
				"into your existing tf2vintage/bin/" + binDirName() + "/ folder and run it from there instead.\n\n" +
				"Otherwise, install Steam from:\nhttps://store.steampowered.com/about/")})
		return
	}

	// ── Check / install Source SDK Base 2013 MP ───────────────────────────────
	report(InstallState{Status: "Checking Source SDK Base 2013 Multiplayer...", Progress: 0.05})
	if !isSDKInstalled(steamPath) {
		report(InstallState{
			Status: "Source SDK Base 2013 Multiplayer not found.\n" +
				"Opening Steam to install it — please wait for it to finish (3.29 GB)..."})
		promptInstallSDK()
		if !waitForSDKInstall(steamPath) {
			openURL("https://store.steampowered.com/app/243750/Source_SDK_Base_2013_Multiplayer/")
			report(InstallState{Err: fmt.Errorf(
				"Source SDK Base 2013 Multiplayer did not finish installing.\n\n" +
					"The Steam store page has been opened in your browser.\n" +
					"Once it is installed (3.29 GB), run this installer again.")})
			return
		}
	}

	// ── Check / install Team Fortress 2 (app 440) ─────────────────────────────
	// TF2V mounts TF2's content directory at runtime and overlays its own assets
	// on top. TF2 must be installed for the game to have its full content.
	report(InstallState{Status: "Checking Team Fortress 2...", Progress: 0.08})
	if !isTF2Installed(steamPath) {
		report(InstallState{
			Status: "Team Fortress 2 not found.\n" +
				"Opening Steam to install it — please wait for it to finish (~25 GB)..."})
		promptInstallTF2()
		if !waitForTF2Install(steamPath) {
			openURL("https://store.steampowered.com/app/440/Team_Fortress_2/")
			report(InstallState{Err: fmt.Errorf(
				"Team Fortress 2 did not finish installing.\n\n" +
					"The Steam store page has been opened in your browser.\n" +
					"Once it is installed (~25 GB), run this installer again.")})
			return
		}
	}

	// ── Find Sourcemods path ──────────────────────────────────────────────────
	report(InstallState{Status: "Checking existing installation...", Progress: 0.10})
	sourcemods, err := findSourcemodsPath()
	if err != nil {
		report(InstallState{Err: fmt.Errorf("Could not locate Sourcemods folder: %v", err)})
		return
	}

	// ── Ask user about alternate install location ─────────────────────────────
	// This blocks until the GUI's folder picker is resolved (or dismissed).
	altPath := askAltPath()

	installDir, err := resolveInstallDir(sourcemods, altPath)
	if err != nil {
		report(InstallState{Err: err})
		return
	}

	// ── Diagnose existing state ───────────────────────────────────────────────
	needs := diagnose(sourcemods, installDir)
	report(InstallState{Status: needs.reason, Progress: 0.12})

	// Healthy install — just run the updater logic
	if !needs.freshInstall && !needs.repairBase && !needs.repairBin {
		report(InstallState{Status: "Existing installation looks healthy — checking for updates...", Progress: 0.15})
		latest, err := fetchLatestRelease()
		if err != nil {
			report(InstallState{Err: fmt.Errorf("Could not reach GitHub: %v", err)})
			return
		}
		binDir := platformBinDir(installDir)
		existingCfg := loadConfig(binDir)
		// Re-ask symbol preference only if config doesn't exist yet
		if _, statErr := os.Stat(configPath(binDir)); os.IsNotExist(statErr) {
			existingCfg.DownloadSymbols = askSymbols()
			saveConfig(binDir, existingCfg)
		}
		// For an already-installed copy, use a staging dir beside the install
		// so updates can be swapped in atomically.
		stagingRoot := installDir + ".staging"
		stagingBinDir := filepath.Join(stagingRoot, "bin", binDirName())
		stagingModDir := filepath.Join(stagingRoot, "mod")
		os.RemoveAll(stagingRoot)
		os.MkdirAll(stagingBinDir, 0755)
		os.MkdirAll(stagingModDir, 0755)

		binUpdated := false
		baseUpdated := false

		switch err := updateBin(binDir, stagingBinDir, latest); {
		case err == nil:
			binUpdated = true
		case errors.Is(err, errUpToDate):
			// nothing staged — do not swap
		default:
			report(InstallState{Err: fmt.Errorf("Bin update failed: %v", err)})
			os.RemoveAll(stagingRoot)
			return
		}
		switch err := updateBase(installDir, stagingModDir, latest); {
		case err == nil:
			baseUpdated = true
		case errors.Is(err, errUpToDate):
			// nothing staged — do not swap
		default:
			report(InstallState{Err: fmt.Errorf("Base update failed: %v", err)})
			os.RemoveAll(stagingRoot)
			return
		}
		if existingCfg.DownloadSymbols {
			switch err := updateSymbols(binDir, stagingBinDir, latest); {
			case err == nil:
				binUpdated = true // symbols staged alongside bin — swap together
			case errors.Is(err, errUpToDate):
				// nothing staged — do not swap
			default:
				termWarn("Symbol update failed: %v", err)
			}
		}
		if binUpdated {
			if err := atomicSwapDir(binDir, stagingBinDir); err != nil {
				report(InstallState{Err: fmt.Errorf("Bin swap failed: %v", err)})
				os.RemoveAll(stagingRoot)
				return
			}
		}
		if baseUpdated {
			if err := atomicSwapDir(installDir, stagingModDir); err != nil {
				report(InstallState{Err: fmt.Errorf("Base swap failed: %v", err)})
				os.RemoveAll(stagingRoot)
				return
			}
		}
		os.RemoveAll(stagingRoot)
		finalize(report, filepath.Join(binDir, updaterName()))
		return
	}

	if err := os.MkdirAll(installDir, 0755); err != nil {
		report(InstallState{Err: fmt.Errorf("Could not create install directory: %v", err)})
		return
	}

	// ── Fetch release info ────────────────────────────────────────────────────
	report(InstallState{Status: "Fetching latest release information...", Progress: 0.15})
	latest, err := fetchLatestRelease()
	if err != nil {
		report(InstallState{Err: fmt.Errorf(
			"Could not reach GitHub: %v\n\nCheck your internet connection and try again.", err)})
		return
	}

	// ── Download + extract base ───────────────────────────────────────────────
	if needs.repairBase {
		if err := checkDiskSpace(installDir, minFreeBytesForBase); err != nil {
			report(InstallState{Err: err})
			return
		}

		baseURL := assetURL(latest, "tf2vintage-base.zip")
		if baseURL == "" {
			report(InstallState{Err: fmt.Errorf("Base package not found in latest release — please try again later.")})
			return
		}

		report(InstallState{Status: "Downloading base assets — 350 MB compressed, may take a while...", Progress: 0.20})
		baseTmp, err := downloadWithProgressCallback(baseURL, func(downloaded, total int64) {
			if total > 0 {
				pct := float64(downloaded) / float64(total)
				report(InstallState{
					Status:   fmt.Sprintf("Downloading base assets... %.0f%%", pct*100),
					Progress: 0.20 + pct*0.45,
				})
			}
		})
		if err != nil {
			report(InstallState{Err: fmt.Errorf(
				"Download interrupted: %v\n\nRun the installer again to resume.", err)})
			return
		}
		defer os.Remove(baseTmp)

		// Verify base integrity before extracting
		if expectedHash := fetchAssetChecksum(latest, "tf2vintage-base.zip"); expectedHash != "" {
			if err := verifyDownloadChecksum(baseTmp, expectedHash); err != nil {
				os.Remove(baseTmp)
				report(InstallState{Err: fmt.Errorf("Base download integrity check failed: %v", err)})
				return
			}
		}

		report(InstallState{Status: "Extracting base assets...", Progress: 0.65})
		if err := extractZip(baseTmp, installDir); err != nil {
			report(InstallState{Err: fmt.Errorf("Failed to extract base assets: %v", err)})
			return
		}
		if manifest, err := fetchReleaseManifest(latest); err == nil {
			saveManifest(filepath.Join(installDir, "base-manifest.json"), manifest)
		}
	}

	// ── Download + extract bin ────────────────────────────────────────────────
	if needs.repairBin {
		if err := checkDiskSpace(installDir, minFreeBytesForBins); err != nil {
			report(InstallState{Err: err})
			return
		}

		binURL := assetURL(latest, platformBinAsset())
		if binURL == "" {
			report(InstallState{Err: fmt.Errorf("Bin package not found in latest release — please try again later.")})
			return
		}

		report(InstallState{Status: "Downloading game binaries...", Progress: 0.70})
		binTmp, err := downloadWithProgressCallback(binURL, func(downloaded, total int64) {
			if total > 0 {
				pct := float64(downloaded) / float64(total)
				report(InstallState{
					Status:   fmt.Sprintf("Downloading game binaries... %.0f%%", pct*100),
					Progress: 0.70 + pct*0.15,
				})
			}
		})
		if err != nil {
			report(InstallState{Err: fmt.Errorf(
				"Download interrupted: %v\n\nRun the installer again to resume.", err)})
			return
		}
		defer os.Remove(binTmp)

		binDir := platformBinDir(installDir)
		if err := os.MkdirAll(binDir, 0755); err != nil {
			report(InstallState{Err: fmt.Errorf("Could not create bin directory: %v", err)})
			return
		}

		// Verify bin integrity before extracting
		if expectedHash := fetchAssetChecksum(latest, platformBinAsset()); expectedHash != "" {
			if err := verifyDownloadChecksum(binTmp, expectedHash); err != nil {
				os.Remove(binTmp)
				report(InstallState{Err: fmt.Errorf("Bin download integrity check failed: %v", err)})
				return
			}
		}

		report(InstallState{Status: "Extracting game binaries...", Progress: 0.85})
		// tf2vintage-bin.zip has "bin/x64/..." paths verbatim — extract into installDir
		// so the zip reconstructs installDir/bin/x64/ (or bin/linux64/) correctly.
		if err := extractZipRaw(binTmp, installDir); err != nil {
			report(InstallState{Err: fmt.Errorf("Failed to extract game binaries: %v", err)})
			return
		}
		if runtime.GOOS != "windows" {
			chmodSo(binDir)
		}
		if err := validateBinDir(binDir); err != nil {
			report(InstallState{Err: fmt.Errorf(
				"Binaries failed validation: %v\n\nRun the installer again to retry.", err)})
			return
		}
	}

	// ── Ask about symbol downloads ───────────────────────────────────────────
	wantsSymbols := askSymbols()
	cfg := UpdaterConfig{DownloadSymbols: wantsSymbols}

	// ── Copy updater into install location ────────────────────────────────────
	report(InstallState{Status: "Installing updater...", Progress: 0.88})
	exe, _ := os.Executable()
	binDir := platformBinDir(installDir)
	updaterDest := filepath.Join(binDir, updaterName())

	if err := copyFile(exe, updaterDest); err != nil {
		report(InstallState{Err: fmt.Errorf(
			"Failed to install updater: %v\n\n"+
				"If TF2 Vintage is already running, close it and try again.", err)})
		return
	}
	if runtime.GOOS != "windows" {
		os.Chmod(updaterDest, 0755)
	}

	// Save config so the updater remembers symbol preference on future runs
	if err := saveConfig(binDir, cfg); err != nil {
		termWarn("Could not save updater config: %v", err)
	}

	// Download symbols immediately if opted in
	if cfg.DownloadSymbols {
		report(InstallState{Status: "Downloading debug symbols...", Progress: 0.89})
		if latest != nil {
			// Fresh install: binDir is live and staging simultaneously — no distinction needed.
			switch err := updateSymbols(binDir, binDir, latest); {
			case err == nil, errors.Is(err, errUpToDate):
				// ok
			default:
				termWarn("Symbol download failed: %v — you can retry with --enable-symbols", err)
			}
		}
	}

	finalize(report, updaterDest)

	// Remove the downloaded installer from wherever the user ran it from
	if exe != updaterDest {
		scheduleDelete(exe)
	}
}

// ── Diagnosis ─────────────────────────────────────────────────────────────────

func diagnose(sourcemods, installDir string) installNeeds {
	needs := installNeeds{sourcemods: sourcemods, installDir: installDir}

	if _, err := os.Stat(installDir); os.IsNotExist(err) {
		needs.freshInstall = true
		needs.repairBase = true
		needs.repairBin = true
		needs.reason = "No existing installation found — performing fresh install."
		return needs
	}

	manifest := loadLocalManifest(filepath.Join(installDir, "base-manifest.json"))
	if manifest == nil || manifest.Tag == "" {
		needs.repairBase = true
	}
	for _, dir := range []string{"maps", "materials", "models", "sound"} {
		if _, err := os.Stat(filepath.Join(installDir, dir)); os.IsNotExist(err) {
			needs.repairBase = true
			break
		}
	}

	if err := validateBinDir(platformBinDir(installDir)); err != nil {
		needs.repairBin = true
	}

	switch {
	case needs.repairBase && needs.repairBin:
		needs.reason = "Installation appears incomplete or corrupted — repairing base and binaries."
	case needs.repairBase:
		needs.reason = "Base assets are missing or corrupted — repairing base."
	case needs.repairBin:
		needs.reason = "Game binaries are missing or corrupted — repairing binaries."
	default:
		needs.reason = "Existing installation looks healthy."
	}
	return needs
}

// ── Post-install ──────────────────────────────────────────────────────────────

func finalize(report func(InstallState), updaterPath string) {
	report(InstallState{Status: "Creating desktop shortcut...", Progress: 0.94})
	if err := createDesktopShortcut(updaterPath); err != nil {
		// Non-fatal: the user can still launch manually via Steam.
		termWarn("Could not create desktop shortcut: %v", err)
		report(InstallState{
			Status:   "Desktop shortcut could not be created — launch TF2 Vintage from Steam instead.",
			Progress: 0.95,
		})
	}

	report(InstallState{
		Status:   "Installation complete!\nA desktop shortcut has been created for TF2 Vintage.",
		Progress: 1.0,
		Done:     true,
	})
}

// ── Helpers ───────────────────────────────────────────────────────────────────

func updaterName() string {
	if runtime.GOOS == "windows" {
		return "tf2vintage-updater.exe"
	}
	return "tf2vintage-updater"
}

func copyFile(src, dst string) error {
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()
	if err := os.MkdirAll(filepath.Dir(dst), 0755); err != nil {
		return err
	}
	out, err := os.Create(dst)
	if err != nil {
		return err
	}
	defer out.Close()
	_, err = copyIO(in, out)
	return err
}
