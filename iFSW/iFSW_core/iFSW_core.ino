#include <Wire.h>
#include <Adafruit_NeoPixel.h>

// LED control parameters
#define LED_PIN 25
#define LED_COUNT 1
#define BRIGHTNESS 200
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

// I2C parameters
const int i2c_address = 0x12;
const byte maj_apid = 0x10;

// Sclk synchronizing
struct SCLK {
  unsigned long sclk;
  unsigned long localMillis;
} sclk;


// HK packet holder
struct HK {
  byte seq;
  uint32_t sclk;
  uint16_t cmdCount;
  uint32_t lastCmdSclk;
  byte cmdEcho;
  int32_t lastTransferSclk;
} hk;

// To add new commands, insert the command call into the switch statement for the lower nibble of the APID
void mapCommand(byte m_apid, byte* packet) {

  hk.lastCmdSclk = millis() / 1000;
  Serial.println("mapping command");
  Serial.println(m_apid);

  switch(m_apid) {
    case 0: // Update reference sclk 
      setSclk(packet);
      break;
    case 1: // NOOP
      break;
    case 2: // Change status light color
      setStatusColor(packet);
      break;
    case 3: // Populate science data packet
      break; // Not yet implemented
    case 4:
      break;
    case 5:
      break;
    case 6:
      break;
    case 7:
      break;
    case 8:
      break;
    case 9:
      break;
  }

}


void setup() {
  
  Serial.println("Very start?");
  // Listen on I2C Bus as receiver
  Wire.begin(i2c_address);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  // Serial for debugging
  Serial.begin(115200);

  strip.begin();
  strip.clear();
  strip.setBrightness(50);

  // Initial color green
  setPixelColor(0, 0xFF, 0);
}

void requestEvent() {
  // Show blue to indicate data is transferring
  setPixelColor(0, 0, 0xFF);
  Serial.println("Received request");
  sendHkPacket();
}

void sendHkPacket() {
  // Update time in HK packet
  hk.sclk = millis() / 1000;

  byte packet[64];
  packet[0] = 0x00;
  packet[1] = maj_apid | 0x0A; 
  packet[2] = 0xC0;
  hk.seq++;
  packet[3] = hk.seq; // Sequence count up
  packet[4] = 0x00; // Unused data length
  packet[5] = 0x3A; // Packet data length = 58
  // First four bytes are sclk
  packet[9] = hk.sclk & 0xFF;
  packet[8] = (hk.sclk >> 8) & 0xFF;
  packet[7] = (hk.sclk >> 16) & 0xFF;
  packet[6] = (hk.sclk >> 24) & 0xFF;
  // Command count
  packet[10] = hk.cmdCount;
  // APID of last command
  packet[11] = hk.cmdEcho;
  // Time of last command
  packet[15] = hk.lastCmdSclk & 0xFF;
  packet[14] = (hk.lastCmdSclk >> 8) & 0xFF;
  packet[13] = (hk.lastCmdSclk >> 16) & 0xFF;
  packet[12] = (hk.lastCmdSclk >> 24) & 0xFF;
  // Time of last data transfer
  packet[19] = hk.lastTransferSclk & 0xFF;
  packet[18] = (hk.lastTransferSclk >> 8) & 0xFF;
  packet[17] = (hk.lastTransferSclk >> 16) & 0xFF;
  packet[16] = (hk.lastTransferSclk >> 24) & 0xFF;

  // Zero out the rest for now
  for (int i=20; i<64; i++) {
    packet[i] = 0;
  }

  hk.lastTransferSclk = millis()/1000;

  Wire.write(packet, 64);
}

void receiveEvent(int num) {

  hk.cmdCount++;
  Serial.print("Command count ");
  Serial.print(hk.cmdCount);
  Serial.print(": ");

  int message_length = Wire.available();
  byte message[message_length];
  for (int i=0; i < message_length; i++) {
    byte b = Wire.read();
    Serial.print(String(b, HEX));
    Serial.print(" ");
    message[i] = b;
  }

  byte apid = message[1];
  hk.cmdEcho = apid;
  mapCommand(0x0F & apid, message);

  Serial.println("");
}

void setStatusColor(byte* packet) {
  // Serial.printf("Setting status color: %i %i %i\n", red, green, blue);
  setPixelColor(int(packet[6]), int(packet[7]), int(packet[8]));
}

void setPixelColor(int red, int green, int blue) {
  strip.setPixelColor(0, strip.Color(green, red, blue));
  strip.show();
}

// Record an sclk value relative to milliseconds since iFSW started running to properly tag packets
void setSclk(byte* packet) {

}

void loop() {
  
  // Nothing for now, just avoid hogging bus to let incoming I2C requests be handled
  // Eventually with a sensor connected it could be taking measurements here
  delay(100);

  // If no commands or transfers have happened in the last 10s, set status to red
  if ((millis() / 1000 - hk.lastCmdSclk) > 10 && (millis() / 1000 - hk.lastTransferSclk) > 10) {
    setPixelColor(0xFF, 0, 0);
  }
}
