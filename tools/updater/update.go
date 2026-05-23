package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"time"
)

// errUpToDate is returned by updateInstall/updateNightly/updateSymbols when the
// installed version already matches the latest release. Nothing is written to
// the staging directory in this case, so the caller must not atomicSwapDir.
var errUpToDate = errors.New("already up to date")

func runUpdateMode(exe string) {
	modDir := modDirFromExe(exe)
	liveBinDir := platformBinDir(modDir)

	// Staging root sits next to the mod dir (same filesystem parent), mirroring
	// modDir's layout exactly: bin/<platform>/ for binaries, everything else for
	// game assets. atomicSwapDir(modDir, stagingRoot) then renames two siblings,
	// which is always valid because they share the same parent directory.
	stagingRoot := modDir + ".staging"
	stagingBinDir := filepath.Join(stagingRoot, "bin", binDirName())
	stagingModDir := stagingRoot // assets go at root of staging, not in a subdir

	// ── Handle config flags ───────────────────────────────────────────────────
	// These read/write config from the mod root and exit immediately.
	if len(os.Args) > 1 {
		switch os.Args[1] {
		case "--enable-symbols":
			cfg := loadConfig(modDir)
			cfg.DownloadSymbols = true
			if err := saveConfig(modDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Symbol downloads enabled.")
			fmt.Println("Debug symbols will be downloaded on the next update.")
			fmt.Println("To disable: run tf2vintage-updater --disable-symbols")
			termPause()
			os.Exit(0)
		case "--disable-symbols":
			cfg := loadConfig(modDir)
			cfg.DownloadSymbols = false
			if err := saveConfig(modDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Symbol downloads disabled.")
			termPause()
			os.Exit(0)
		case "--enable-nightly":
			cfg := loadConfig(modDir)
			cfg.CheckNightly = true
			if err := saveConfig(modDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Nightly updates enabled.")
			fmt.Println("The updater will check for nightly pre-releases on each launch.")
			fmt.Println("Note: nightlies contain only binary updates, not game asset changes.")
			fmt.Println("To disable: run tf2vintage-updater --disable-nightly")
			termPause()
			os.Exit(0)
		case "--disable-nightly":
			cfg := loadConfig(modDir)
			cfg.CheckNightly = false
			if err := saveConfig(modDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Nightly updates disabled.")
			fmt.Println("The updater will only install stable releases.")
			termPause()
			os.Exit(0)
		}
	}

	originalArgs := os.Args[1:]
	standaloneMode := len(originalArgs) == 0

	termPrintBanner()

	// ── Acquire lockfile (against live bin dir) ───────────────────────────────
	release, err := acquireLock(liveBinDir)
	if err != nil {
		termFatal("%v", err)
	}
	defer release()

	// Set up staging directory (after lock, so only one instance uses it)
	os.RemoveAll(stagingRoot)
	if err := os.MkdirAll(stagingBinDir, 0755); err != nil {
		termFatal("Could not create staging directory: %v", err)
	}
	// stagingModDir == stagingRoot, already created above via MkdirAll(stagingBinDir)

	// ── Load user config from mod root ───────────────────────────────────────
	cfg := loadConfig(modDir)

	// ── Check disk space (staging is beside modDir, same drive) ──────────────
	if err := checkDiskSpace(modDir, minFreeBytesForBins+minFreeBytesForBase); err != nil {
		termWarn("Disk space check failed: %v — proceeding anyway", err)
	}

	// ── Fetch latest stable release (cached) ─────────────────────────────────
	termPrint("Checking for updates...")
	latest, err := fetchLatestRelease()
	if err != nil {
		termWarn("Could not check for updates (%v) — launching game.", err)
		os.RemoveAll(stagingRoot)
		launchGame(gameExePath(modDir))
		return
	}

	// ── Verify own integrity against published checksum ───────────────────────
	if err := verifyUpdaterChecksum(latest); err != nil {
		termFatal("Integrity check failed: %v", err)
	}

	// ── Optionally fetch the nightly pre-release (silent check) ──────────────
	var nightlyRelease *ghRelease
	if cfg.CheckNightly {
		if n, err := fetchNightlyRelease(); err != nil {
			termWarn("Could not check for nightly (%v) — using stable release.", err)
		} else {
			nightlyRelease = n
		}
	}

	// ── Download updates into staging ────────────────────────────────────────
	binUpdated := false
	baseUpdated := false

	if nightlyRelease != nil {
		switch err := updateNightly(liveBinDir, stagingBinDir, nightlyRelease); {
		case err == nil:
			binUpdated = true
		case errors.Is(err, errUpToDate):
		default:
			termWarn("Nightly update failed: %v — falling back to stable.", err)
		}
	}

	switch err := updateInstall(modDir, stagingRoot, stagingBinDir, stagingModDir, latest); {
	case err == nil:
		binUpdated = true
		baseUpdated = true
	case errors.Is(err, errUpToDate):
		// nothing staged — do not swap
	default:
		termWarn("Update failed: %v", err)
	}

	if cfg.DownloadSymbols {
		switch err := updateSymbols(liveBinDir, stagingBinDir, latest); {
		case err == nil:
			binUpdated = true
		case errors.Is(err, errUpToDate):
		default:
			termWarn("Symbol update failed: %v", err)
		}
	}

	// ── Atomic swap: move staging into place ──────────────────────────────────
	if binUpdated || baseUpdated {
		fmt.Println("Applying update...")
		if err := swapBinDir(modDir, stagingRoot); err != nil {
			termFatal("Swap failed: %v", err)
		}
		fmt.Println("Update applied.")
	}

	// Clean up staging root
	os.RemoveAll(stagingRoot)

	fmt.Println()
	if standaloneMode {
		// Launched directly (not via game launcher) — autostart the game.
		fmt.Println("Launching TF2 Vintage...")
		time.Sleep(2 * time.Second)
		launchGame(gameExePath(modDir))
		return
	}
	// Launched with args — pass them along to the game (e.g. Steam launch args).
	fmt.Println("Launching TF2 Vintage...")
	time.Sleep(2 * time.Second)
	launchGame(append(gameExePath(modDir), originalArgs...))
}

// ── Nightly bin update ────────────────────────────────────────────────────────

// updateNightly checks the nightly-version.txt asset against the locally
// installed version-bin.txt and, if newer, downloads and stages
// tf2vintage-nightly.zip (debug bins only — no game assets).
//
// All output is intentionally suppressed. Nightly is opt-in for advanced users
// who do not need the updater narrating background checks. Only actual failures
// are surfaced via termWarn at the call site.
//
// Returns nil if the nightly was staged, errUpToDate if already current.
func updateNightly(liveBinDir, stagingBinDir string, nightly *ghRelease) error {
	versionURL := assetURL(nightly, "nightly-version.txt")
	if versionURL == "" {
		return fmt.Errorf("nightly release is missing nightly-version.txt")
	}

	remoteShort, err := fetchRemoteField(versionURL, "short")
	if err != nil {
		return fmt.Errorf("could not read nightly version: %v", err)
	}
	if remoteShort == "" {
		return fmt.Errorf("nightly-version.txt missing 'short' field")
	}

	localShort := readField(filepath.Join(liveBinDir, "version-bin.txt"), "short")
	if localShort != "" && localShort == remoteShort {
		return errUpToDate // already current — say nothing
	}

	url := assetURL(nightly, nightlyBinAsset())
	if url == "" {
		return fmt.Errorf("nightly zip asset not found in pre-release")
	}

	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}

	if expectedHash := fetchAssetChecksum(nightly, nightlyBinAsset()); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("nightly integrity check failed: %v", err)
		}
	}

	// tf2vintage-nightly.zip contains bin/x64/** and bin/linux64/** verbatim,
	// plus the launcher and updater at the root level.
	stagingRoot := filepath.Dir(filepath.Dir(stagingBinDir))
	if err := extractZipRaw(tmp, stagingRoot); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("nightly extraction failed: %v", err)
	}
	os.Remove(tmp)

	if runtime.GOOS != "windows" {
		chmodSo(stagingBinDir)
	}

	if err := validateBinDir(stagingBinDir); err != nil {
		return fmt.Errorf("nightly staging failed validation: %v", err)
	}

	return nil
}

// ── Unified install update ────────────────────────────────────────────────────

// updateInstall checks the remote base-manifest.json against the locally
// installed one and downloads exactly ONE zip to bring the install up to date:
//
//   - No local manifest OR install > 180 days old → tf2vintage-full.zip
//   - Local manifest present, recent, and diff available → tf2vintage-diff.zip
//   - Local manifest present, recent, but no diff (first release) → tf2vintage-full.zip
//   - Already up to date → errUpToDate (nothing staged)
//
// Both full and diff zips contain the complete bin trees for both platforms, so
// a single download always updates both game assets and binaries together.
//
// modDir is read-only. Content is written into stagingModDir (base assets) and
// stagingBinDir (binaries) inside stagingRoot.
func updateInstall(modDir, stagingRoot, stagingBinDir, stagingModDir string, latest *ghRelease) error {
	// Fetch the remote manifest (small JSON) to check the release tag cheaply.
	manifestURL := assetURL(latest, "base-manifest.json")
	if manifestURL == "" {
		// No base-manifest.json in this release — either a nightly-only release
		// or the release assets haven't been published yet.
		return fmt.Errorf("release %s has no base-manifest.json asset — the release may not be fully published yet", latest.TagName)
	}

	remoteManifest, err := fetchManifest(manifestURL)
	if err != nil {
		return fmt.Errorf("could not fetch remote manifest: %v", err)
	}

	localManifestPath := filepath.Join(modDir, "base-manifest.json")
	localManifest := loadLocalManifest(localManifestPath)

	// ── Case 1: fresh install ─────────────────────────────────────────────────
	if localManifest == nil {
		fmt.Println("No base install found — downloading full package (this may take a while)...")
		return downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir, latest, remoteManifest)
	}

	// ── Case 2: already up to date ───────────────────────────────────────────
	if localManifest.Tag == remoteManifest.Tag {
		fmt.Println("Game is up to date.")
		return errUpToDate
	}

	// ── Case 3: staleness check — force full if install is too old ────────────
	// Caps the maximum diff gap and guards against deeply corrupted installs.
	const maxInstallAgeDays = 180
	if localManifest.ReleasedAt != "" {
		if relDate, parseErr := time.Parse("2006-01-02", localManifest.ReleasedAt); parseErr == nil {
			ageDays := int(time.Since(relDate).Hours() / 24)
			if ageDays > maxInstallAgeDays {
				fmt.Printf("Install is %d days old (limit %d) — downloading full package...\n",
					ageDays, maxInstallAgeDays)
				return downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir, latest, remoteManifest)
			}
		}
	}

	// ── Case 4: incremental diff update ──────────────────────────────────────
	fmt.Printf("Update available: %s → %s\n", localManifest.Tag, remoteManifest.Tag)

	diffURL := assetURL(latest, diffInstallAsset())
	if diffURL == "" {
		// No diff available (first release, or diff not published) — fall back to full.
		fmt.Println("No diff available — downloading full package...")
		return downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir, latest, remoteManifest)
	}

	fmt.Printf("Downloading %s...\n", diffInstallAsset())
	tmp, err := downloadWithProgress(diffURL)
	if err != nil {
		fmt.Printf("Diff download failed (%v) — falling back to full package...\n", err)
		return downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir, latest, remoteManifest)
	}

	if expectedHash := fetchAssetChecksum(latest, diffInstallAsset()); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			fmt.Printf("Diff integrity check failed (%v) — falling back to full package...\n", err)
			return downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir, latest, remoteManifest)
		}
	}

	fmt.Println("Extracting diff update to staging...")
	// The diff zip contains "tf2vintage/**" (changed game files), "bin/**" (full bins),
	// and "base-manifest.json" at root. extractZipRouted routes each to the correct dir.
	//
	// For the game-asset tree we need the CURRENT files as a base first, because
	// the diff only carries changed/added files — unchanged files stay from live.
	if err := copyDir(modDir, stagingModDir); err != nil {
		os.Remove(tmp)
		fmt.Printf("Could not seed staging from current install (%v) — falling back to full...\n", err)
		os.RemoveAll(stagingModDir)
		os.MkdirAll(stagingModDir, 0755)
		return downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir, latest, remoteManifest)
	}

	if err := extractZipRouted(tmp, stagingModDir, stagingRoot); err != nil {
		os.Remove(tmp)
		os.RemoveAll(stagingModDir)
		os.MkdirAll(stagingModDir, 0755)
		fmt.Printf("Diff extraction failed (%v) — falling back to full package...\n", err)
		return downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir, latest, remoteManifest)
	}
	os.Remove(tmp)

	if runtime.GOOS != "windows" {
		chmodSo(stagingBinDir)
	}
	if err := validateBinDir(stagingBinDir); err != nil {
		return fmt.Errorf("diff staging failed bin validation: %v", err)
	}

	fmt.Println("Diff applied in staging.")
	return nil
}

// downloadAndExtractFull downloads tf2vintage-full.zip and extracts it into staging.
// The full zip contains the complete game-asset tree + both platform bin trees.
func downloadAndExtractFull(stagingRoot, stagingBinDir, stagingModDir string, latest *ghRelease, manifest *BaseManifest) error {
	// Full zip contains the complete game-asset tree AND both platform bin trees,
	// so reserve space for both components before downloading.
	if err := checkDiskSpace(stagingModDir, minFreeBytesForBase+minFreeBytesForBins); err != nil {
		return err
	}

	url := assetURL(latest, fullInstallAsset())
	if url == "" {
		return fmt.Errorf("full package asset not found in release")
	}

	fmt.Printf("Downloading %s...\n", fullInstallAsset())
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}

	if expectedHash := fetchAssetChecksum(latest, fullInstallAsset()); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("full package integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting full package to staging...")
	if err := extractZipRouted(tmp, stagingModDir, stagingRoot); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("full package extraction failed: %v", err)
	}
	os.Remove(tmp)

	if err := validateModRoot(stagingModDir); err != nil {
		return fmt.Errorf("extraction validation failed: %v", err)
	}

	if runtime.GOOS != "windows" {
		chmodSo(stagingBinDir)
	}
	if err := validateBinDir(stagingBinDir); err != nil {
		return fmt.Errorf("full package staging failed bin validation: %v", err)
	}

	fmt.Println("Full package staged.")
	return nil
}
