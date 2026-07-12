// Simulator broadcasts system-controller-like telemetry for testing the
// logger on a laptop. The DMS follows a 40s cycle that includes a 1s
// accidental release to exercise the log-continuity edge case:
// 0-10s released, 10-20s pressed, 20-21s released (blip), 21-30s pressed,
// 30-40s released.
package main

import (
	"encoding/json"
	"flag"
	"log"
	"math"
	"net"
	"time"

	"github.com/mxcd/bob-e-car/logger/internal/pb"
	"google.golang.org/protobuf/proto"
)

func main() {
	target := flag.String("target", "127.0.0.1:4242", "UDP target address")
	interval := flag.Duration("interval", 20*time.Millisecond, "send interval (firmware: 20ms)")
	legacyJson := flag.Bool("json", false, "send legacy JSON instead of protobuf")
	flag.Parse()

	conn, err := net.Dial("udp", *target)
	if err != nil {
		log.Fatalln(err)
	}
	format := "protobuf"
	if *legacyJson {
		format = "legacy JSON"
	}
	log.Printf("simulating system controller telemetry (%s) to %s every %s", format, *target, *interval)

	start := time.Now()
	loopCounter := uint32(0)
	for range time.Tick(*interval) {
		loopCounter += 2
		t := time.Since(start).Seconds()
		cycle := math.Mod(t, 40)
		dms := (cycle >= 10 && cycle < 20) || (cycle >= 21 && cycle < 30)

		throttle := 0.0
		if dms {
			throttle = 127 + 127*math.Sin(t/2)
		}
		telemetry := &pb.Telemetry{
			LoopCounter:                  loopCounter,
			VehicleState:                 3,
			SteeringStartupCheckOk:       true,
			SteeringDeadManSwitchPressed: dms,
			SteeringAccelerationCommand:  dms && throttle > 10,
			SteeringThrottle:             uint32(math.Round(throttle)),
			BatteryOk:                    true,
			RelayMcEn:                    true,
			RelayPrecharge:               true,
			RelayScOk:                    true,
			VdBatteryVoltage:             float32(48.0 - t*0.005 + 0.2*math.Sin(t*3)),
			VdMotorCurrentLeft:           float32(throttle / 255 * 20),
			VdMotorCurrentRight:          float32(throttle / 255 * 20),
			VdErpmLeft:                   int32(throttle / 255 * 8000),
			VdErpmRight:                  int32(throttle / 255 * 8000),
			VdFetTemp:                    float32(30 + throttle/255*15),
			VdMotorTemp:                  float32(25 + throttle/255*20),
			VdInputCurrent:               float32(throttle / 255 * 35),
			VdDutyCycle:                  float32(throttle / 255),
		}

		var data []byte
		if *legacyJson {
			data, _ = json.Marshal(map[string]any{
				"loopCounter":                  loopCounter,
				"steeringDeadManSwitchPressed": dms,
				"steeringThrottle":             math.Round(throttle),
				"vdBatteryVoltage":             48.0 - t*0.005,
			})
		} else {
			data, _ = proto.Marshal(telemetry)
		}
		if _, err := conn.Write(data); err != nil {
			log.Println("send failed:", err)
		}
	}
}
