//go:build windows

package main

import (
	"golang.org/x/sys/windows"
	"unsafe"
)

// createJunctionWindows creates a directory junction using the Windows API.
// Junctions (unlike symlinks) do not require administrator privileges.
func createJunctionWindows(linkPath, targetPath string) error {
	// Junctions require the target as a native NT path
	// e.g. F:\Games\tf2vintage → \??\F:\Games\tf2vintage
	ntTarget := `\??\` + targetPath

	// Create the link directory first
	if err := windows.CreateDirectory(
		windows.StringToUTF16Ptr(linkPath), nil); err != nil {
		return err
	}

	// Open the directory for reparse point manipulation
	handle, err := windows.CreateFile(
		windows.StringToUTF16Ptr(linkPath),
		windows.GENERIC_WRITE,
		0,
		nil,
		windows.OPEN_EXISTING,
		windows.FILE_FLAG_BACKUP_SEMANTICS|windows.FILE_FLAG_OPEN_REPARSE_POINT,
		0,
	)
	if err != nil {
		return err
	}
	defer windows.CloseHandle(handle)

	// Build the reparse data buffer
	type reparseBuffer struct {
		ReparseTag        uint32
		ReparseDataLength uint16
		Reserved          uint16
		SubstituteOffset  uint16
		SubstituteLength  uint16
		PrintOffset       uint16
		PrintLength       uint16
		PathBuffer        [1]uint16
	}

	targetUTF16 := windows.StringToUTF16(ntTarget)
	printUTF16 := windows.StringToUTF16(targetPath)

	targetLen := len(targetUTF16)*2 - 2 // bytes, no null terminator
	printLen := len(printUTF16)*2 - 2

	bufSize := 12 + targetLen + printLen
	buf := make([]byte, 8+bufSize)

	// IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003
	*(*uint32)(unsafe.Pointer(&buf[0])) = 0xA0000003
	*(*uint16)(unsafe.Pointer(&buf[4])) = uint16(bufSize)
	*(*uint16)(unsafe.Pointer(&buf[6])) = 0
	*(*uint16)(unsafe.Pointer(&buf[8])) = 0
	*(*uint16)(unsafe.Pointer(&buf[10])) = uint16(targetLen)
	*(*uint16)(unsafe.Pointer(&buf[12])) = uint16(targetLen + 2)
	*(*uint16)(unsafe.Pointer(&buf[14])) = uint16(printLen)

	targetUTF16 := windows.StringToUTF16(ntTarget)
	targetBytes := (*[1 << 20]byte)(unsafe.Pointer(&targetUTF16[0]))[:len(targetUTF16)*2]
	copy(buf[16:], targetBytes)

	pathUTF16 := windows.StringToUTF16(targetPath)
	pathBytes := (*[1 << 20]byte)(unsafe.Pointer(&pathUTF16[0]))[:len(pathUTF16)*2]
	copy(buf[16 + targetLen + 2:], pathBytes)
	
	copy(buf[16+targetLen+2:], windows.StringToUTF16(targetPath))

	var bytesReturned uint32
	return windows.DeviceIoControl(
		handle,
		0x000900A4, // FSCTL_SET_REPARSE_POINT
		&buf[0],
		uint32(len(buf)),
		nil,
		0,
		&bytesReturned,
		nil,
	)
}
