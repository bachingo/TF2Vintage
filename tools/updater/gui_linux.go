//go:build !windows

package main

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

func startInstallUI(install func(func(InstallState), func() string)) {
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
		if state.ManualLaunch != "" {
			fmt.Println()
			fmt.Println("⚠  Steam launch option could not be set automatically.")
			fmt.Println("   Set it manually in Steam → Source SDK Base 2013 Multiplayer → Properties → Launch Options:")
			fmt.Println()
			fmt.Println("   " + state.ManualLaunch)
			fmt.Println()
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

	go install(report, askAltPath)
	<-done

	if installErr != nil {
		fmt.Fprintf(os.Stderr, "\n[ERROR] Installation failed: %v\n", installErr)
		termPause()
		os.Exit(1)
	}

	fmt.Println("\nInstallation complete!")
	fmt.Println("Steam is restarting — TF2 Vintage will appear in your library shortly.")
}
