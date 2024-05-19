#ifndef __PACKET_H
#define __PACKET_H

#include <stdint.h>

#define PACKET_MAX_SIZE (32 + PACKET_HEADER_SIZE + PACKET_CRC_SIZE)
#define PACKET_HEADER_SIZE 2 // cmd and length
#define PACKET_CRC_SIZE 0    // unused

typedef union {
  uint8_t data[PACKET_MAX_SIZE];
  struct {
    uint8_t cmd;
    uint8_t length;
    uint8_t data[PACKET_MAX_SIZE - PACKET_HEADER_SIZE - PACKET_CRC_SIZE];
    uint8_t crc; // unused
  } packet_struct;
} packet_t;

#endif // __PACKET_H