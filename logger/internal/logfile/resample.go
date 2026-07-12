package logfile

// Fixed-grid resampler after the fsw-dl pattern: the point budget spread over
// the requested window determines the effective sample rate, each grid point
// averages the raw points within half a sample interval around it, and empty
// buckets fall back to linear interpolation. A moving pivot cursor keeps the
// whole pass O(n) per channel.

type ResampledChannel struct {
	StartUs      uint64    `json:"startUs"`
	SampleRateHz float64   `json:"sampleRateHz"`
	Values       []float64 `json:"values"`
}

type ResampleResult struct {
	FromUs   uint64                       `json:"fromUs"`
	ToUs     uint64                       `json:"toUs"`
	Channels map[string]*ResampledChannel `json:"channels"`
	// flag button presses (rising edges, deduplicated) — always included at
	// full resolution so no flag disappears through decimation
	FlagsUs []uint64 `json:"flagsUs"`
}

func Resample(data *Data, fromUs, toUs uint64, maxPoints int, channels []string) *ResampleResult {
	if fromUs < data.FirstUs || fromUs >= data.LastUs {
		fromUs = data.FirstUs
	}
	if toUs > data.LastUs || toUs <= fromUs {
		toUs = data.LastUs
	}
	if maxPoints < 2 {
		maxPoints = 2
	}

	result := &ResampleResult{FromUs: fromUs, ToUs: toUs, Channels: map[string]*ResampledChannel{}, FlagsUs: []uint64{}}
	for _, t := range data.FlagsUs {
		if t >= fromUs && t <= toUs {
			result.FlagsUs = append(result.FlagsUs, t)
		}
	}
	windowUs := toUs - fromUs
	if windowUs == 0 {
		return result
	}

	stepUs := windowUs / uint64(maxPoints)
	if stepUs == 0 {
		stepUs = 1
	}
	pointCount := int(windowUs/stepUs) + 1
	halfUs := stepUs / 2
	sampleRateHz := 1e6 / float64(stepUs)

	selected := data.Channels
	if len(channels) > 0 {
		selected = map[string][]Point{}
		for _, name := range channels {
			if points, ok := data.Channels[name]; ok {
				selected[name] = points
			}
		}
	}

	// ponytail: sequential per-channel loop — parallelize per channel if logs grow into the hours
	for name, points := range selected {
		if len(points) == 0 {
			continue
		}
		values := make([]float64, 0, pointCount)
		pivot := 0
		for i := 0; i < pointCount; i++ {
			t := fromUs + uint64(i)*stepUs
			lo := t - min(halfUs, t)
			hi := t + halfUs

			for pivot < len(points) && points[pivot].TimestampUs < lo {
				pivot++
			}
			sum, n := 0.0, 0
			for j := pivot; j < len(points) && points[j].TimestampUs <= hi; j++ {
				sum += points[j].Value
				n++
			}
			if n > 0 {
				values = append(values, sum/float64(n))
			} else {
				values = append(values, interpolate(points, pivot, t))
			}
		}
		result.Channels[name] = &ResampledChannel{StartUs: fromUs, SampleRateHz: sampleRateHz, Values: values}
	}
	return result
}

// interpolate returns the linear interpolation between the points surrounding
// t; pivot is the index of the first point with timestamp >= the bucket start.
func interpolate(points []Point, pivot int, t uint64) float64 {
	if pivot == 0 {
		return points[0].Value
	}
	if pivot >= len(points) {
		return points[len(points)-1].Value
	}
	prev, next := points[pivot-1], points[pivot]
	if next.TimestampUs == prev.TimestampUs {
		return prev.Value
	}
	ratio := float64(t-prev.TimestampUs) / float64(next.TimestampUs-prev.TimestampUs)
	return prev.Value + ratio*(next.Value-prev.Value)
}
