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

type BaseManifest struct {
	Tag     string            `json:"tag"`
	PrevTag string            `json:"prev_tag"`
	Files   map[string]string `json:"files"`
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
	resp, err := http.Get(url)
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

// ── Misc helpers ──────────────────────────────────────────────────────────────

func platformBinAsset() string {
	return "tf2vintage-bin.zip"
}

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

func AtomicUpdate(binDir string, stagingDir string) error {
    backupDir := binDir + ".old"
    
    // 1. Move old to .old
    os.Rename(binDir, backupDir)
    
    // 2. Move staging to bin
    if err := os.Rename(stagingDir, binDir); err != nil {
        // Rollback: try to put the old one back if renaming fails
        os.Rename(backupDir, binDir)
        return err
    }
    
    // 3. Success: remove the backup
    return os.RemoveAll(backupDir)
}
