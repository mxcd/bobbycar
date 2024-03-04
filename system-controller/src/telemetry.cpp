#include "telemetry.h"

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 42, 1, 130);
IPAddress broadcastIp(10, 42, 1, 255);

unsigned int localPort = 4240;
unsigned int broadcastPort = 4242;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

elapsedMillis timeSinceLastTelemetryMessage = 0;

#define TELEMETRY_MESSAGE_INTERVAL 200

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
  String buffer;
  serializeJson(getStateData(), buffer);
  Udp.beginPacket(broadcastIp, broadcastPort);
  Udp.write(buffer.c_str());
  Udp.endPacket();
}