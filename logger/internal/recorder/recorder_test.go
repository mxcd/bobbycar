package recorder

import (
	"os"
	"path/filepath"
	"testing"

	"github.com/mxcd/bob-e-car/logger/internal/ingest"
	"github.com/mxcd/bob-e-car/logger/internal/logfile"
	"github.com/mxcd/bob-e-car/logger/internal/pb"
	"github.com/mxcd/bob-e-car/logger/internal/store"
	"google.golang.org/protobuf/proto"
)

const dmsKey = "steeringDeadManSwitchPressed"

func record(tSec float64, dms bool) *ingest.Frame {
	raw, _ := proto.Marshal(&pb.Telemetry{
		VdBatteryVoltage:             48,
		SteeringDeadManSwitchPressed: dms,
	})
	return &ingest.Frame{
		Record: &pb.LogRecord{TimestampUs: uint64(tSec * 1e6), Telemetry: raw},
		Dms:    dms,
	}
}

func TestDmsTriggerWithPrerollAndBlip(t *testing.T) {
	dir := t.TempDir()
	st, err := store.New(filepath.Join(dir, "test.sqlite"))
	if err != nil {
		t.Fatal(err)
	}
	defer st.Close()

	rec := New(st, dir, store.Settings{TriggerMode: "dms", BufferSeconds: 10})
	records := make(chan *ingest.Frame, 1024)

	// timeline at 5Hz: 0-30s released (ring keeps only last 10s),
	// 30-40s pressed, 40-41s blip release, 41-50s pressed,
	// 50-70s released (post-roll closes at 60s), 70s+ still released
	for tick := 0; tick <= 350; tick++ {
		tSec := float64(tick) * 0.2
		dms := (tSec >= 30 && tSec < 40) || (tSec >= 41 && tSec < 50)
		records <- record(tSec, dms)
	}
	close(records)
	rec.Run(records)

	logs, err := st.ListLogs()
	if err != nil {
		t.Fatal(err)
	}
	if len(logs) != 1 {
		t.Fatalf("expected exactly 1 log (blip must not split it), got %d", len(logs))
	}
	log := logs[0]
	if log.EndedAtUs == nil {
		t.Fatal("log not finalized")
	}

	// pre-roll: file starts ~20s (10s before DMS at 30s)
	if log.StartedAtUs > 21*1e6 || log.StartedAtUs < 19*1e6 {
		t.Errorf("expected start ~20s, got %.1fs", float64(log.StartedAtUs)/1e6)
	}
	// post-roll: file ends ~60s (10s after last DMS at 50s)
	if *log.EndedAtUs > 61*1e6 || *log.EndedAtUs < 59*1e6 {
		t.Errorf("expected end ~60s, got %.1fs", float64(*log.EndedAtUs)/1e6)
	}

	data, err := logfile.Read(filepath.Join(dir, log.Filename))
	if err != nil {
		t.Fatal(err)
	}
	points := data.Channels[dmsKey]
	if int64(len(points)) != log.RecordCount {
		t.Errorf("file has %d records, metadata says %d", len(points), log.RecordCount)
	}
	// every tick in [20s, 60s] present — nothing resampled away
	expected := int(40/0.2) + 1
	if len(points) != expected {
		t.Errorf("expected %d records, got %d", expected, len(points))
	}
}

func flagRecord(tSec float64, flag bool) *ingest.Frame {
	raw, _ := proto.Marshal(&pb.Telemetry{SteeringAccelerationCommand: flag})
	return &ingest.Frame{
		Record: &pb.LogRecord{TimestampUs: uint64(tSec * 1e6), Telemetry: raw},
		Flag:   flag,
	}
}

func TestFlagTriggersRecording(t *testing.T) {
	dir := t.TempDir()
	st, err := store.New(filepath.Join(dir, "test.sqlite"))
	if err != nil {
		t.Fatal(err)
	}
	defer st.Close()

	rec := New(st, dir, store.Settings{TriggerMode: "dms", BufferSeconds: 10})
	records := make(chan *ingest.Frame, 1024)

	// 5Hz, DMS never pressed: flag held 20-21s must open a file with
	// pre-roll (~10s) and close after the post-roll (~31s)
	for tick := 0; tick <= 200; tick++ {
		tSec := float64(tick) * 0.2
		records <- flagRecord(tSec, tSec >= 20 && tSec < 21)
	}
	close(records)
	rec.Run(records)

	logs, err := st.ListLogs()
	if err != nil {
		t.Fatal(err)
	}
	if len(logs) != 1 {
		t.Fatalf("expected flag press to open exactly 1 log, got %d", len(logs))
	}
	log := logs[0]
	if log.StartedAtUs > 11*1e6 || log.StartedAtUs < 9*1e6 {
		t.Errorf("expected pre-roll start ~10s, got %.1fs", float64(log.StartedAtUs)/1e6)
	}
	if log.EndedAtUs == nil || *log.EndedAtUs > 32*1e6 || *log.EndedAtUs < 30*1e6 {
		t.Errorf("expected post-roll end ~31s, got %+v", log.EndedAtUs)
	}

	// the 1s hold (5 samples) dedups to exactly one flag at the press
	data, err := logfile.Read(filepath.Join(dir, log.Filename))
	if err != nil {
		t.Fatal(err)
	}
	if len(data.FlagsUs) != 1 || data.FlagsUs[0] != 20*1e6 {
		t.Errorf("expected one flag at 20s, got %v", data.FlagsUs)
	}
}

func TestRecover(t *testing.T) {
	dir := t.TempDir()
	st, err := store.New(filepath.Join(dir, "test.sqlite"))
	if err != nil {
		t.Fatal(err)
	}
	defer st.Close()

	// interrupted recording: unfinalized row, file with 10 records + torn tail
	writer, err := logfile.NewWriter(filepath.Join(dir, "torn.bblog"))
	if err != nil {
		t.Fatal(err)
	}
	for tick := 0; tick < 10; tick++ {
		writer.Append(record(float64(tick)*0.2, true).Record)
	}
	writer.Close()
	tornId, _ := st.InsertLog("torn.bblog", "dms", 0)
	file, _ := os.OpenFile(filepath.Join(dir, "torn.bblog"), os.O_APPEND|os.O_WRONLY, 0644)
	file.Write([]byte{0x55, 0x03, 0x01}) // partial record: length prefix, missing payload
	file.Close()

	// row whose file disappeared
	goneId, _ := st.InsertLog("gone.bblog", "dms", 0)

	// power cut before any record hit the disk: empty file, unfinalized row
	os.WriteFile(filepath.Join(dir, "empty.bblog"), nil, 0644)
	emptyId, _ := st.InsertLog("empty.bblog", "dms", 0)

	Recover(st, dir)

	torn, err := st.GetLog(tornId)
	if err != nil {
		t.Fatal(err)
	}
	if torn.EndedAtUs == nil || torn.RecordCount != 10 {
		t.Errorf("torn log not finalized correctly: %+v", torn)
	}
	info, _ := os.Stat(filepath.Join(dir, "torn.bblog"))
	if info.Size() != torn.SizeBytes {
		t.Errorf("torn tail not truncated: file=%d meta=%d", info.Size(), torn.SizeBytes)
	}
	data, err := logfile.Read(filepath.Join(dir, "torn.bblog"))
	if err != nil || len(data.Channels[dmsKey]) != 10 {
		t.Errorf("recovered file not cleanly readable: %v", err)
	}
	if _, err := st.GetLog(goneId); err == nil {
		t.Error("row with missing file should be deleted")
	}
	if _, err := st.GetLog(emptyId); err == nil {
		t.Error("row with empty file should be deleted")
	}
	if _, err := os.Stat(filepath.Join(dir, "empty.bblog")); err == nil {
		t.Error("empty file should be deleted")
	}
}

func TestAlwaysMode(t *testing.T) {
	dir := t.TempDir()
	st, err := store.New(filepath.Join(dir, "test.sqlite"))
	if err != nil {
		t.Fatal(err)
	}
	defer st.Close()

	rec := New(st, dir, store.Settings{TriggerMode: "always", BufferSeconds: 10})
	records := make(chan *ingest.Frame, 64)
	for tick := 0; tick < 50; tick++ {
		records <- record(float64(tick)*0.2, false)
	}
	close(records)
	rec.Run(records)

	logs, _ := st.ListLogs()
	if len(logs) != 1 || logs[0].RecordCount != 50 {
		t.Fatalf("expected 1 log with all 50 records, got %+v", logs)
	}
}
