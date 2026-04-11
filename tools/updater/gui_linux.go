//go:build !windows

package main

import (
	"fmt"
	"os"
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
		if state.Err != nil {
			installErr = state.Err
			close(done)
		}
		if state.Done {
			close(done)
		}
	}

	askAltPath := func() string {
		return ""
	}

	go install(report, askAltPath)
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
