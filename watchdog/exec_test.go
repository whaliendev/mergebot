package main

import (
	"runtime"
	"testing"
)

func TestCheckBinaryExists(t *testing.T) {
	if runtime.GOOS != "linux" && runtime.GOOS != "darwin" {
		return
	}

	exists, _, _ := CheckBinaryExists("ls")
	if !exists {
		t.Errorf("expected true, got %t", exists)
	}
}
