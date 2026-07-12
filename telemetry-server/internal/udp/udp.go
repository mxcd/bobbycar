package udp

import (
	"fmt"
	"net"

	"github.com/mxcd/bobbycar/internal/pb"
	"github.com/mxcd/bobbycar/internal/server"
	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/proto"
)

type UdpServer struct {
	UdpConnection *net.UDPConn
}

// protojson with EmitUnpopulated produces the same camelCase keys (incl.
// zero values) the dashboard consumed from the legacy JSON firmware.
var jsonMarshaler = protojson.MarshalOptions{EmitUnpopulated: true}

func InitUdpServer(server *server.Server) (*UdpServer, error) {
	// Define the UDP port to listen on
	const port = ":4242"

	addr, err := net.ResolveUDPAddr("udp", port)
	if err != nil {
		return nil, err
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		return nil, err
	}

	receiveLoop := func() {
		buffer := make([]byte, 2048)

		for {
			n, _, err := conn.ReadFromUDP(buffer)
			if err != nil {
				fmt.Println(err)
				continue
			}
			data := buffer[:n]

			// legacy JSON firmware — pass through unchanged
			if n > 0 && data[0] == '{' {
				server.WebsocketManager.Broadcast(data)
				continue
			}

			telemetry := &pb.Telemetry{}
			if err := proto.Unmarshal(data, telemetry); err != nil {
				fmt.Println(err)
				continue
			}
			jsonData, err := jsonMarshaler.Marshal(telemetry)
			if err != nil {
				fmt.Println(err)
				continue
			}
			server.WebsocketManager.Broadcast(jsonData)
		}
	}
	go receiveLoop()

	return &UdpServer{
		UdpConnection: conn,
	}, nil
}
