#pragma once

#include <Arduino.h>

class MIDIByteBuffer : public Stream {
public:
  MIDIByteBuffer() : head(0), tail(0) {}

  int available() override {
    return (head - tail + sizeof(buffer)) % sizeof(buffer);
  }

  int read() override {
    if (head == tail) return -1;
    uint8_t b = buffer[tail];
    tail = (tail + 1) % sizeof(buffer);
    return b;
  }

  int peek() override {
    if (head == tail) return -1;
    return buffer[tail];
  }

  size_t write(uint8_t b) override {
    uint8_t next = (head + 1) % sizeof(buffer);
    if (next == tail) return 0;
    buffer[head] = b;
    head = next;
    return 1;
  }

private:
  static const size_t BUFFER_SIZE = 512;
  uint8_t buffer[BUFFER_SIZE];
  volatile size_t head;
  volatile size_t tail;
};

extern MIDIByteBuffer midiBuffer;
