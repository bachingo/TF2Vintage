//go:build windows

package main

import (
	"fmt"
	"os"

	"github.com/lxn/walk"
	. "github.com/lxn/walk/declarative"
)

func startInstallUI(install func(func(InstallState), func() string, func() bool)) {
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
			if state.ManualLaunch != "" {
				msg := "The Steam launch option could not be set automatically.\n\n" +
					"Please set it manually:\n\n" +
					"1. In Steam, right-click Source SDK Base 2013 Multiplayer → Properties\n" +
					"2. Paste the following into Launch Options:\n\n" +
					state.ManualLaunch + "\n\n" +
					"This ensures TF2 Vintage updates automatically when you launch the game."
				walk.MsgBox(mw, "Manual Setup Required", msg, walk.MsgBoxIconWarning|walk.MsgBoxOK)
			}
			if state.Err != nil {
				walk.MsgBox(mw, "Installation Failed", state.Err.Error(), walk.MsgBoxIconError|walk.MsgBoxOK)
				os.Exit(1)
			}
			if state.Done {
				progressBar.SetValue(100)
				walk.MsgBox(mw, "Installation Complete",
					"TF2 Vintage has been installed successfully.\n\nSteam is restarting — TF2 Vintage will appear in your library shortly.",
					walk.MsgBoxIconInformation|walk.MsgBoxOK)
				os.Exit(0)
			}
		})
	}

	// askAltPath shows a dialog asking whether to install to a different drive.
	// Blocks the install goroutine via a channel until the user responds.
	askAltPath := func() string {
		ch := make(chan string, 1)

		mw.Synchronize(func() {
			result := walk.MsgBox(mw,
				"Install Location",
				"TF2 Vintage will be installed to your Steam Sourcemods folder by default.\n\n"+
					"Would you like to install the game files to a different drive instead?\n"+
					"(A junction will be created so Steam can still find it.)",
				walk.MsgBoxIconQuestion|walk.MsgBoxYesNo)

			if result == walk.DlgCmdNo {
				ch <- ""
				return
			}

			// Open a folder picker
			dlg := new(walk.FileDialog)
			dlg.Title = "Choose Install Location"
			dlg.FilePath = `C:\`

			accepted, err := dlg.ShowBrowseFolder(mw)
			if err != nil || !accepted {
				ch <- ""
				return
			}
			ch <- dlg.FilePath
		})

		return <-ch
	}

	askSymbols := func() bool {
			ch := make(chan bool, 1)
			mw.Synchronize(func() {
				result := walk.MsgBox(mw,
					"Debug Symbols",
					`Would you like to download debug symbols?

	Symbols let contributors read crash reports with full function names
	and source file locations instead of just memory addresses.

	Not needed for normal play. Adds ~50–200 MB per update.

	You can change this later by running:
	  tf2vintage-updater.exe --enable-symbols
	  tf2vintage-updater.exe --disable-symbols`,
					walk.MsgBoxIconQuestion|walk.MsgBoxYesNo)
				ch <- result == walk.DlgCmdYes
			})
			return <-ch
		}

	go install(report, askAltPath, askSymbols)
	mw.Run()
}
