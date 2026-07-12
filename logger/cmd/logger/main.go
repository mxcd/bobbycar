package main

import (
	"context"
	"flag"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"github.com/mxcd/bob-e-car/logger/internal/api"
	"github.com/mxcd/bob-e-car/logger/internal/auth"
	"github.com/mxcd/bob-e-car/logger/internal/ingest"
	"github.com/mxcd/bob-e-car/logger/internal/live"
	"github.com/mxcd/bob-e-car/logger/internal/recorder"
	"github.com/mxcd/bob-e-car/logger/internal/store"
	"github.com/mxcd/go-config/config"
)

func envOr(key, fallback string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return fallback
}

func main() {
	// loads .env into the process env first, so the flag env fallbacks below
	// pick up .env values too
	if err := config.LoadConfig([]config.Value{
		config.String("LOGGER_USERNAME").Default(""),
		config.String("LOGGER_PASSWORD").Default("").Sensitive(),
	}); err != nil {
		log.Fatalln("failed to load config:", err)
	}

	logDir := flag.String("log-dir", envOr("LOGGER_LOG_DIR", "./logs"), "directory for log files")
	dbPath := flag.String("db", envOr("LOGGER_DB", "./logs.sqlite"), "sqlite metadata database path")
	udpAddr := flag.String("udp", envOr("LOGGER_UDP", ":4242"), "UDP listen address for telemetry")
	httpAddr := flag.String("http", envOr("LOGGER_HTTP", ":8090"), "HTTP listen address for dashboard")
	flag.Parse()

	if err := os.MkdirAll(*logDir, 0755); err != nil {
		log.Fatalln("failed to create log dir:", err)
	}

	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()

	st, err := store.New(*dbPath)
	if err != nil {
		log.Fatalln("failed to open sqlite store:", err)
	}
	defer st.Close()

	settings, err := st.GetSettings()
	if err != nil {
		log.Fatalln("failed to load settings:", err)
	}

	recorder.Recover(st, *logDir)

	hub := live.NewHub()
	in, err := ingest.Listen(ctx, *udpAddr, hub.Send)
	if err != nil {
		log.Fatalln("failed to listen on UDP:", err)
	}

	rec := recorder.New(st, *logDir, *settings)
	recorderDone := make(chan struct{})
	go func() {
		rec.Run(in.Records)
		close(recorderDone)
	}()

	authenticator := auth.New(config.Get().String("LOGGER_USERNAME"), config.Get().String("LOGGER_PASSWORD"), st)
	if !authenticator.Enabled() {
		log.Println("WARNING: dashboard auth disabled — set LOGGER_USERNAME and LOGGER_PASSWORD")
	}

	server := &http.Server{Addr: *httpAddr, Handler: api.New(st, in, rec, hub, authenticator, *logDir)}
	go func() {
		<-ctx.Done()
		server.Shutdown(context.Background())
	}()

	log.Printf("logger listening: udp=%s http=%s log-dir=%s mode=%s buffer=%ds",
		*udpAddr, *httpAddr, *logDir, settings.TriggerMode, settings.BufferSeconds)
	if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
		log.Fatalln(err)
	}
	<-recorderDone
	log.Println("shutdown complete")
}
