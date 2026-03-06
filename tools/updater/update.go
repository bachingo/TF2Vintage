package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"time"
)

func runUpdateMode(exe string) {
	// bin/<platform> → bin → tf2vintage
	modDir := modDirFromExe(exe)
	liveBinDir := platformBinDir(modDir)

	// Staging root: sits next to the mod dir so it's on the same filesystem,
	// which is required for os.Rename to work atomically.
	stagingRoot := modDir + ".staging"
	stagingBinDir := filepath.Join(stagingRoot, "bin", binDirName())
	stagingModDir := filepath.Join(stagingRoot, "mod")

	// Clean up any leftover staging dir from a previous interrupted update
	os.RemoveAll(stagingRoot)
	if err := os.MkdirAll(stagingBinDir, 0755); err != nil {
		termFatal("Could not create staging directory: %v", err)
	}
	if err := os.MkdirAll(stagingModDir, 0755); err != nil {
		termFatal("Could not create staging directory: %v", err)
	}

	// ── Handle config flags ───────────────────────────────────────────────────
	// These read/write config from the live bin dir and exit immediately —
	// they don't participate in the update flow at all.
	if len(os.Args) > 1 {
		switch os.Args[1] {
		case "--enable-symbols":
			cfg := loadConfig(liveBinDir)
			cfg.DownloadSymbols = true
			if err := saveConfig(liveBinDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Symbol downloads enabled.")
			fmt.Println("Debug symbols will be downloaded on the next update.")
			fmt.Println("To disable: run tf2vintage-updater --disable-symbols")
			termPause()
			os.Exit(0)
		case "--disable-symbols":
			cfg := loadConfig(liveBinDir)
			cfg.DownloadSymbols = false
			if err := saveConfig(liveBinDir, cfg); err != nil {
				termFatal("Could not save config: %v", err)
			}
			fmt.Println("Symbol downloads disabled.")
			termPause()
			os.Exit(0)
		}
	}

	steamArgs := os.Args[1:]
	standaloneMode := len(steamArgs) == 0

	termPrintBanner()

	// ── Acquire lockfile (against live bin dir) ───────────────────────────────
	release, err := acquireLock(liveBinDir)
	if err != nil {
		termFatal("%v", err)
	}
	defer release()

	// ── Load user config from live location ──────────────────────────────────
	cfg := loadConfig(liveBinDir)

	// ── Check disk space (staging is beside modDir, same drive) ──────────────
	if err := checkDiskSpace(modDir, minFreeBytesForBins+minFreeBytesForBase); err != nil {
		termWarn("Disk space check failed: %v — proceeding anyway", err)
	}

	// ── Fetch latest release (cached) ────────────────────────────────────────
	termPrint("Checking for updates...")
	latest, err := fetchLatestRelease()
	if err != nil {
		termWarn("Could not check for updates (%v) — launching with current version.", err)
		os.RemoveAll(stagingRoot)
		launchIfNotStandalone(standaloneMode, steamArgs)
		return
	}

	// ── Verify own integrity against published checksum ───────────────────────
	if err := verifyUpdaterChecksum(latest); err != nil {
		termFatal("Integrity check failed: %v", err)
	}

	// ── Download all updates into staging ────────────────────────────────────
	binUpdated := false
	baseUpdated := false

	if err := updateBin(liveBinDir, stagingBinDir, latest); err != nil {
		termWarn("Bin update failed: %v", err)
	} else {
		binUpdated = true
	}

	if err := updateBase(modDir, stagingModDir, latest); err != nil {
		termWarn("Base update failed: %v", err)
	} else {
		baseUpdated = true
	}

	if cfg.DownloadSymbols {
		// liveBinDir for version check (persists across runs), stagingBinDir for extraction
		if err := updateSymbols(liveBinDir, stagingBinDir, latest); err != nil {
			termWarn("Symbol update failed: %v", err)
		}
	}

	// ── Atomic swap: move staging into place ──────────────────────────────────
	// Only swap the subtrees that were actually updated, so a bin failure
	// doesn't roll back a successful base update.
	if binUpdated {
		fmt.Println("Applying bin update...")
		if err := atomicSwapDir(liveBinDir, stagingBinDir); err != nil {
			termFatal("Bin atomic swap failed: %v", err)
		}
		fmt.Println("Binaries updated.")
	}

	if baseUpdated {
		fmt.Println("Applying base update...")
		if err := atomicSwapDir(modDir, stagingModDir); err != nil {
			termFatal("Base atomic swap failed: %v", err)
		}
		fmt.Println("Base updated.")
	}

	// Clean up staging root (now empty)
	os.RemoveAll(stagingRoot)

	fmt.Println()
	if standaloneMode {
		termPause()
		return
	}
	fmt.Println("Launching TF2 Vintage in 5 seconds...")
	time.Sleep(5 * time.Second)
	launchGame(steamArgs)
}

// ── Bin update ────────────────────────────────────────────────────────────────

// updateBin downloads the bin package into stagingBinDir.
// liveBinDir is read-only here — used only to check the current version.
// Returns nil only if a new version was downloaded and is ready to swap in.
// Returns nil immediately (no staging writes) if already up to date.
func updateBin(liveBinDir, stagingBinDir string, latest *ghRelease) error {
	remoteTag := latest.TagName
	localCommit := readField(filepath.Join(liveBinDir, "version-bin.txt"), "commit")

	if localCommit != "" && !validateCommitSHA(localCommit) {
		termWarn("version-bin.txt has invalid commit SHA — forcing bin update")
		localCommit = ""
	}

	if localCommit != "" && remoteTag == "build-"+localCommit {
		fmt.Println("Binaries are up to date.")
		return nil
	}

	fmt.Printf("Bin update: %s → %s\n", shortOrNone(localCommit), remoteTag)

	url := assetURL(latest, platformBinAsset())
	if url == "" {
		return fmt.Errorf("bin asset not found in release")
	}

	fmt.Printf("Downloading %s...\n", platformBinAsset())
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}

	// Verify integrity before extracting into staging
	if expectedHash := fetchAssetChecksum(latest, platformBinAsset()); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("bin download integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting binaries to staging...")
	if err := extractZip(tmp, stagingBinDir); err != nil {
		os.Remove(tmp)
		return fmt.Errorf("bin extraction failed: %v", err)
	}
	os.Remove(tmp)

	if runtime.GOOS != "windows" {
		chmodSo(stagingBinDir)
	}

	// Validate staging before we commit to swapping it in
	if err := validateBinDir(stagingBinDir); err != nil {
		return fmt.Errorf("bin staging failed validation: %v", err)
	}

	return nil
}

// ── Base update ───────────────────────────────────────────────────────────────

// updateBase downloads/patches base assets into stagingModDir.
// liveModDir is read-only — used only to check current manifest.
// Returns nil only if new content is ready in stagingModDir.
// Returns nil immediately (no staging writes) if already up to date.
func updateBase(liveModDir, stagingModDir string, latest *ghRelease) error {
	manifestURL := assetURL(latest, "base-manifest.json")
	if manifestURL == "" {
		return nil
	}

	remoteManifest, err := fetchManifest(manifestURL)
	if err != nil {
		return fmt.Errorf("could not fetch remote manifest: %v", err)
	}

	localManifestPath := filepath.Join(liveModDir, "base-manifest.json")
	localManifest := loadLocalManifest(localManifestPath)

	if localManifest == nil {
		fmt.Println("No base install found — downloading full base (this may take a while)...")
		return fullBaseDownload(stagingModDir, latest, remoteManifest,
			filepath.Join(stagingModDir, "base-manifest.json"))
	}

	if localManifest.Tag == remoteManifest.Tag {
		fmt.Println("Base is up to date.")
		return nil
	}

	fmt.Printf("Base update: %s → %s\n", localManifest.Tag, remoteManifest.Tag)

	chain, err := buildPatchChain(localManifest.Tag, latest)
	if err != nil {
		fmt.Printf("Patch chain unavailable (%v) — falling back to full base download...\n", err)
		return fullBaseDownload(stagingModDir, latest, remoteManifest,
			filepath.Join(stagingModDir, "base-manifest.json"))
	}

	// For patch application we need the current files as a base.
	// Copy live mod dir into staging first, then apply patches on top.
	fmt.Println("Seeding staging from current install...")
	if err := copyDir(liveModDir, stagingModDir); err != nil {
		fmt.Printf("Could not seed staging (%v) — falling back to full base download...\n", err)
		os.RemoveAll(stagingModDir)
		os.MkdirAll(stagingModDir, 0755)
		return fullBaseDownload(stagingModDir, latest, remoteManifest,
			filepath.Join(stagingModDir, "base-manifest.json"))
	}

	fmt.Printf("Applying %d patch(es)...\n", len(chain))
	for i, release := range chain {
		fmt.Printf("[%d/%d] Applying patch %s\n", i+1, len(chain), release.TagName)
		if err := applyPatch(stagingModDir, release); err != nil {
			fmt.Printf("Patch %s failed (%v) — falling back to full base download...\n", release.TagName, err)
			os.RemoveAll(stagingModDir)
			os.MkdirAll(stagingModDir, 0755)
			return fullBaseDownload(stagingModDir, latest, remoteManifest,
				filepath.Join(stagingModDir, "base-manifest.json"))
		}
	}

	saveManifest(filepath.Join(stagingModDir, "base-manifest.json"), remoteManifest)
	fmt.Println("Base patched in staging.")
	return nil
}

// buildPatchChain walks backwards through releases using cached manifests,
// building the ordered list of patches to apply from localTag to latest.
func buildPatchChain(localTag string, latest *ghRelease) ([]*ghRelease, error) {
	const maxChain = 20
	var chain []*ghRelease
	current := latest

	for i := 0; i < maxChain; i++ {
		manifest, err := fetchReleaseManifest(current)
		if err != nil {
			return nil, fmt.Errorf("could not fetch manifest for %s: %v", current.TagName, err)
		}

		chain = append([]*ghRelease{current}, chain...)

		if manifest.PrevTag == localTag {
			return chain, nil
		}
		if manifest.PrevTag == "" {
			return nil, fmt.Errorf("patch chain does not reach local tag %s", localTag)
		}

		prev, err := fetchRelease(manifest.PrevTag)
		if err != nil {
			return nil, fmt.Errorf("could not fetch release %s: %v", manifest.PrevTag, err)
		}
		current = prev
	}

	return nil, fmt.Errorf("patch chain exceeded %d steps", maxChain)
}

func applyPatch(destDir string, release *ghRelease) error {
	url := assetURL(release, "base-patch.zip")
	if url == "" {
		return nil
	}
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)
	return extractZip(tmp, destDir)
}

func fullBaseDownload(destDir string, latest *ghRelease, manifest *BaseManifest, localManifestPath string) error {
	if err := checkDiskSpace(destDir, minFreeBytesForBase); err != nil {
		return err
	}

	url := assetURL(latest, "tf2vintage-base.zip")
	if url == "" {
		return fmt.Errorf("full base asset not found in release")
	}
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}

	if expectedHash := fetchAssetChecksum(latest, "tf2vintage-base.zip"); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("base download integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting base to staging...")
	if err := extractZip(tmp, destDir); err != nil {
		os.Remove(tmp)
		return err
	}
	os.Remove(tmp)
	saveManifest(localManifestPath, manifest)
	fmt.Println("Base staged.")
	return nil
}
