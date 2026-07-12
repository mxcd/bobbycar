package ingest

import (
	"context"
	"encoding/json"
	"log"
	"net"
	"sync/atomic"
	"syscall"
	"time"

	"github.com/mxcd/bob-e-car/logger/internal/pb"
	"golang.org/x/sys/unix"
	"google.golang.org/protobuf/proto"
)

const (
	dmsKey  = "steeringDeadManSwitchPressed"
	flagKey = "steeringAccelerationCommand" // steering wheel "R" button, repurposed as flag marker
)

// Frame is one received telemetry packet plus the decoded trigger signals
// the recorder acts on.
type Frame struct {
	Record         *pb.LogRecord
	Dms            bool
	Flag           bool
	VehicleState   uint32
	BatteryVoltage float64
}

// Ingest listens for the system controller's UDP broadcasts (protobuf
// bob.Telemetry, or legacy JSON) and turns each packet into a LogRecord.
// SO_REUSEPORT lets the logger share the port with the telemetry-server on
// the same host. onPacket (optional) receives every valid raw payload — it
// must not retain the slice or block.
type Ingest struct {
	conn             net.PacketConn
	Records          chan *Frame
	onPacket         func([]byte)
	packetCount      atomic.Int64
	lastPacketUs     atomic.Int64
	vehicleState     atomic.Uint32
	batteryMillivolt atomic.Int64
}

func Listen(ctx context.Context, addr string, onPacket func([]byte)) (*Ingest, error) {
	config := net.ListenConfig{
		Control: func(network, address string, c syscall.RawConn) error {
			var sockErr error
			err := c.Control(func(fd uintptr) {
				sockErr = unix.SetsockoptInt(int(fd), unix.SOL_SOCKET, unix.SO_REUSEPORT, 1)
			})
			if err != nil {
				return err
			}
			return sockErr
		},
	}
	conn, err := config.ListenPacket(ctx, "udp", addr)
	if err != nil {
		return nil, err
	}

	ingest := &Ingest{conn: conn, Records: make(chan *Frame, 256), onPacket: onPacket}
	go ingest.receiveLoop()
	go func() {
		<-ctx.Done()
		conn.Close()
	}()
	return ingest, nil
}

func (i *Ingest) receiveLoop() {
	buffer := make([]byte, 4096)
	for {
		n, _, err := i.conn.ReadFrom(buffer)
		if err != nil {
			close(i.Records)
			return
		}
		frame := parse(buffer[:n])
		if frame == nil {
			continue
		}
		i.packetCount.Add(1)
		i.lastPacketUs.Store(int64(frame.Record.TimestampUs))
		i.vehicleState.Store(frame.VehicleState)
		i.batteryMillivolt.Store(int64(frame.BatteryVoltage * 1000))
		if i.onPacket != nil {
			i.onPacket(buffer[:n])
		}
		select {
		case i.Records <- frame:
		default:
			log.Println("record channel full, dropping packet")
		}
	}
}

func parse(data []byte) *Frame {
	if len(data) == 0 {
		return nil
	}
	timestampUs := uint64(time.Now().UnixMicro())

	// legacy JSON firmware — a proto frame can never start with '{'
	if data[0] == '{' {
		fields := map[string]any{}
		if err := json.Unmarshal(data, &fields); err != nil {
			return nil
		}
		record := &pb.LogRecord{TimestampUs: timestampUs, Values: make(map[string]float64, len(fields))}
		for key, value := range fields {
			switch v := value.(type) {
			case float64:
				record.Values[key] = v
			case bool:
				if v {
					record.Values[key] = 1
				} else {
					record.Values[key] = 0
				}
			}
		}
		return &Frame{
			Record:         record,
			Dms:            record.Values[dmsKey] != 0,
			Flag:           record.Values[flagKey] != 0,
			VehicleState:   uint32(record.Values["vehicleState"]),
			BatteryVoltage: record.Values["vdBatteryVoltage"],
		}
	}

	telemetry := &pb.Telemetry{}
	if err := proto.Unmarshal(data, telemetry); err != nil {
		return nil
	}
	raw := make([]byte, len(data)) // receive buffer is reused — copy
	copy(raw, data)
	return &Frame{
		Record:         &pb.LogRecord{TimestampUs: timestampUs, Telemetry: raw},
		Dms:            telemetry.SteeringDeadManSwitchPressed,
		Flag:           telemetry.SteeringAccelerationCommand,
		VehicleState:   telemetry.VehicleState,
		BatteryVoltage: float64(telemetry.VdBatteryVoltage),
	}
}

type Stats struct {
	PacketCount    int64   `json:"packetCount"`
	LastPacketUs   int64   `json:"lastPacketUs"`
	VehicleState   uint32  `json:"vehicleState"`
	BatteryVoltage float64 `json:"batteryVoltage"`
}

func (i *Ingest) Stats() Stats {
	return Stats{
		PacketCount:    i.packetCount.Load(),
		LastPacketUs:   i.lastPacketUs.Load(),
		VehicleState:   i.vehicleState.Load(),
		BatteryVoltage: float64(i.batteryMillivolt.Load()) / 1000,
	}
}
