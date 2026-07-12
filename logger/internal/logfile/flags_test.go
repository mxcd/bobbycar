package logfile

import (
	"slices"
	"testing"
)

func TestFlagTimestamps(t *testing.T) {
	// starts mid-press (100 counts), released at 300, second press held
	// 400-500 (dedups to one flag at 400)
	points := []Point{{100, 1}, {200, 1}, {300, 0}, {400, 1}, {500, 1}, {600, 0}}
	flags := flagTimestamps(points)
	if !slices.Equal(flags, []uint64{100, 400}) {
		t.Errorf("expected flags [100 400], got %v", flags)
	}
	if len(flagTimestamps(nil)) != 0 {
		t.Error("no points must yield no flags")
	}
}
