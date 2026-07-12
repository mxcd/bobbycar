package ota

import (
	"context"
	"errors"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"sync"
	"time"
)

// STM32 flash base — where the Nucleo F767ZI boots from.
const flashAddress = "0x8000000"

var ErrBusy = errors.New("an st-link operation is already in progress")

// Flasher drives the system controller's onboard ST-LINK (Nucleo USB
// connected to the Pi) via stlink-tools.
type Flasher struct {
	mu sync.Mutex
}

func run(timeout time.Duration, name string, args ...string) (string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()
	output, err := exec.CommandContext(ctx, name, args...).CombinedOutput()
	return string(output), err
}

// Probe reports whether the system controller's ST-LINK is visible on USB.
// It deliberately reads sysfs only and never runs st-info: an SWD attach
// halts/resets the running controller — only Flash may ever touch the target.
func (f *Flasher) Probe() (string, error) {
	vendorFiles, _ := filepath.Glob("/sys/bus/usb/devices/*/idVendor")
	for _, vendorFile := range vendorFiles {
		vendor, _ := os.ReadFile(vendorFile)
		if strings.TrimSpace(string(vendor)) != "0483" { // STMicroelectronics
			continue
		}
		product, _ := os.ReadFile(filepath.Join(filepath.Dir(vendorFile), "product"))
		name := strings.TrimSpace(string(product))
		// iProduct varies by firmware: "ST-LINK/V2.1", "STM32 STLink", "STLINK-V3"
		if strings.Contains(strings.ReplaceAll(strings.ToUpper(name), "-", ""), "STLINK") {
			return "found: " + name, nil
		}
	}
	return "no ST-LINK on USB", nil
}

// Flash writes the firmware image to the STM32 and resets it.
func (f *Flasher) Flash(firmware []byte) (string, error) {
	if !f.mu.TryLock() {
		return "", ErrBusy
	}
	defer f.mu.Unlock()
	path := filepath.Join(os.TempDir(), "bob-firmware.bin")
	if err := os.WriteFile(path, firmware, 0644); err != nil {
		return "", err
	}
	return run(2*time.Minute, "st-flash", "--reset", "write", path, flashAddress)
}
