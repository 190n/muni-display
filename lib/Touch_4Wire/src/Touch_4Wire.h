// Touch screen library with X Y and Z (pressure) readings as well
// as oversampling to avoid 'bouncing'
// (c) ladyada / adafruit
// Code under MIT License
//
// ported for Spark Core by ScruffR Nov. 2014

#ifndef _ADAFRUIT_TOUCH4WIRE_H_
#define _ADAFRUIT_TOUCH4WIRE_H_

#if defined(PARTICLE)
#include <Particle.h>

// some very useful macros for Spark Core and porting Arduino libraries for it

#if (SYSTEM_VERSION < 0x00040400) // no fast pin functions before 0.4.4
#if defined(STM32F2XX)  // for the Photon and friends
#define pinResetFast(_pin)	               (PIN_MAP[_pin].gpio_peripheral->BSRRH = PIN_MAP[_pin].gpio_pin)
#define pinSetFast(_pin)	                 (PIN_MAP[_pin].gpio_peripheral->BSRRL = PIN_MAP[_pin].gpio_pin)
#elif defined(STM32F10X_MD)  // for the Core
#define pinResetFast(_pin)	               (PIN_MAP[_pin].gpio_peripheral->BRR = PIN_MAP[_pin].gpio_pin)
#define pinSetFast(_pin)	                 (PIN_MAP[_pin].gpio_peripheral->BSRR = PIN_MAP[_pin].gpio_pin)
#endif
#define digitalWriteFast(_pin, _hilo)      (_hilo ? pinSetFast(_pin) : pinResetFast(_pin))
#define pinReadFast(_pin)                  (PIN_MAP[_pin].gpio_peripheral->IDR & PIN_MAP[_pin].gpio_pin ? 0xFF : LOW)
#endif

#define ADC_MAX_VALUE (0x0FFF)
#define XY_TOLERANCE 15

#else
#include <stdint.h>

#define ADC_MAX_VALUE (0x03FF)
#define XY_TOLERANCE 5
#endif

typedef enum TOUCH_ACTIONS {
  TOUCH_UNDEF,
  TOUCH_X,
  TOUCH_Y,
  TOUCH_PRESSURE
};

class TSPoint {
 public:
  TSPoint(void);
  TSPoint(int16_t x, int16_t y, int16_t z);

  bool operator==(TSPoint);
  bool operator!=(TSPoint);

  int16_t x, y, z;
};

class TouchScreen {
 public:
  TouchScreen(uint8_t xp, uint8_t yp, uint8_t xm, uint8_t ym);
  TouchScreen(uint8_t xp, uint8_t yp, uint8_t xm, uint8_t ym, uint16_t rx);

  bool isTouching(void);
  uint16_t pressure(void);
  int readTouchY();
  int readTouchX();
  TSPoint getPoint();
  TSPoint getPoint(bool withPressure);
  int16_t pressureThreshhold;

private:
  uint8_t _yp, _ym, _xm, _xp;
  uint16_t _rxplate;
  TOUCH_ACTIONS _prevAction = TOUCH_UNDEF;
};

#endif
