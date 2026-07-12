package recorder

import (
	"log"
	"path/filepath"
	"sync"
	"time"

	"github.com/mxcd/bob-e-car/logger/internal/ingest"
	"github.com/mxcd/bob-e-car/logger/internal/logfile"
	"github.com/mxcd/bob-e-car/logger/internal/pb"
	"github.com/mxcd/bob-e-car/logger/internal/store"
)

// Recorder applies the trigger logic to the incoming record stream.
//
// Mode "always": one file per application run, opened on the first record.
// Mode "dms": a ring buffer holds the last n seconds. DMS pressed opens a
// file (pre-filled from the ring buffer); the file closes only after n
// seconds without a DMS press, so a brief accidental release never splits
// the log.
type Recorder struct {
	mu       sync.Mutex
	store    *store.Store
	logDir   string
	settings store.Settings

	ring          []*pb.LogRecord
	writer        *logfile.Writer
	logId         int64
	filename      string
	lastTriggerUs uint64
}

type Status struct {
	TriggerMode   string `json:"triggerMode"`
	BufferSeconds int    `json:"bufferSeconds"`
	Recording     bool   `json:"recording"`
	Filename      string `json:"filename,omitempty"`
	RecordCount   int64  `json:"recordCount"`
	SizeBytes     int64  `json:"sizeBytes"`
	StartedAtUs   int64  `json:"startedAtUs,omitempty"`
}

func New(st *store.Store, logDir string, settings store.Settings) *Recorder {
	return &Recorder{store: st, logDir: logDir, settings: settings}
}

func (r *Recorder) Run(frames <-chan *ingest.Frame) {
	for frame := range frames {
		r.mu.Lock()
		r.handle(frame)
		r.mu.Unlock()
	}
	r.mu.Lock()
	r.closeFile()
	r.mu.Unlock()
}

func (r *Recorder) handle(frame *ingest.Frame) {
	record := frame.Record
	if r.settings.TriggerMode == "always" {
		if r.writer == nil {
			r.openFile(record.TimestampUs, nil)
		}
		r.append(record)
		return
	}

	// dms mode — a flag press (steering "R" button) triggers like DMS, so a
	// flagged moment always lands in a file, pre-roll included
	trigger := frame.Dms || frame.Flag
	if trigger {
		r.lastTriggerUs = record.TimestampUs
	}
	bufferUs := uint64(r.settings.BufferSeconds) * 1e6

	if r.writer == nil {
		r.ring = append(r.ring, record)
		cutoff := record.TimestampUs - min(bufferUs, record.TimestampUs)
		for len(r.ring) > 0 && r.ring[0].TimestampUs < cutoff {
			r.ring = r.ring[1:]
		}
		if trigger {
			r.openFile(r.ring[0].TimestampUs, r.ring)
			r.ring = nil
		}
		return
	}

	r.append(record)
	if !trigger && record.TimestampUs-r.lastTriggerUs > bufferUs {
		r.closeFile()
	}
}

func (r *Recorder) openFile(startedAtUs uint64, backlog []*pb.LogRecord) {
	filename := time.UnixMicro(int64(startedAtUs)).Format("2006-01-02_15-04-05") + ".bblog"
	writer, err := logfile.NewWriter(filepath.Join(r.logDir, filename))
	if err != nil {
		log.Println("failed to open log file:", err)
		return
	}
	logId, err := r.store.InsertLog(filename, r.settings.TriggerMode, int64(startedAtUs))
	if err != nil {
		log.Println("failed to insert log metadata:", err)
		writer.Close()
		return
	}
	r.writer, r.logId, r.filename = writer, logId, filename
	log.Println("recording started:", filename)
	for _, record := range backlog {
		r.append(record)
	}
}

func (r *Recorder) append(record *pb.LogRecord) {
	if r.writer == nil {
		return
	}
	if err := r.writer.Append(record); err != nil {
		log.Println("failed to write record:", err)
	}
}

func (r *Recorder) closeFile() {
	if r.writer == nil {
		return
	}
	size := r.writer.Size()
	if err := r.writer.Close(); err != nil {
		log.Println("failed to close log file:", err)
	}
	err := r.store.FinalizeLog(r.logId, int64(r.writer.LastUs), size, r.writer.RecordCount)
	if err != nil {
		log.Println("failed to finalize log metadata:", err)
	}
	log.Printf("recording stopped: %s (%d records, %d bytes)", r.filename, r.writer.RecordCount, size)
	r.writer, r.logId, r.filename = nil, 0, ""
}

// SetSettings closes any active recording and applies the new trigger config.
func (r *Recorder) SetSettings(settings store.Settings) {
	r.mu.Lock()
	defer r.mu.Unlock()
	r.closeFile()
	r.ring = nil
	r.settings = settings
}

func (r *Recorder) Status() Status {
	r.mu.Lock()
	defer r.mu.Unlock()
	status := Status{
		TriggerMode:   r.settings.TriggerMode,
		BufferSeconds: r.settings.BufferSeconds,
		Recording:     r.writer != nil,
	}
	if r.writer != nil {
		status.Filename = r.filename
		status.RecordCount = r.writer.RecordCount
		status.SizeBytes = r.writer.Size()
		status.StartedAtUs = int64(r.writer.FirstUs)
	}
	return status
}
