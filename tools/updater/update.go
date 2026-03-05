package main

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"time"
)

func runUpdateMode(exe string) {
	// bin/x64 → bin → tf2vintage
	modDir := filepath.Dir(filepath.Dir(filepath.Dir(exe)))
	binDir := filepath.Join(modDir, "bin", "x64")
	steamArgs := os.Args[1:]
	standaloneMode := len(steamArgs) == 0

	termPrintBanner()

	// ── Acquire lockfile ──────────────────────────────────────────────────────
	release, err := acquireLock(binDir)
	if err != nil {
		termFatal("%v", err)
	}
	defer release()

	// ── Fetch latest release (cached) ─────────────────────────────────────────
	termPrint("Checking for updates...")
	latest, err := fetchLatestRelease()
	if err != nil {
		termWarn("Could not check for updates (%v) — launching with current version.", err)
		launchIfNotStandalone(standaloneMode, steamArgs)
		return
	}

	// ── Verify own integrity against published checksum ───────────────────────
	if err := verifyUpdaterChecksum(latest); err != nil {
		termFatal("Integrity check failed:

%v", err)
	}

	// ── Bin update ────────────────────────────────────────────────────────────
	if err := updateBin(modDir, binDir, latest); err != nil {
		termWarn("Bin update failed: %v", err)
	}

	// ── Base update ───────────────────────────────────────────────────────────
	if err := updateBase(modDir, latest); err != nil {
		termWarn("Base update failed: %v", err)
	}

	fmt.Println()
	if standaloneMode {
		termPause()
		return
	}
	fmt.Println("Launching TF2 Vintage in 3 seconds...")
	time.Sleep(3 * time.Second)
	launchGame(steamArgs)
}

// ── Bin update ────────────────────────────────────────────────────────────────

func updateBin(modDir, binDir string, latest *ghRelease) error {
	remoteTag := latest.TagName
	localCommit := readField(filepath.Join(binDir, "version-bin.txt"), "commit")

	// Validate local commit looks like a real SHA before trusting it
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

	// Check disk space before downloading
	if err := checkDiskSpace(modDir, minFreeBytesForBins); err != nil {
		return err
	}

	fmt.Printf("Downloading %s...\n", platformBinAsset())
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	// Don't defer-remove the temp file — it's a stable resume path.
	// Only remove on success so interrupted downloads can resume.

	// Back up current bin before overwriting
	if err := backupBinDir(binDir); err != nil {
		termWarn("Could not create bin backup: %v — proceeding without backup", err)
	}

	// Verify bin download integrity before extracting
	if expectedHash := fetchAssetChecksum(latest, platformBinAsset()); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("bin download integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting binaries...")
	if err := extractZip(tmp, binDir); err != nil {
		// Extraction failed — attempt restore
		termWarn("Extraction failed: %v", err)
		if rerr := restoreBinDir(binDir); rerr != nil {
			return fmt.Errorf("extraction failed AND restore failed: %v (restore: %v)", err, rerr)
		}
		fmt.Println("Previous binaries restored.")
		return fmt.Errorf("bin update failed (previous version restored): %v", err)
	}

	if runtime.GOOS != "windows" {
		chmodSo(binDir)
	}

	// Validate the extracted bin before declaring success
	if err := validateBinDir(binDir); err != nil {
		termWarn("Bin validation failed after extraction: %v", err)
		if rerr := restoreBinDir(binDir); rerr != nil {
			return fmt.Errorf("validation failed AND restore failed: %v (restore: %v)", err, rerr)
		}
		fmt.Println("Previous binaries restored.")
		return fmt.Errorf("bin update failed validation (previous version restored): %v", err)
	}

	// Success — clean up temp file and stale backup
	os.Remove(tmp)
	os.RemoveAll(binDir + ".bak")
	fmt.Println("Binaries updated.")
	return nil
}

// ── Base update ───────────────────────────────────────────────────────────────

func updateBase(modDir string, latest *ghRelease) error {
	manifestURL := assetURL(latest, "base-manifest.json")
	if manifestURL == "" {
		return nil
	}

	remoteManifest, err := fetchManifest(manifestURL)
	if err != nil {
		return fmt.Errorf("could not fetch remote manifest: %v", err)
	}

	localManifestPath := filepath.Join(modDir, "base-manifest.json")
	localManifest := loadLocalManifest(localManifestPath)

	if localManifest == nil {
		fmt.Println("No base install found — downloading full base (this may take a while)...")
		return fullBaseDownload(modDir, latest, remoteManifest, localManifestPath)
	}

	if localManifest.Tag == remoteManifest.Tag {
		fmt.Println("Base is up to date.")
		return nil
	}

	fmt.Printf("Base update: %s → %s\n", localManifest.Tag, remoteManifest.Tag)

	chain, err := buildPatchChain(localManifest.Tag, latest)
	if err != nil {
		fmt.Printf("Patch chain unavailable (%v) — falling back to full base download...\n", err)
		return fullBaseDownload(modDir, latest, remoteManifest, localManifestPath)
	}

	fmt.Printf("Applying %d patch(es)...\n", len(chain))
	for i, release := range chain {
		fmt.Printf("[%d/%d] Applying patch %s\n", i+1, len(chain), release.TagName)
		if err := applyPatch(modDir, release); err != nil {
			fmt.Printf("Patch %s failed (%v) — falling back to full base download...\n", release.TagName, err)
			return fullBaseDownload(modDir, latest, remoteManifest, localManifestPath)
		}
	}

	saveManifest(localManifestPath, remoteManifest)
	fmt.Println("Base updated via patch chain.")
	return nil
}

// buildPatchChain walks backwards through releases using cached manifests,
// building the ordered list of patches to apply from localTag to latest.
func buildPatchChain(localTag string, latest *ghRelease) ([]*ghRelease, error) {
	const maxChain = 20
	var chain []*ghRelease
	current := latest

	for i := 0; i < maxChain; i++ {
		// fetchRelease is cache-aware — won't hit the API if already cached
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

func applyPatch(modDir string, release *ghRelease) error {
	url := assetURL(release, "base-patch.zip")
	if url == "" {
		return nil
	}
	tmp, err := downloadWithProgress(url)
	if err != nil {
		return err
	}
	defer os.Remove(tmp)
	return extractZip(tmp, modDir)
}

func fullBaseDownload(modDir string, latest *ghRelease, manifest *BaseManifest, localManifestPath string) error {
	if err := checkDiskSpace(modDir, minFreeBytesForBase); err != nil {
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

	// Verify base download integrity before extracting
	if expectedHash := fetchAssetChecksum(latest, "tf2vintage-base.zip"); expectedHash != "" {
		if err := verifyDownloadChecksum(tmp, expectedHash); err != nil {
			os.Remove(tmp)
			return fmt.Errorf("base download integrity check failed: %v", err)
		}
	}

	fmt.Println("Extracting base...")
	if err := extractZip(tmp, modDir); err != nil {
		return err
	}
	os.Remove(tmp)
	saveManifest(localManifestPath, manifest)
	fmt.Println("Base installed.")
	return nil
}
