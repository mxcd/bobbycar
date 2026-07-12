package api

import (
	"embed"
	"encoding/json"
	"errors"
	"io"
	"io/fs"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/mxcd/bob-e-car/logger/internal/auth"
	"github.com/mxcd/bob-e-car/logger/internal/ingest"
	"github.com/mxcd/bob-e-car/logger/internal/live"
	"github.com/mxcd/bob-e-car/logger/internal/logfile"
	"github.com/mxcd/bob-e-car/logger/internal/ota"
	"github.com/mxcd/bob-e-car/logger/internal/recorder"
	"github.com/mxcd/bob-e-car/logger/internal/store"
)

//go:embed ui
var uiFiles embed.FS

type Api struct {
	store       *store.Store
	ingest      *ingest.Ingest
	recorder    *recorder.Recorder
	flasher     *ota.Flasher
	logDir      string
	authEnabled bool

	cacheMu sync.Mutex
	cache   struct {
		id   int64
		size int64
		data *logfile.Data
	}
}

func New(st *store.Store, in *ingest.Ingest, rec *recorder.Recorder, hub *live.Hub, authenticator *auth.Auth, logDir string) http.Handler {
	api := &Api{store: st, ingest: in, recorder: rec, flasher: &ota.Flasher{}, logDir: logDir, authEnabled: authenticator.Enabled()}

	mux := http.NewServeMux()
	mux.HandleFunc("POST /api/login", authenticator.HandleLogin)
	mux.HandleFunc("POST /api/logout", authenticator.HandleLogout)
	mux.HandleFunc("GET /api/status", api.getStatus)
	mux.HandleFunc("GET /api/settings", api.getSettings)
	mux.HandleFunc("PUT /api/settings", api.putSettings)
	mux.HandleFunc("GET /api/logs", api.getLogs)
	mux.HandleFunc("DELETE /api/logs", api.deleteAllLogs)
	mux.HandleFunc("GET /api/logs/{id}/data", api.getLogData)
	mux.HandleFunc("GET /api/logs/{id}/download", api.downloadLog)
	mux.HandleFunc("PATCH /api/logs/{id}", api.renameLog)
	mux.HandleFunc("DELETE /api/logs/{id}", api.deleteLog)
	mux.HandleFunc("GET /api/ota/probe", api.otaProbe)
	mux.HandleFunc("POST /api/ota/flash", api.otaFlash)
	mux.Handle("GET /ws", hub)

	ui, _ := fs.Sub(uiFiles, "ui")
	mux.Handle("/", http.FileServerFS(ui))
	return authenticator.Middleware(mux)
}

func writeJson(w http.ResponseWriter, value any) {
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(value)
}

func (a *Api) getStatus(w http.ResponseWriter, r *http.Request) {
	stats := a.ingest.Stats()
	lastPacketAgeMs := int64(-1)
	if stats.LastPacketUs > 0 {
		lastPacketAgeMs = (time.Now().UnixMicro() - stats.LastPacketUs) / 1000
	}
	writeJson(w, map[string]any{
		"packetCount":     stats.PacketCount,
		"lastPacketAgeMs": lastPacketAgeMs,
		"receiving":       lastPacketAgeMs >= 0 && lastPacketAgeMs < 1000,
		"vehicleState":    stats.VehicleState,
		"batteryVoltage":  stats.BatteryVoltage,
		"authEnabled":     a.authEnabled,
		"recorder":        a.recorder.Status(),
	})
}

func (a *Api) otaProbe(w http.ResponseWriter, r *http.Request) {
	output, err := a.flasher.Probe()
	writeOtaResult(w, output, err)
}

const maxFirmwareBytes = 2 << 20 // STM32F767ZI flash size

func (a *Api) otaFlash(w http.ResponseWriter, r *http.Request) {
	stats := a.ingest.Stats()
	receiving := stats.LastPacketUs > 0 && time.Now().UnixMicro()-stats.LastPacketUs < 1_000_000
	if receiving && stats.VehicleState != 0 && r.URL.Query().Get("force") != "true" {
		http.Error(w, "vehicle is not in IDLE — add ?force=true to flash anyway", http.StatusConflict)
		return
	}
	firmware, err := io.ReadAll(io.LimitReader(r.Body, maxFirmwareBytes+1))
	if err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	if len(firmware) < 1024 || len(firmware) > maxFirmwareBytes {
		http.Error(w, "firmware must be a raw .bin between 1kB and 2MB", http.StatusBadRequest)
		return
	}
	output, err := a.flasher.Flash(firmware)
	writeOtaResult(w, output, err)
}

func writeOtaResult(w http.ResponseWriter, output string, err error) {
	errorText := ""
	if err != nil {
		errorText = err.Error()
		if errors.Is(err, ota.ErrBusy) {
			w.WriteHeader(http.StatusConflict)
		} else {
			w.WriteHeader(http.StatusInternalServerError)
		}
	}
	writeJson(w, map[string]any{"ok": err == nil, "output": output, "error": errorText})
}

func (a *Api) getSettings(w http.ResponseWriter, r *http.Request) {
	settings, err := a.store.GetSettings()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	writeJson(w, settings)
}

func (a *Api) putSettings(w http.ResponseWriter, r *http.Request) {
	var settings store.Settings
	if err := json.NewDecoder(r.Body).Decode(&settings); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	if settings.TriggerMode != "always" && settings.TriggerMode != "dms" {
		http.Error(w, "triggerMode must be 'always' or 'dms'", http.StatusBadRequest)
		return
	}
	if settings.BufferSeconds < 1 || settings.BufferSeconds > 600 {
		http.Error(w, "bufferSeconds must be 1-600", http.StatusBadRequest)
		return
	}
	if err := a.store.SaveSettings(&settings); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	a.recorder.SetSettings(settings)
	writeJson(w, settings)
}

func (a *Api) getLogs(w http.ResponseWriter, r *http.Request) {
	logs, err := a.store.ListLogs()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	writeJson(w, logs)
}

func (a *Api) logPath(r *http.Request) (string, int64, error) {
	id, err := strconv.ParseInt(r.PathValue("id"), 10, 64)
	if err != nil {
		return "", 0, err
	}
	meta, err := a.store.GetLog(id)
	if err != nil {
		return "", 0, err
	}
	return filepath.Join(a.logDir, meta.Filename), id, nil
}

func (a *Api) getLogData(w http.ResponseWriter, r *http.Request) {
	path, id, err := a.logPath(r)
	if err != nil {
		http.Error(w, "log not found", http.StatusNotFound)
		return
	}
	data, err := a.loadCached(id, path)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	query := r.URL.Query()
	fromUs, _ := strconv.ParseUint(query.Get("from"), 10, 64)
	toUs, _ := strconv.ParseUint(query.Get("to"), 10, 64)
	points, _ := strconv.Atoi(query.Get("points"))
	if points <= 0 || points > 100000 {
		points = 2000
	}
	var channels []string
	if raw := query.Get("channels"); raw != "" {
		channels = strings.Split(raw, ",")
	}
	writeJson(w, logfile.Resample(data, fromUs, toUs, points, channels))
}

func (a *Api) downloadLog(w http.ResponseWriter, r *http.Request) {
	path, _, err := a.logPath(r)
	if err != nil {
		http.Error(w, "log not found", http.StatusNotFound)
		return
	}
	w.Header().Set("Content-Disposition", "attachment; filename=\""+filepath.Base(path)+"\"")
	http.ServeFile(w, r, path)
}

func (a *Api) renameLog(w http.ResponseWriter, r *http.Request) {
	id, err := strconv.ParseInt(r.PathValue("id"), 10, 64)
	if err != nil {
		http.Error(w, "invalid id", http.StatusBadRequest)
		return
	}
	meta, err := a.store.GetLog(id)
	if err != nil {
		http.Error(w, "log not found", http.StatusNotFound)
		return
	}
	if meta.EndedAtUs == nil {
		http.Error(w, "log is currently recording", http.StatusConflict)
		return
	}
	var body struct {
		Filename string `json:"filename"`
	}
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}
	filename := strings.TrimSpace(body.Filename)
	if !strings.HasSuffix(filename, ".bblog") {
		filename += ".bblog"
	}
	if filename == ".bblog" || filename != filepath.Base(filename) || strings.HasPrefix(filename, ".") {
		http.Error(w, "invalid filename", http.StatusBadRequest)
		return
	}
	if filename == meta.Filename {
		writeJson(w, meta)
		return
	}
	newPath := filepath.Join(a.logDir, filename)
	if _, err := os.Stat(newPath); err == nil {
		http.Error(w, "a log with that filename already exists", http.StatusConflict)
		return
	}
	if err := os.Rename(filepath.Join(a.logDir, meta.Filename), newPath); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	if err := a.store.RenameLog(id, filename); err != nil {
		os.Rename(newPath, filepath.Join(a.logDir, meta.Filename)) // keep file and db in sync
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	meta.Filename = filename
	writeJson(w, meta)
}

func (a *Api) deleteLog(w http.ResponseWriter, r *http.Request) {
	id, err := strconv.ParseInt(r.PathValue("id"), 10, 64)
	if err != nil {
		http.Error(w, "invalid id", http.StatusBadRequest)
		return
	}
	meta, err := a.store.GetLog(id)
	if err != nil {
		http.Error(w, "log not found", http.StatusNotFound)
		return
	}
	if meta.EndedAtUs == nil {
		http.Error(w, "log is currently recording", http.StatusConflict)
		return
	}
	if err := a.removeLog(meta); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.WriteHeader(http.StatusNoContent)
}

func (a *Api) deleteAllLogs(w http.ResponseWriter, r *http.Request) {
	logs, err := a.store.ListLogs()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	deleted := 0
	for _, meta := range logs {
		if meta.EndedAtUs == nil {
			continue // never touch an active recording
		}
		if err := a.removeLog(&meta); err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		deleted++
	}
	writeJson(w, map[string]int{"deleted": deleted})
}

func (a *Api) removeLog(meta *store.LogMeta) error {
	if err := a.store.DeleteLog(meta.Id); err != nil {
		return err
	}
	if err := os.Remove(filepath.Join(a.logDir, meta.Filename)); err != nil && !os.IsNotExist(err) {
		return err
	}
	a.cacheMu.Lock()
	if a.cache.id == meta.Id {
		a.cache.data = nil
	}
	a.cacheMu.Unlock()
	return nil
}

// loadCached keeps the last decoded log in memory; an active log that grew
// on disk is re-decoded on the next request.
func (a *Api) loadCached(id int64, path string) (*logfile.Data, error) {
	info, err := os.Stat(path)
	if err != nil {
		return nil, err
	}
	a.cacheMu.Lock()
	defer a.cacheMu.Unlock()
	if a.cache.id == id && a.cache.size == info.Size() && a.cache.data != nil {
		return a.cache.data, nil
	}
	data, err := logfile.Read(path)
	if err != nil {
		return nil, err
	}
	a.cache.id, a.cache.size, a.cache.data = id, info.Size(), data
	return data, nil
}
