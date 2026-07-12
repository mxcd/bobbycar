#include "telemetry.h"
#include <pb_encode.h>

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
// vehicle LAN 10.42.10.0/24 — deliberately NOT the 10.42.1.0/24 home LAN
// subnet, so the logger Pi can hold both without route conflicts
IPAddress ip(10, 42, 10, 130);
IPAddress broadcastIp(10, 42, 10, 255);

unsigned int localPort = 4240;
unsigned int broadcastPort = 4242;

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

elapsedMillis timeSinceLastTelemetryMessage = 0;

#define TELEMETRY_MESSAGE_INTERVAL 20 // ms → 50Hz

void telemetrySetup() {
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
}

void telemetryLoop() {
  if(timeSinceLastTelemetryMessage > TELEMETRY_MESSAGE_INTERVAL) {
    _sendTelemetryMessage();
    timeSinceLastTelemetryMessage -= TELEMETRY_MESSAGE_INTERVAL;
  }
}

void _sendTelemetryMessage() {
  uint8_t buffer[bob_Telemetry_size];
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
  if (!pb_encode(&stream, bob_Telemetry_fields, getTelemetry())) {
    return;
  }
  Udp.beginPacket(broadcastIp, broadcastPort);
  Udp.write(buffer, stream.bytes_written);
  Udp.endPacket();
}
