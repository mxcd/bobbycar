package main

import (
	"os"

	"github.com/mxcd/bobbycar/internal/server"
	"github.com/mxcd/bobbycar/internal/udp"
	"github.com/rs/zerolog/log"
)

func main() {
	serverContext, err := server.InitServer()
	if err != nil {
		log.Error().Err(err).Msg("Failed to initialize server")
		os.Exit(1)
	}

	udpServer, err := udp.InitUdpServer(serverContext)
	if err != nil {
		log.Error().Err(err).Msg("Failed to initialize UDP server")
		os.Exit(1)
	}
	defer udpServer.UdpConnection.Close()

	serverContext.Engine.Run(":8080")
}
