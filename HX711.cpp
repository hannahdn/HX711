#include <Arduino.h>
#include <HX711.h>

#if ARDUINO_VERSION <= 106

// "yield" is not implemented as noop in older Arduino Core releases, so let's
// define it.
// See also:
// https://stackoverflow.com/questions/34497758/what-is-the-secret-of-the-arduino-yieldfunction/34498165#34498165
void yield(void) {}

#endif // if ARDUINO_VERSION <= 106


HX711::HX711(uint8_t newValuesBufferSize, byte dout, byte pd_sck, byte gain) :
  valuesBufferSize(max(newValuesBufferSize, BUFFERSIZE_MAX)),
  index(0) {
  PD_SCK = pd_sck;
  DOUT   = dout;

  pinMode(PD_SCK, OUTPUT);
  pinMode(DOUT,   INPUT);

  set_gain(gain);
}

HX711::~HX711() {}

bool HX711::is_ready() {
  return digitalRead(DOUT) == LOW;
}

void HX711::set_gain(byte gain) {
  switch (gain) {
  case 128: // channel A, gain factor 128
    GAIN = 1;
    break;

  case 64: // channel A, gain factor 64
    GAIN = 3;
    break;

  case 32: // channel B, gain factor 32
    GAIN = 2;
    break;
  }

  digitalWrite(PD_SCK, LOW);
  read();
}

void HX711::update() {
  if (is_ready()) {
    values[index] = read();
    index++;

    if (index >= valuesBufferSize) index = 0; return true;
  }
  return false;
}

long HX711::read() {
  // // wait for the chip to become ready
  // while (!is_ready()) {
  //   // Will do nothing on Arduino but prevent resets of ESP8266 (Watchdog
  // Issue)
  //   yield();
  // }

  unsigned long value = 0;
  uint8_t data[3]     = { 0 };
  uint8_t filler      = 0x00;

  // pulse the clock pin 24 times to read the data
  data[2] = shiftIn(DOUT, PD_SCK, MSBFIRST);
  data[1] = shiftIn(DOUT, PD_SCK, MSBFIRST);
  data[0] = shiftIn(DOUT, PD_SCK, MSBFIRST);

  // set the channel and the gain factor for the next reading using the clock
  // pin
  for (unsigned int i = 0; i < GAIN; i++) {
    digitalWrite(PD_SCK, HIGH);
    digitalWrite(PD_SCK, LOW);
  }

  // Replicate the most significant bit to pad out a 32-bit signed integer
  if (data[2] & 0x80) {
    filler = 0xFF;
  } else {
    filler = 0x00;
  }

  // Construct a 32-bit signed integer
  value = (static_cast<unsigned long>(filler) << 24
           | static_cast<unsigned long>(data[2]) << 16
           | static_cast<unsigned long>(data[1]) << 8
           | static_cast<unsigned long>(data[0]));

  return static_cast<long>(value);
}

long HX711::read_average() {
  double sum = 0;

  for (unsigned int i = 0; i < valuesBufferSize; i++) {
    sum += values[i];
  }
  return sum / valuesBufferSize;
}

double HX711::get_value() {
  return read_average() - OFFSET;
}

float HX711::get_units() {
  return get_value() / SCALE;
}

void HX711::tare() {
  double sum = read_average();

  set_offset(sum);
}

void HX711::set_scale(float scale) {
  SCALE = scale;
}

float HX711::get_scale() {
  return SCALE;
}

void HX711::set_offset(long offset) {
  OFFSET = offset;
}

long HX711::get_offset() {
  return OFFSET;
}

void HX711::power_down() {
  digitalWrite(PD_SCK, LOW);
  digitalWrite(PD_SCK, HIGH);
}

void HX711::power_up() {
  digitalWrite(PD_SCK, LOW);
}
