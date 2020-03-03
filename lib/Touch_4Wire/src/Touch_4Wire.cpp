// Touch screen library with X Y and Z (pressure) readings as well
// as oversampling to avoid 'bouncing'
// (c) ladyada / adafruit
// Code under MIT License
//
// ported for Spark Core by ScruffR Nov. 2014

#include "Touch_4Wire.h"

#if defined (PARTICLE)
#include <Particle.h>
#else
#include "pins_arduino.h"
#include "wiring_private.h"
#include <avr/pgmspace.h>
#endif

// increase or decrease the touchscreen oversampling. This is a little different than you may think:
// 1 is no oversampling, whatever data we get is immediately returned
// 2 is double-sampling and we only return valid data if both points are the same
// 3+ uses insert sort to get the median value.
// We found 2 is precise yet not too slow so we suggest sticking with it!

#define NUMSAMPLES 2

TouchScreen::TouchScreen(uint8_t xp, uint8_t yp, uint8_t xm, uint8_t ym) {
  _yp = yp;
  _xm = xm;
  _ym = ym;
  _xp = xp;
  _rxplate = 0;
  pressureThreshhold = 10;
}

TouchScreen::TouchScreen(uint8_t xp, uint8_t yp, uint8_t xm, uint8_t ym,
       uint16_t rxplate) {
  _yp = yp;
  _xm = xm;
  _ym = ym;
  _xp = xp;
  _rxplate = rxplate;

  pressureThreshhold = 10;
}

TSPoint::TSPoint(void) {
  x = y = 0;
}

TSPoint::TSPoint(int16_t x0, int16_t y0, int16_t z0) {
  x = x0;
  y = y0;
  z = z0;
}

bool TSPoint::operator==(TSPoint p1) {
  return  ((p1.x == x) && (p1.y == y) && (p1.z == z));
}

bool TSPoint::operator!=(TSPoint p1) {
  return  ((p1.x != x) || (p1.y != y) || (p1.z != z));
}

#if (NUMSAMPLES > 2)
static void insert_sort(int array[], uint8_t size) {
  uint8_t j;
  int save;

  for (int i = 1; i < size; i++) {
    save = array[i];
    for (j = i; j >= 1 && save < array[j - 1]; j--)
      array[j] = array[j - 1];
    array[j] = save;
  }
}
#endif

TSPoint TouchScreen::getPoint(void) {
  int x, y, z;
  int samples[NUMSAMPLES];
  uint8_t i, valid;

#if defined (SPARK)
  // nothing here
#else
  uint8_t xp_port = digitalPinToPort(_xp);
  uint8_t yp_port = digitalPinToPort(_yp);
  uint8_t xm_port = digitalPinToPort(_xm);
  uint8_t ym_port = digitalPinToPort(_ym);

  uint8_t xp_pin = digitalPinToBitMask(_xp);
  uint8_t yp_pin = digitalPinToBitMask(_yp);
  uint8_t xm_pin = digitalPinToBitMask(_xm);
  uint8_t ym_pin = digitalPinToBitMask(_ym);
#endif

  valid = 1;

  pinMode(_yp, INPUT);
  pinMode(_ym, INPUT);

  // ScruffR: I guess this should detach PULL_UPs
#if defined (SPARK)
  // ScruffR: Since Spark Core has no PULL UPs with INPUT - not needed
  //pinResetFast(_yp);
  //pinResetFast(_ym);
#else
  //digitalWrite(_yp, LOW);
  //digitalWrite(_ym, LOW);
  *portOutputRegister(yp_port) &= ~yp_pin;
  *portOutputRegister(ym_port) &= ~ym_pin;
#endif

  pinMode(_xp, OUTPUT);
  pinMode(_xm, OUTPUT);

#if defined (SPARK)
  pinSetFast(_xp);
  pinResetFast(_xm);
#else
  //digitalWrite(_xp, HIGH);
  //digitalWrite(_xm, LOW);
  *portOutputRegister(xp_port) |= xp_pin;
  *portOutputRegister(xm_port) &= ~xm_pin;
#endif

   for (i=0; i<NUMSAMPLES; i++) {
     samples[i] = analogRead(_yp);
   }

#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
  if (abs(samples[0] - samples[1]) > XY_TOLERANCE) { valid = 0; }
#endif

   x = (ADC_MAX_VALUE - samples[NUMSAMPLES/2]);

   pinMode(_xp, INPUT);
   pinMode(_xm, INPUT);

   // ScruffR: I guess this should detach PULL_UPs
#if defined (SPARK)
   // ScruffR: Since Spark Core has no PULL UPs with INPUT - not needed
   //pinResetFast(_xp);
#else
   //digitalWrite(_xp, LOW);
   *portOutputRegister(xp_port) &= ~xp_pin;
#endif

   pinMode(_yp, OUTPUT);
   pinMode(_ym, OUTPUT);

#if defined (SPARK)
   pinSetFast(_yp);
   pinResetFast(_ym);  // ScruffR: since we didn't do it before
#else
   //digitalWrite(_yp, HIGH);
   *portOutputRegister(yp_port) |= yp_pin;
#endif


   for (i=0; i<NUMSAMPLES; i++) {
     samples[i] = analogRead(_xm);
   }

#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
   if (abs(samples[0] - samples[1]) > XY_TOLERANCE) { valid = 0; }
#endif

   y = (ADC_MAX_VALUE - samples[NUMSAMPLES/2]);

   // Set X+ to ground
   pinMode(_xp, OUTPUT);

#if defined (SPARK)
   pinResetFast(_xp);
   pinSetFast(_ym);
   pinResetFast(_yp);
#else
   //digitalWrite(_xp, LOW);
   *portOutputRegister(xp_port) &= ~xp_pin;

   // Set Y- to VCC
   //digitalWrite(_ym, HIGH);
   *portOutputRegister(ym_port) |= ym_pin;

   // Hi-Z X- and Y+
   //digitalWrite(_yp, LOW);
   *portOutputRegister(yp_port) &= ~yp_pin;
#endif

   pinMode(_yp, INPUT);

   int z1 = analogRead(_xm);
   int z2 = analogRead(_yp);

   if (_rxplate != 0) {
     // now read the x
     float rtouch;
     rtouch = z2;
     rtouch /= z1;
     rtouch -= 1;
     rtouch *= x;
     rtouch *= _rxplate;
     rtouch /= ADC_MAX_VALUE + 1;

     z = rtouch;
   } else {
     z = (ADC_MAX_VALUE - (z2 - z1));
   }

   if (!valid) {
     z = -1;
   }

   _prevAction = TOUCH_PRESSURE;

   return TSPoint(x, y, z);
}

TSPoint TouchScreen::getPoint(bool withPressure) {
  int x, y, z;

  if (_prevAction == TOUCH_X) {
    x = readTouchX();
#if (NUMSAMPLES == 2)
    if (abs(x - readTouchX()) > XY_TOLERANCE)
      return TSPoint(0,0,-1);
#endif
    if (withPressure)
      z = pressure();
    y = readTouchY();
#if (NUMSAMPLES == 2)
    if (abs(y - readTouchY()) > XY_TOLERANCE)
      return TSPoint(0,0,-1);
#endif
  } else {
    y = readTouchY();
#if (NUMSAMPLES == 2)
    if (abs(y - readTouchY()) > XY_TOLERANCE)
      return TSPoint(0,0,-1);
#endif
    if (withPressure)
      z = pressure();
    x = readTouchX();
#if (NUMSAMPLES == 2)
    if (abs(x - readTouchX()) > XY_TOLERANCE)
      return TSPoint(0,0,-1);
#endif
  }

  return TSPoint(x, y, z);
}

int TouchScreen::readTouchX(void) {
if (_prevAction != TOUCH_X) {
  _prevAction = TOUCH_X;

  pinMode(_yp, INPUT);
  pinMode(_ym, INPUT);

  pinMode(_xp, OUTPUT);
  pinMode(_xm, OUTPUT);

#if defined(SPARK)
  pinSetFast(_xp);
  pinResetFast(_xm);
#else
  digitalWrite(_yp, LOW);
  digitalWrite(_ym, LOW);

  digitalWrite(_xp, HIGH);
  digitalWrite(_xm, LOW);
#endif
  }
  return (ADC_MAX_VALUE - analogRead(_yp));
}

int TouchScreen::readTouchY(void) {
  if (_prevAction != TOUCH_Y) {
    _prevAction = TOUCH_Y;

    pinMode(_xp, INPUT);
    pinMode(_xm, INPUT);

    pinMode(_yp, OUTPUT);
    pinMode(_ym, OUTPUT);

#if defined (SPARK)
    pinSetFast(_yp);
    pinResetFast(_ym);
#else
    digitalWrite(_xp, LOW);
    digitalWrite(_xm, LOW);

    digitalWrite(_yp, HIGH);
    digitalWrite(_ym, LOW);
#endif
  }
  return (ADC_MAX_VALUE - analogRead(_xm));
}

uint16_t TouchScreen::pressure(void) {
  if (_prevAction != TOUCH_PRESSURE) {
    _prevAction = TOUCH_PRESSURE;

    pinMode(_xm, INPUT);
    pinMode(_yp, INPUT);

    pinMode(_xp, OUTPUT);
    pinMode(_ym, OUTPUT);

#if defined(SPARK)
    pinResetFast(_xp);
    pinSetFast(_ym);
    // ScruffR: Since Spark Core has no PULL UPs with INPUT - not needed
    //pinResetFast(_xm);
    //pinResetFast(_yp);
#else
    // Set X+ to ground
    digitalWrite(_xp, LOW);

    // Set Y- to VCC
    digitalWrite(_ym, HIGH);

    // Hi-Z X- and Y+
    digitalWrite(_xm, LOW);
    digitalWrite(_yp, LOW);
#endif
  }

  int z1 = analogRead(_xm);
  int z2 = analogRead(_yp);

  if (_rxplate != 0) {
    // now read the x
    float rtouch;
    rtouch = z2;
    rtouch /= z1;
    rtouch -= 1;
    rtouch *= readTouchX();
    rtouch *= _rxplate;
    rtouch /= ADC_MAX_VALUE + 1;

    return rtouch;
  } else {
    return (ADC_MAX_VALUE - (z2 - z1));
  }
}
