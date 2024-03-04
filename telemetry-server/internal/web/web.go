package web

import (
	"embed"
	"io/fs"
	"net/http"

	"github.com/gin-gonic/gin"
	"github.com/rs/zerolog/log"
)

//go:embed html/**
var webRoot embed.FS

func GetHandleFunc() gin.HandlerFunc {
	sub, err := fs.Sub(webRoot, "html")
	if err != nil {
		log.Panic().Err(err).Msg("error getting subdirectory for webRoot")
	}
	return func(c *gin.Context) {
		http.FileServer(http.FS(sub)).ServeHTTP(c.Writer, c.Request)
	}
}

func CacheControlMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Cache-Control", "public, max-age=86400")
		next.ServeHTTP(w, r)
	})
}
