package server

import (
	"sync"

	"github.com/rs/zerolog/log"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{}

type WebsocketManager struct {
	connections map[*websocket.Conn]bool
	lock        sync.Mutex
}

func (s *Server) registerWebsocketHandler() {

	websocketManager := &WebsocketManager{
		connections: make(map[*websocket.Conn]bool),
		lock:        sync.Mutex{},
	}
	s.WebsocketManager = websocketManager

	s.Engine.GET("/ws", func(c *gin.Context) {
		websocketConnection, err := upgrader.Upgrade(c.Writer, c.Request, nil)
		if err != nil {
			log.Error().Err(err).Msg("error upgrading websocket connection")
			return
		}
		websocketManager.addConnection(websocketConnection)
	})
}

func (w *WebsocketManager) addConnection(conn *websocket.Conn) {
	log.Info().Msg("adding websocket connection")
	w.lock.Lock()
	w.connections[conn] = true
	w.lock.Unlock()
}

func (w *WebsocketManager) removeConnection(conn *websocket.Conn) {
	w.lock.Lock()
	log.Info().Msg("removing websocket connection")
	err := conn.Close()
	if err != nil {
		log.Error().Err(err).Msg("error closing websocket connection")
	}
	delete(w.connections, conn)
	w.lock.Unlock()
}

func (w *WebsocketManager) Broadcast(message []byte) {
	w.lock.Lock()
	for conn := range w.connections {
		err := conn.WriteMessage(websocket.TextMessage, message)
		if err != nil {
			log.Error().Err(err).Msg("error broadcasting message to websocket connection")
			w.removeConnection(conn)
		}
	}
	w.lock.Unlock()
}
