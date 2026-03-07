package main

import (
	"archive/zip"
	"crypto/md5"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strings"
)

// ── GitHub API types ──────────────────────────────────────────────────────────

// BaseManifest records the state of the game-asset tree for a given release.
// It is published as a standalone base-manifest.json asset so the updater can
// check the installed version cheaply before deciding which zip to download.
type BaseManifest struct {
	Tag        string            `json:"tag"`
	PrevTag    string            `json:"prev_tag"`
	ReleasedAt string            `json:"released_at"` // YYYY-MM-DD; used for 180-day staleness check
	Files      map[string]string `json:"files"`
}

// ── Manifest helpers ──────────────────────────────────────────────────────────

func fetchReleaseManifest(r *ghRelease) (*BaseManifest, error) {
	url := assetURL(r, "base-manifest.json")
	if url == "" {
		return nil, fmt.Errorf("no base-manifest.json in release %s", r.TagName)
	}
	return fetchManifest(url)
}

func fetchManifest(url string) (*BaseManifest, error) {
	resp, err := http.Get(url) //nolint:noctx
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	var m BaseManifest
	if err := json.NewDecoder(resp.Body).Decode(&m); err != nil {
		return nil, err
	}
	return &m, nil
}

func loadLocalManifest(path string) *BaseManifest {
	b, err := os.ReadFile(path)
	if err != nil {
		return nil
	}
	var m BaseManifest
	if err := json.Unmarshal(b, &m); err != nil {
		return nil
	}
	return &m
}

func saveManifest(path string, m *BaseManifest) {
	b, err := json.MarshalIndent(m, "", "  ")
	if err != nil {
		return
	}
	os.WriteFile(path, b, 0644)
}

func md5File(path string) (string, error) {
	f, err := os.Open(path)
	if err != nil {
		return "", err
	}
	defer f.Close()
	h := md5.New()
	if _, err := io.Copy(h, f); err != nil {
		return "", err
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}

// ── Extraction ────────────────────────────────────────────────────────────────

// extractZip extracts a zip, stripping the single top-level wrapper directory
// that zip tools add when zipping a folder (e.g. "tf2vintage/maps/foo" → "maps/foo").
// Use extractZipRaw when entries should be extracted verbatim (no stripping).
func extractZip(src, destDir string) error {
	r, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		parts := strings.SplitN(f.Name, "/", 2)
		relPath := f.Name
		if len(parts) == 2 {
			relPath = parts[1]
		}
		if relPath == "" {
			continue
		}

		target := filepath.Join(destDir, filepath.FromSlash(relPath))
		if f.FileInfo().IsDir() {
			os.MkdirAll(target, 0755)
			continue
		}

		os.MkdirAll(filepath.Dir(target), 0755)
		out, err := os.OpenFile(target, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, f.Mode())
		if err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			out.Close()
			return err
		}
		_, cerr := copyIO(rc, out)
		rc.Close()
		out.Close()
		if cerr != nil {
			return cerr
		}
	}
	return nil
}

// extractZipRaw extracts a zip verbatim — no leading path component is stripped.
// Use this when the zip already encodes the full relative path you want on disk,
// e.g. tf2vintage-nightly.zip which contains "bin/x64/server.dll".
func extractZipRaw(src, destDir string) error {
	r, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		relPath := filepath.FromSlash(f.Name)
		if relPath == "" || f.FileInfo().IsDir() {
			if f.FileInfo().IsDir() {
				os.MkdirAll(filepath.Join(destDir, relPath), 0755)
			}
			continue
		}
		target := filepath.Join(destDir, relPath)
		os.MkdirAll(filepath.Dir(target), 0755)
		out, err := os.OpenFile(target, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, f.Mode())
		if err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			out.Close()
			return err
		}
		_, cerr := copyIO(rc, out)
		rc.Close()
		out.Close()
		if cerr != nil {
			return cerr
		}
	}
	return nil
}

// extractZipRouted extracts tf2vintage-full.zip or tf2vintage-diff.zip, routing
// entries to the correct destination directory based on their path prefix:
//
//   - "tf2vintage/..."     → destModDir  (game-asset tree; "tf2vintage/" prefix stripped)
//   - "bin/..."            → stagingRoot  (verbatim; reconstructs bin/x64/ or bin/linux64/)
//   - "base-manifest.json" → destModDir/base-manifest.json
//
// destModDir is always a staging directory, never the live install — the caller
// is responsible for passing the correct staging path so that atomicSwapDir can
// commit the manifest alongside the rest of the game-asset tree atomically.
//
// This mirrors how package-combined assembles those zips in CI.
func extractZipRouted(src, destModDir, stagingRoot string) error {
	r, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		name := filepath.ToSlash(f.Name)

		var target string
		switch {
		case name == "base-manifest.json":
			target = filepath.Join(destModDir, "base-manifest.json")
		case strings.HasPrefix(name, "tf2vintage/"):
			rel := strings.TrimPrefix(name, "tf2vintage/")
			if rel == "" {
				continue
			}
			target = filepath.Join(destModDir, filepath.FromSlash(rel))
		case strings.HasPrefix(name, "bin/"):
			target = filepath.Join(stagingRoot, filepath.FromSlash(name))
		default:
			continue // unknown prefix — skip
		}

		if f.FileInfo().IsDir() {
			os.MkdirAll(target, 0755)
			continue
		}

		os.MkdirAll(filepath.Dir(target), 0755)
		out, err := os.OpenFile(target, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, f.Mode())
		if err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			out.Close()
			return err
		}
		_, cerr := copyIO(rc, out)
		rc.Close()
		out.Close()
		if cerr != nil {
			return cerr
		}
	}
	return nil
}

// extractZipFiltered extracts a zip, calling filter(entryName) for each entry.
// filter returns (destRelPath, include) — if include is false the entry is skipped,
// otherwise it is extracted to destDir/destRelPath.
func extractZipFiltered(src, destDir string, filter func(string) (string, bool)) error {
	r, err := zip.OpenReader(src)
	if err != nil {
		return err
	}
	defer r.Close()

	for _, f := range r.File {
		relPath, include := filter(filepath.ToSlash(f.Name))
		if !include || relPath == "" {
			continue
		}

		target := filepath.Join(destDir, filepath.FromSlash(relPath))
		if f.FileInfo().IsDir() {
			os.MkdirAll(target, 0755)
			continue
		}

		os.MkdirAll(filepath.Dir(target), 0755)
		out, err := os.OpenFile(target, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, f.Mode())
		if err != nil {
			return err
		}
		rc, err := f.Open()
		if err != nil {
			out.Close()
			return err
		}
		_, cerr := copyIO(rc, out)
		rc.Close()
		out.Close()
		if cerr != nil {
			return cerr
		}
	}
	return nil
}

func copyIO(r io.Reader, w io.Writer) (int64, error) {
	return io.Copy(w, r)
}

// chmodSo restores execute permissions on .so files after zip extraction.
func chmodSo(dir string) {
	filepath.WalkDir(dir, func(path string, d os.DirEntry, err error) error {
		if err != nil || d.IsDir() {
			return err
		}
		if strings.HasSuffix(path, ".so") {
			os.Chmod(path, 0755)
		}
		return nil
	})
}

// ── Asset name constants ──────────────────────────────────────────────────────

// fullInstallAsset is the single zip that contains the complete game-asset tree
// plus both platform bin trees. Used for fresh installs and staleness-triggered
// full re-downloads.
func fullInstallAsset() string { return "tf2vintage-full.zip" }

// diffInstallAsset is the zip that contains only the base files changed since
// the previous full release, plus both platform bin trees. Used for incremental
// updates when the local install is recent and a diff is available.
func diffInstallAsset() string { return "tf2vintage-diff.zip" }

// nightlyBinAsset is the zip published with every nightly pre-release.
// It contains only the debug bin trees (no game assets).
func nightlyBinAsset() string { return "tf2vintage-nightly.zip" }

// ── Misc helpers ──────────────────────────────────────────────────────────────

func readField(path, key string) string {
	b, err := os.ReadFile(path)
	if err != nil {
		return ""
	}
	prefix := key + "="
	for _, line := range strings.Split(string(b), "\n") {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, prefix) {
			return strings.TrimSpace(strings.TrimPrefix(line, prefix))
		}
	}
	return ""
}

func shortOrNone(commit string) string {
	if commit == "" {
		return "(none)"
	}
	if len(commit) > 9 {
		return commit[:9]
	}
	return commit
}

func launchGame(steamArgs []string) {
	if len(steamArgs) == 0 {
		return
	}
	cmd := newCmd(steamArgs[0], steamArgs[1:]...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Run()
}

func launchIfNotStandalone(standalone bool, steamArgs []string) {
	if standalone {
		termPause()
	} else {
		launchGame(steamArgs)
	}
}

func termPrintBanner() {
	fmt.Println("================================")
	fmt.Println("   TF2 Vintage Updater")
	fmt.Println("================================")
	fmt.Println()
}

func termPrint(msg string) {
	fmt.Println(msg)
}

func termPause() {
	fmt.Println()
	fmt.Print("Press Enter to close...")
	fmt.Scanln()
}

func termWarn(format string, args ...any) {
	fmt.Fprintf(os.Stderr, "[WARN] "+format+"\n", args...)
}

func termFatal(format string, args ...any) {
	fmt.Fprintf(os.Stderr, "[ERROR] "+format+"\n", args...)
	termPause()
	os.Exit(1)
}

// atomicSwapDir replaces liveDir with stagingDir using a backup-swap-cleanup
// sequence. If the rename of stagingDir→liveDir fails, the original is restored
// from backup. The backup is removed on success.
//
// Both liveDir and stagingDir must be on the same filesystem for os.Rename to
// be atomic. stagingDir is consumed (moved) by this call.
func atomicSwapDir(liveDir, stagingDir string) error {
	backupDir := liveDir + ".old"
	os.RemoveAll(backupDir) // clear any leftover from a previous failed update

	// Step 1: move live → backup (fast, same-fs rename)
	if err := os.Rename(liveDir, backupDir); err != nil {
		// Live dir may not exist yet (first install) — that's fine
		if !os.IsNotExist(err) {
			return fmt.Errorf("could not back up %s: %v", liveDir, err)
		}
	}

	// Step 2: move staging → live
	if err := os.Rename(stagingDir, liveDir); err != nil {
		// Rollback: restore backup
		if rerr := os.Rename(backupDir, liveDir); rerr != nil {
			return fmt.Errorf(
				"swap failed (%v) AND rollback failed (%v) — manual recovery needed: restore %s to %s",
				err, rerr, backupDir, liveDir)
		}
		return fmt.Errorf("atomic swap failed (original restored): %v", err)
	}

	// Step 3: remove backup
	os.RemoveAll(backupDir)
	return nil
}

// ── Remote version helpers ────────────────────────────────────────────────────

// fetchRemoteField downloads a small key=value text file from url and returns
// the value for the given key. Used to read nightly-version.txt cheaply
// (a few hundred bytes) without downloading the full zip.
func fetchRemoteField(url, key string) (string, error) {
	resp, err := http.Get(url) //nolint:noctx
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("HTTP %s fetching %s", resp.Status, url)
	}
	b, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}
	return parseField(string(b), key), nil
}

// parseField extracts a value from a key=value text block (one entry per line).
func parseField(text, key string) string {
	for _, line := range strings.Split(text, "\n") {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, key+"=") {
			return strings.TrimSpace(line[len(key)+1:])
		}
	}
	return ""
}
