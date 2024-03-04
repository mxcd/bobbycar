package server

import (
	"github.com/gin-gonic/gin"
	"github.com/mxcd/bobbycar/internal/web"
)

type Server struct {
	Engine           *gin.Engine
	WebsocketManager *WebsocketManager
}

func InitServer() (*Server, error) {
	server := &Server{
		Engine: gin.Default(),
	}

	server.registerRoutes()

	return server, nil
}

func (s *Server) registerRoutes() {
	s.registerWebsocketHandler()
	s.Engine.Use(web.GetHandleFunc())
}
