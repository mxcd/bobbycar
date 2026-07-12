package live

import (
	"encoding/json"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/gorilla/websocket"
	"github.com/mxcd/bob-e-car/logger/internal/pb"
	"google.golang.org/protobuf/proto"
)

func TestHubBroadcastsProtoAsJson(t *testing.T) {
	hub := NewHub()
	server := httptest.NewServer(hub)
	defer server.Close()

	conn, _, err := websocket.DefaultDialer.Dial(strings.Replace(server.URL, "http", "ws", 1), nil)
	if err != nil {
		t.Fatal(err)
	}
	defer conn.Close()

	payload, err := proto.Marshal(&pb.Telemetry{VehicleState: 3, SteeringThrottle: 128})
	if err != nil {
		t.Fatal(err)
	}
	// client registration races with Send's idle check — retry briefly
	deadline := time.Now().Add(2 * time.Second)
	received := make(chan []byte, 1)
	go func() {
		_, message, err := conn.ReadMessage()
		if err == nil {
			received <- message
		}
	}()
	for {
		hub.Send(payload)
		select {
		case message := <-received:
			fields := map[string]any{}
			if err := json.Unmarshal(message, &fields); err != nil {
				t.Fatal(err)
			}
			if fields["vehicleState"] != float64(3) || fields["steeringThrottle"] != float64(128) {
				t.Fatalf("unexpected message: %s", message)
			}
			return
		case <-time.After(20 * time.Millisecond):
			if time.Now().After(deadline) {
				t.Fatal("no websocket message received")
			}
		}
	}
}
