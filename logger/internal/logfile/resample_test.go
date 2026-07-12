package logfile

import (
	"math"
	"testing"
)

func TestResampleZoomIncreasesDetail(t *testing.T) {
	// 60s of 5Hz data, value = t in seconds
	points := make([]Point, 0, 301)
	for i := 0; i <= 300; i++ {
		us := uint64(i) * 200_000
		points = append(points, Point{us, float64(us) / 1e6})
	}
	data := &Data{Channels: map[string][]Point{"ch": points}, FirstUs: 0, LastUs: 300 * 200_000}

	full := Resample(data, 0, 0, 100, nil)
	channel := full.Channels["ch"]
	if len(channel.Values) < 90 || len(channel.Values) > 110 {
		t.Fatalf("expected ~100 points, got %d", len(channel.Values))
	}
	// averaged values must still follow the ramp
	mid := channel.Values[len(channel.Values)/2]
	if math.Abs(mid-30) > 2 {
		t.Errorf("mid value should be ~30s, got %f", mid)
	}

	// zooming into 10% of the window with the same budget → ~10x sample rate
	zoom := Resample(data, 0, 6_000_000, 100, nil)
	if zoom.Channels["ch"].SampleRateHz < full.Channels["ch"].SampleRateHz*5 {
		t.Errorf("zoom should raise effective rate: full=%f zoom=%f",
			full.Channels["ch"].SampleRateHz, zoom.Channels["ch"].SampleRateHz)
	}
}

func TestResampleChannelFilterAndInterpolation(t *testing.T) {
	data := &Data{
		Channels: map[string][]Point{
			"a": {{0, 0}, {10_000_000, 10}},
			"b": {{0, 5}, {10_000_000, 5}},
		},
		FirstUs: 0, LastUs: 10_000_000,
	}
	result := Resample(data, 0, 0, 10, []string{"a"})
	if len(result.Channels) != 1 || result.Channels["a"] == nil {
		t.Fatalf("expected only channel a, got %v", result.Channels)
	}
	// sparse channel: empty buckets between the two points must interpolate the ramp
	values := result.Channels["a"].Values
	mid := values[len(values)/2]
	if math.Abs(mid-5) > 1.5 {
		t.Errorf("expected interpolated ~5 at midpoint, got %f", mid)
	}
}
