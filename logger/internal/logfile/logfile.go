package logfile

import (
	"bufio"
	"errors"
	"io"
	"os"
	"path/filepath"
	"time"

	"github.com/mxcd/bob-e-car/logger/internal/pb"
	"google.golang.org/protobuf/encoding/protodelim"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/reflect/protoreflect"
)

// telemetryFields enumerates the bob.Telemetry schema once — channel names
// are the proto JSON names (camelCase), matching the legacy JSON keys.
var telemetryFields = (&pb.Telemetry{}).ProtoReflect().Descriptor().Fields()

func fieldValue(message protoreflect.Message, fd protoreflect.FieldDescriptor) float64 {
	value := message.Get(fd)
	switch fd.Kind() {
	case protoreflect.BoolKind:
		if value.Bool() {
			return 1
		}
		return 0
	case protoreflect.FloatKind, protoreflect.DoubleKind:
		return value.Float()
	case protoreflect.Int32Kind, protoreflect.Int64Kind,
		protoreflect.Sint32Kind, protoreflect.Sint64Kind,
		protoreflect.Sfixed32Kind, protoreflect.Sfixed64Kind:
		return float64(value.Int())
	case protoreflect.Uint32Kind, protoreflect.Uint64Kind,
		protoreflect.Fixed32Kind, protoreflect.Fixed64Kind:
		return float64(value.Uint())
	}
	return 0
}

type Writer struct {
	file        *os.File
	buf         *bufio.Writer
	lastSync    time.Time
	RecordCount int64
	FirstUs     uint64
	LastUs      uint64
}

func NewWriter(path string) (*Writer, error) {
	file, err := os.OpenFile(path, os.O_CREATE|os.O_WRONLY|os.O_TRUNC, 0644)
	if err != nil {
		return nil, err
	}
	// fsync the directory so the file itself survives a power cut —
	// the car is switched off hard, there is no clean shutdown
	if dir, err := os.Open(filepath.Dir(path)); err == nil {
		dir.Sync()
		dir.Close()
	}
	return &Writer{file: file, buf: bufio.NewWriter(file), lastSync: time.Now()}, nil
}

func (w *Writer) Append(record *pb.LogRecord) error {
	if _, err := protodelim.MarshalTo(w.buf, record); err != nil {
		return err
	}
	if w.RecordCount == 0 {
		w.FirstUs = record.TimestampUs
	}
	w.LastUs = record.TimestampUs
	w.RecordCount++
	// ponytail: flush per record (5Hz) — batch flushing if the telemetry rate ever gets high
	if err := w.buf.Flush(); err != nil {
		return err
	}
	// fsync every second: a hard power cut loses at most the last second,
	// not the whole file sitting in the page cache
	if time.Since(w.lastSync) > time.Second {
		w.lastSync = time.Now()
		return w.file.Sync()
	}
	return nil
}

func (w *Writer) Size() int64 {
	info, err := w.file.Stat()
	if err != nil {
		return 0
	}
	return info.Size()
}

func (w *Writer) Close() error {
	if err := w.buf.Flush(); err != nil {
		w.file.Close()
		return err
	}
	if err := w.file.Sync(); err != nil {
		w.file.Close()
		return err
	}
	return w.file.Close()
}

// ScanResult describes the intact prefix of a log file.
type ScanResult struct {
	ValidBytes  int64
	RecordCount int64
	FirstUs     uint64
	LastUs      uint64
}

// Scan walks a log file and reports how many bytes form complete records —
// anything beyond ValidBytes is a partial record from an interrupted write.
func Scan(path string) (*ScanResult, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	reader := &countingReader{reader: bufio.NewReaderSize(file, 1<<20)}
	result := &ScanResult{}
	for {
		record := &pb.LogRecord{}
		if err := protodelim.UnmarshalFrom(reader, record); err != nil {
			// EOF = clean end, anything else = truncated/corrupt tail;
			// either way the intact prefix is what we keep
			return result, nil
		}
		result.ValidBytes = reader.count
		result.RecordCount++
		if result.RecordCount == 1 {
			result.FirstUs = record.TimestampUs
		}
		result.LastUs = record.TimestampUs
	}
}

type countingReader struct {
	reader *bufio.Reader
	count  int64
}

func (c *countingReader) Read(p []byte) (int, error) {
	n, err := c.reader.Read(p)
	c.count += int64(n)
	return n, err
}

func (c *countingReader) ReadByte() (byte, error) {
	b, err := c.reader.ReadByte()
	if err == nil {
		c.count++
	}
	return b, err
}

// Point is one decoded sample of a channel.
type Point struct {
	TimestampUs uint64
	Value       float64
}

// Data is a fully decoded log file, ready for resampling.
type Data struct {
	Channels map[string][]Point
	FlagsUs  []uint64
	FirstUs  uint64
	LastUs   uint64
}

// flagChannel is the steering wheel "R" button, repurposed as flag marker.
const flagChannel = "steeringAccelerationCommand"

// flagTimestamps reduces the flag button channel to its rising edges — a
// held button yields exactly one flag at the moment it was pressed. A file
// that starts mid-press counts that press too.
func flagTimestamps(points []Point) []uint64 {
	flags := []uint64{}
	previous := 0.0
	for _, point := range points {
		if point.Value >= 0.5 && previous < 0.5 {
			flags = append(flags, point.TimestampUs)
		}
		previous = point.Value
	}
	return flags
}

func Read(path string) (*Data, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	reader := bufio.NewReaderSize(file, 1<<20)
	data := &Data{Channels: map[string][]Point{}}
	for {
		record := &pb.LogRecord{}
		if err := protodelim.UnmarshalFrom(reader, record); err != nil {
			if errors.Is(err, io.EOF) {
				break
			}
			// truncated tail (e.g. power loss mid-write): keep what we have
			if errors.Is(err, io.ErrUnexpectedEOF) {
				break
			}
			return nil, err
		}
		if data.FirstUs == 0 {
			data.FirstUs = record.TimestampUs
		}
		data.LastUs = record.TimestampUs
		if len(record.Telemetry) > 0 {
			telemetry := &pb.Telemetry{}
			if err := proto.Unmarshal(record.Telemetry, telemetry); err != nil {
				continue
			}
			message := telemetry.ProtoReflect()
			for i := 0; i < telemetryFields.Len(); i++ {
				fd := telemetryFields.Get(i)
				name := fd.JSONName()
				data.Channels[name] = append(data.Channels[name], Point{record.TimestampUs, fieldValue(message, fd)})
			}
		} else {
			for key, value := range record.Values {
				data.Channels[key] = append(data.Channels[key], Point{record.TimestampUs, value})
			}
		}
	}
	data.FlagsUs = flagTimestamps(data.Channels[flagChannel])
	return data, nil
}
