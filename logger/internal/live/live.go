package live

import (
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/gorilla/websocket"
	"github.com/mxcd/bob-e-car/logger/internal/pb"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
)

// EmitUnpopulated keeps the camelCase keys (incl. zero values) the live
// dashboard consumed from the legacy JSON firmware.
var jsonMarshaler = protojson.MarshalOptions{EmitUnpopulated: true}

// the dashboard is reached via Tailscale hostnames, not the listen address
var upgrader = websocket.Upgrader{CheckOrigin: func(*http.Request) bool { return true }}

// Hub broadcasts each received telemetry packet as JSON to the connected
// live-dashboard websockets. Decoding and writing happen on a dedicated
// goroutine so a slow viewer can never stall the UDP ingest/recording path.
type Hub struct {
	mu      sync.Mutex
	clients map[*websocket.Conn]bool
	packets chan []byte
}

func NewHub() *Hub {
	hub := &Hub{clients: map[*websocket.Conn]bool{}, packets: make(chan []byte, 64)}
	go hub.pump()
	return hub
}

// Send queues a raw UDP payload for broadcast. Never blocks: with no
// clients or a full queue the packet is dropped.
func (h *Hub) Send(data []byte) {
	if len(data) == 0 {
		return
	}
	h.mu.Lock()
	idle := len(h.clients) == 0
	h.mu.Unlock()
	if idle {
		return
	}
	buffer := make([]byte, len(data)) // receive buffer is reused — copy
	copy(buffer, data)
	select {
	case h.packets <- buffer:
	default:
	}
}

func (h *Hub) pump() {
	for data := range h.packets {
		message := data
		if data[0] != '{' { // not legacy JSON firmware → protobuf
			telemetry := &pb.Telemetry{}
			if err := proto.Unmarshal(data, telemetry); err != nil {
				continue
			}
			var err error
			if message, err = jsonMarshaler.Marshal(telemetry); err != nil {
				continue
			}
		}
		h.broadcast(message)
	}
}

func (h *Hub) broadcast(message []byte) {
	h.mu.Lock()
	defer h.mu.Unlock()
	for conn := range h.clients {
		conn.SetWriteDeadline(time.Now().Add(time.Second))
		if err := conn.WriteMessage(websocket.TextMessage, message); err != nil {
			conn.Close()
			delete(h.clients, conn)
		}
	}
}

func (h *Hub) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println("websocket upgrade:", err)
		return
	}
	h.mu.Lock()
	h.clients[conn] = true
	h.mu.Unlock()
	go func() { // reader: consumes control frames and detects close
		for {
			if _, _, err := conn.ReadMessage(); err != nil {
				h.mu.Lock()
				conn.Close()
				delete(h.clients, conn)
				h.mu.Unlock()
				return
			}
		}
	}()
}
