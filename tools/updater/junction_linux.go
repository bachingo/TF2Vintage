//go:build linux

package main

import "os"

// createLink creates a symlink from linkPath pointing to targetPath.
func createLink(linkPath, targetPath string) error {
	return os.Symlink(targetPath, linkPath)
}
