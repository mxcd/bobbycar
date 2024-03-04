package udp

import (
	"encoding/json"
	"fmt"
	"net"

	"github.com/mxcd/bobbycar/internal/server"
)

type UdpServer struct {
	UdpConnection *net.UDPConn
}

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

			jsonData := buffer[:n]
			jsonObject := make(map[string]interface{})
			err = json.Unmarshal(jsonData, &jsonObject)
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
