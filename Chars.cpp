#include "Chars.h"

const unsigned char HUMIDITY_PATTERN[] = {
  0B00000,
  0B00100,
  0B01110,
  0B11111,
  0B11111,
  0B11111,
  0B01110,
  0B00000
};

const unsigned char LIGHT_PATTERN[] = {
  0B00000,
  0B10101,
  0B01110,
  0B11111,
  0B01110,
  0B10101,
  0B00000,
  0B00000
};

const unsigned char LIGHT_PATTERN_SELECTED[] = {
  0B11111,
  0B01010,
  0B10001,
  0B00000,
  0B10001,
  0B01010,
  0B11111,
  0B11111
};

const unsigned char COOLING_PATTERN[] = {
  0B00000,
  0B00000,
  0B00110,
  0B11001,
  0B00000,
  0B01100,
  0B10011,
  0B00000
};

const unsigned char COOLING_PATTERN_SELECTED[] = {
  0B11111,
  0B11111,
  0B11001,
  0B00110,
  0B11111,
  0B10011,
  0B01100,
  0B11111
};

const unsigned char WATERING_PATTERN[] = {
  0B01001,
  0B10010,
  0B01000,
  0B10001,
  0B00010,
  0B01000,
  0B10000,
  0B00000
};

const unsigned char WATERING_PATTERN_SELECTED[] = {
  0B10110,
  0B01101,
  0B10111,
  0B01110,
  0B11101,
  0B10111,
  0B01111,
  0B11111
};

const unsigned char BACK_PATTERN_SELECTED[] = {
  0B11111,
  0B11011,
  0B10111,
  0B00000,
  0B10111,
  0B11011,
  0B11111,
  0B11111
};
