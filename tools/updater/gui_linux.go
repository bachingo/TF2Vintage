//go:build !windows

package main

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

func startInstallUI(install func(func(InstallState), func() string, func() bool)) {
	termPrintBanner()
	fmt.Println("Install mode — TF2 Vintage not found in current directory.")
	fmt.Println()

	done := make(chan struct{})
	var installErr error

	report := func(state InstallState) {
		if state.Status != "" {
			fmt.Println(state.Status)
		}
		if state.Progress > 0 && state.Progress < 1.0 {
			fmt.Printf("  [%.0f%%]\n", state.Progress*100)
		}
		if state.Err != nil {
			installErr = state.Err
			close(done)
		}
		if state.Done {
			close(done)
		}
	}

	askAltPath := func() string {
		fmt.Println("Install location:")
		fmt.Println("  TF2 Vintage will be installed to your Steam Sourcemods folder by default.")
		fmt.Println("  To install game files on a different drive, enter the path now.")
		fmt.Println("  Leave blank and press Enter to use the default location.")
		fmt.Println()
		fmt.Print("  Alternate path (or Enter for default): ")

		scanner := bufio.NewScanner(os.Stdin)
		scanner.Scan()
		path := strings.TrimSpace(scanner.Text())

		if path == "" {
			fmt.Println("  Using default Sourcemods location.")
		} else {
			fmt.Printf("  Installing to: %s\n", path)
			fmt.Println("  A symlink will be created from Sourcemods/tf2vintage to that location.")
		}
		fmt.Println()
		return path
	}

	askSymbols := func() bool {
		fmt.Println("Debug symbols:")
		fmt.Println("  Symbols let contributors read crash reports with full function names")
		fmt.Println("  and source file locations. Not needed for normal play.")
		fmt.Println("  Adds ~50-200 MB per update.")
		fmt.Println()
		fmt.Print("  Download debug symbols? [y/N]: ")

		scanner := bufio.NewScanner(os.Stdin)
		scanner.Scan()
		answer := strings.TrimSpace(strings.ToLower(scanner.Text()))
		yes := answer == "y" || answer == "yes"
		if yes {
			fmt.Println("  Symbols will be downloaded with each update.")
			fmt.Println("  To disable later: tf2vintage-updater --disable-symbols")
		} else {
			fmt.Println("  Skipping symbols.")
			fmt.Println("  To enable later: tf2vintage-updater --enable-symbols")
		}
		fmt.Println()
		return yes
	}

	go install(report, askAltPath, askSymbols)
	<-done

	if installErr != nil {
		fmt.Fprintf(os.Stderr, "\n[ERROR] Installation failed: %v\n", installErr)
		termPause()
		os.Exit(1)
	}

	fmt.Println("\nInstallation complete!")
	fmt.Println("A desktop shortcut (tf2vintage.desktop) has been created on your Desktop.")
	fmt.Println("Double-click it to launch TF2 Vintage.")
}
