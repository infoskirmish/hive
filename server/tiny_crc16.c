#include "tiny_crc16.h"

uint16_t
tiny_crc16(const uint8_t * msg, uint32_t sz){
  
  uint32_t index;
  uint16_t crc;
  uint8_t val, t;

  /* 
   * CRC16 Lookup tables (High and Low Byte) for 4 bits per iteration. 
   */
  unsigned short CRC16_LookupHigh[16] = {
    0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
    0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1
  };
  unsigned short CRC16_LookupLow[16] = {
    0x00, 0x21, 0x42, 0x63, 0x84, 0xA5, 0xC6, 0xE7,
    0x08, 0x29, 0x4A, 0x6B, 0x8C, 0xAD, 0xCE, 0xEF
  };
  
  /*
   * CRC16 "Register". This is implemented as two 8bit values
   */
  unsigned char CRC16_High, CRC16_Low;
  // Initialise the CRC to 0xFFFF for the CCITT specification
  CRC16_High = 0xFF;
  CRC16_Low = 0xFF;

  for (index = 0; index < sz; index++){
    val = msg[index] >> 4;

    // Step one, extract the Most significant 4 bits of the CRC register
    t = CRC16_High >> 4;

    // XOR in the Message Data into the extracted bits
    t = t ^ val;

    // Shift the CRC Register left 4 bits
    CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4);
    CRC16_Low = CRC16_Low << 4;
    
    // Do the table lookups and XOR the result into the CRC Tables
    CRC16_High = CRC16_High ^ CRC16_LookupHigh[t];
    CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t];

    val = msg[index] & 0x0F;

    // Step one, extract the Most significant 4 bits of the CRC register
    t = CRC16_High >> 4;

    // XOR in the Message Data into the extracted bits
    t = t ^ val;

    // Shift the CRC Register left 4 bits
    CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4);
    CRC16_Low = CRC16_Low << 4;

    // Do the table lookups and XOR the result into the CRC Tables
    CRC16_High = CRC16_High ^ CRC16_LookupHigh[t];
    CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t];
  }
  crc = CRC16_High;
  crc = crc << 8;
  crc = crc ^ CRC16_Low;

  return crc;
}
