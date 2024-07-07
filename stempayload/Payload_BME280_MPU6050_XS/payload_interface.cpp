#include <map>
#include <Wire.h>
#include <Arduino.h>

const byte muxAddress = 0x70;

// struct to contain both the I2C address and which multiplexer port a receiver is on
struct Payload_Address {
  int mux;
  int address;
};

// Add the addresses of any new payloads on the 
std::vector<Payload_Address> payloads = {
  // {0, 0x12}, // Payload 0 is in MUX slot 0 with I2C address 0x12
  {1, 0x12}, // Payload 1 is in MUX slot 1 with I2C address 0x12
  {4, 0x12}, // Payload 2 is in MUX slot 2 with I2C address 0x12
};

std::vector<std::vector<byte>> routing = {
  // {0, 15},   // APIDs 0-15 route to payload 0
  {16, 31}, // APIDS 16-31 route to payload 1
  {64, 79}  // APIDS 32-47 route to payload 2
};

void initialize_interface() {
  Wire.begin();
}

//
void interface_send(byte* packet, int length) {

  // Use APID to look up address and mux port of receiving payload
  byte apid = packet[1];
  Payload_Address address;
  for (int i=0; i < routing.size(); i++) {
    if (routing[i][0] < apid && routing[i][1] > apid) {
      address = payloads[i];
    }
  }

  // Write to MUX to switch to correct bus port
  Wire.beginTransmission(muxAddress);
  Wire.write(1 << address.mux);
  Wire.endTransmission();

  Serial.print("Command packet APID: ");
  Serial.println(apid);

  Wire.beginTransmission(address.address);
  Wire.write(packet, length);
  Wire.endTransmission();
}

void interface_read() {

  Serial.println("Requesting payload data transfer"); 

  // Give each payload the opportunity to transfer packets
  for (int i=0; i<payloads.size(); i++) {
    // Switch to the proper MUX
    Wire.beginTransmission(muxAddress);
    Wire.write(1 << payloads[i].mux);
    Wire.endTransmission();

    // Request the transmission for payload i, up to 1024 bytes
    int returnSize = Wire.requestFrom(payloads[i].address, 64);
    Serial.printf("Payload %i response:\n", i);
    while (Wire.available()) {
      byte b = Wire.read();
      Serial.print(String(b, HEX));
      Serial.print(" ");
    }
    Serial.println("");
    
  }

}



// Code from this point down is only for test / demo convenience purposes during integration weekend,
// not an actual part of the interface. The interface itself knows nothing about packet formats.


/* Array of predefined CCSDS telecommand packets that can be manually triggered for testing purposes.
Format of packet is:
3 bits: packet version number
1 bit: packet type (0 = tlm, 1 = cmd)
1 bit: secondary header present (1 = true, 0 = false)
11 bits: Application Process Identifier (APID) - this is used for routing to the payload
2 bits: Sequence flags (00 = continuation, 01 = first segment, 10 = last segment, 11 = unsegmented)
14 bits: Sequence count
16 bits: Packet data length
*/

// Utility method for manually sending command packets
// For convenience for now, limiting both APID and data length to < 256
void command_test(byte apid, byte* data, byte length) {
  Serial.println("Command test");
  // Only allowing APIDs up to 255 for convenience
  if (apid > 255 || length > 255) {
    Serial.println("Maximum APID is 255; maximum data length is 255 bytes");
    return;
  }

  // Array to store the CCSDS packet
  byte packet[6+length];

  // Fill the CCSDS header
  packet[0] = 0x10; // (3 bits) Packet version 0, (1 bit) packet type = cmd, (1 bit) no secondary header, (3 bits) unused APID start
  packet[1] = apid; // (8 bits) APID
  packet[2] = 0xC0; // (2 bits) Unsegmented, (6 bits) sequence count = 0
  packet[3] = 0x00; // (8 bits) sequence count = 0
  packet[4] = 0x00; // (8 bits) unused data length
  packet[5] = length; // (8 bits) data length

  // Fill the CCSDS data
  for (int i=6; i < 6+length; i++) {
    packet[i] = data[i-6];
  }

  Serial.print("Outgoing command: ");
  for (int i=0; i < 6+length; i++) {
    byte b = packet[i];
    Serial.print(String(b, HEX));
    Serial.print(" ");
  }

  interface_send(packet, 6+length);
}

int char2int(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  else return 0;
}

byte hex2byte(char upper, char lower)
{
  int result = char2int(upper)*16 + char2int(lower);
  return byte(result);
}