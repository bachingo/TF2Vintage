//go:build windows

package main

import (
	"golang.org/x/sys/windows"
	"unsafe"
)

func availableDiskSpace(path string) (int64, error) {
	pathPtr, err := windows.UTF16PtrFromString(path)
	if err != nil {
		return 0, err
	}
	var free, total, totalFree uint64
	err = windows.GetDiskFreeSpaceEx(
		pathPtr,
		(*uint64)(unsafe.Pointer(&free)),
		(*uint64)(unsafe.Pointer(&total)),
		(*uint64)(unsafe.Pointer(&totalFree)),
	)
	return int64(free), err
}
