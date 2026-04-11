//go:build windows

package main

import (
	"fmt"
	"os"

	"github.com/lxn/walk"
	. "github.com/lxn/walk/declarative"
)

func startInstallUI(install func(func(InstallState), func() string)) {
	var (
		mw          *walk.MainWindow
		statusLabel *walk.Label
		progressBar *walk.ProgressBar
		logBox      *walk.TextEdit
	)

	if err := (MainWindow{
		AssignTo: &mw,
		Title:    "TF2 Vintage Installer",
		MinSize:  Size{Width: 500, Height: 340},
		MaxSize:  Size{Width: 500, Height: 340},
		Layout:   VBox{Margins: Margins{Left: 16, Right: 16, Top: 16, Bottom: 16}},
		Children: []Widget{
			Label{
				Text: "Team Fortress 2: Vintage",
				Font: Font{Bold: true, PointSize: 12},
			},
			Label{
				AssignTo: &statusLabel,
				Text:     "Preparing...",
			},
			ProgressBar{
				AssignTo: &progressBar,
				MinValue: 0,
				MaxValue: 100,
			},
			TextEdit{
				AssignTo: &logBox,
				ReadOnly: true,
				VScroll:  true,
			},
		},
	}).Create(); err != nil {
		fmt.Fprintf(os.Stderr, "GUI error: %v\n", err)
		os.Exit(1)
	}
	mw.SetVisible(true)

	appendLog := func(msg string) {
		current := logBox.Text()
		if current != "" {
			logBox.SetText(current + "\r\n" + msg)
		} else {
			logBox.SetText(msg)
		}
	}

	report := func(state InstallState) {
		mw.Synchronize(func() {
			if state.Status != "" {
				statusLabel.SetText(state.Status)
				appendLog(state.Status)
			}
			if state.Progress > 0 {
				progressBar.SetValue(int(state.Progress * 100))
			}
			if state.Err != nil {
				walk.MsgBox(mw, "Installation Failed", state.Err.Error(), walk.MsgBoxIconError|walk.MsgBoxOK)
				os.Exit(1)
			}
			if state.Done {
				progressBar.SetValue(100)
				walk.MsgBox(mw, "Installation Complete",
					"TF2 Vintage has been installed successfully.\n\nA desktop shortcut has been created — double-click it to launch TF2 Vintage.",
					walk.MsgBoxIconInformation|walk.MsgBoxOK)
				os.Exit(0)
			}
		})
	}

	// askAltPath always returns the default location — no prompt shown.
	askAltPath := func() string {
		return ""
	}

	go install(report, askAltPath)
	mw.Run()
}
