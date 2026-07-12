package store

import (
	"database/sql"
	"strconv"

	_ "modernc.org/sqlite"
)

type Store struct {
	db *sql.DB
}

type LogMeta struct {
	Id          int64  `json:"id"`
	Filename    string `json:"filename"`
	TriggerMode string `json:"triggerMode"`
	StartedAtUs int64  `json:"startedAtUs"`
	EndedAtUs   *int64 `json:"endedAtUs"`
	SizeBytes   int64  `json:"sizeBytes"`
	RecordCount int64  `json:"recordCount"`
}

type Settings struct {
	TriggerMode   string `json:"triggerMode"` // "always" | "dms"
	BufferSeconds int    `json:"bufferSeconds"`
}

func New(path string) (*Store, error) {
	db, err := sql.Open("sqlite", path)
	if err != nil {
		return nil, err
	}
	_, err = db.Exec(`
		CREATE TABLE IF NOT EXISTS logs (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			filename TEXT NOT NULL UNIQUE,
			trigger_mode TEXT NOT NULL,
			started_at_us INTEGER NOT NULL,
			ended_at_us INTEGER,
			size_bytes INTEGER NOT NULL DEFAULT 0,
			record_count INTEGER NOT NULL DEFAULT 0
		);
		CREATE TABLE IF NOT EXISTS settings (
			key TEXT PRIMARY KEY,
			value TEXT NOT NULL
		);
		CREATE TABLE IF NOT EXISTS sessions (
			token TEXT PRIMARY KEY,
			created_at_us INTEGER NOT NULL
		);
	`)
	if err != nil {
		db.Close()
		return nil, err
	}
	return &Store{db: db}, nil
}

func (s *Store) Close() error {
	return s.db.Close()
}

func (s *Store) InsertLog(filename, triggerMode string, startedAtUs int64) (int64, error) {
	result, err := s.db.Exec(
		`INSERT INTO logs (filename, trigger_mode, started_at_us) VALUES (?, ?, ?)`,
		filename, triggerMode, startedAtUs)
	if err != nil {
		return 0, err
	}
	return result.LastInsertId()
}

func (s *Store) FinalizeLog(id, endedAtUs, sizeBytes, recordCount int64) error {
	_, err := s.db.Exec(
		`UPDATE logs SET ended_at_us = ?, size_bytes = ?, record_count = ? WHERE id = ?`,
		endedAtUs, sizeBytes, recordCount, id)
	return err
}

func (s *Store) ListLogs() ([]LogMeta, error) {
	rows, err := s.db.Query(
		`SELECT id, filename, trigger_mode, started_at_us, ended_at_us, size_bytes, record_count
		 FROM logs ORDER BY started_at_us DESC`)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	logs := []LogMeta{}
	for rows.Next() {
		var log LogMeta
		if err := rows.Scan(&log.Id, &log.Filename, &log.TriggerMode, &log.StartedAtUs,
			&log.EndedAtUs, &log.SizeBytes, &log.RecordCount); err != nil {
			return nil, err
		}
		logs = append(logs, log)
	}
	return logs, rows.Err()
}

func (s *Store) GetLog(id int64) (*LogMeta, error) {
	var log LogMeta
	err := s.db.QueryRow(
		`SELECT id, filename, trigger_mode, started_at_us, ended_at_us, size_bytes, record_count
		 FROM logs WHERE id = ?`, id).
		Scan(&log.Id, &log.Filename, &log.TriggerMode, &log.StartedAtUs,
			&log.EndedAtUs, &log.SizeBytes, &log.RecordCount)
	if err != nil {
		return nil, err
	}
	return &log, nil
}

func (s *Store) RenameLog(id int64, filename string) error {
	_, err := s.db.Exec(`UPDATE logs SET filename = ? WHERE id = ?`, filename, id)
	return err
}

func (s *Store) DeleteLog(id int64) error {
	_, err := s.db.Exec(`DELETE FROM logs WHERE id = ?`, id)
	return err
}

// ListSessions purges sessions older than maxAgeUs and returns the rest —
// sessions survive reboots (the Pi power-cycles with the car).
func (s *Store) ListSessions(nowUs, maxAgeUs int64) ([]string, error) {
	if _, err := s.db.Exec(`DELETE FROM sessions WHERE created_at_us < ?`, nowUs-maxAgeUs); err != nil {
		return nil, err
	}
	rows, err := s.db.Query(`SELECT token FROM sessions`)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	tokens := []string{}
	for rows.Next() {
		var token string
		if err := rows.Scan(&token); err != nil {
			return nil, err
		}
		tokens = append(tokens, token)
	}
	return tokens, rows.Err()
}

func (s *Store) InsertSession(token string, createdAtUs int64) error {
	_, err := s.db.Exec(`INSERT INTO sessions (token, created_at_us) VALUES (?, ?)`, token, createdAtUs)
	return err
}

func (s *Store) DeleteSession(token string) error {
	_, err := s.db.Exec(`DELETE FROM sessions WHERE token = ?`, token)
	return err
}

func (s *Store) GetSettings() (*Settings, error) {
	settings := &Settings{TriggerMode: "dms", BufferSeconds: 10}
	rows, err := s.db.Query(`SELECT key, value FROM settings`)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	for rows.Next() {
		var key, value string
		if err := rows.Scan(&key, &value); err != nil {
			return nil, err
		}
		switch key {
		case "triggerMode":
			settings.TriggerMode = value
		case "bufferSeconds":
			if n, err := strconv.Atoi(value); err == nil && n > 0 {
				settings.BufferSeconds = n
			}
		}
	}
	return settings, rows.Err()
}

func (s *Store) SaveSettings(settings *Settings) error {
	for key, value := range map[string]string{
		"triggerMode":   settings.TriggerMode,
		"bufferSeconds": strconv.Itoa(settings.BufferSeconds),
	} {
		if _, err := s.db.Exec(
			`INSERT INTO settings (key, value) VALUES (?, ?)
			 ON CONFLICT(key) DO UPDATE SET value = excluded.value`, key, value); err != nil {
			return err
		}
	}
	return nil
}
