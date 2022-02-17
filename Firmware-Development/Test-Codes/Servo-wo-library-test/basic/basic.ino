#include "ESP32PWM.h"

#define DEFAULT_TIMER_WIDTH 16
#define DEFAULT_PULSE_WIDTH_TICKS 4825
#define DEFAULT_TIMER_WIDTH_TICKS 65536

int ticks = DEFAULT_PULSE_WIDTH_TICKS; // current pulse width on this channel
int timer_width = DEFAULT_TIMER_WIDTH; // ESP32 allows variable width PWM timers
int timer_width_ticks = DEFAULT_TIMER_WIDTH_TICKS; // no. of ticks at rollover; varies with widt
int REFRESH_CPS = 50;

ESP32PWM pwm;

void setup() {
  // put your setup code here, to run once:
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  setPeriodHertz(50);

}

void loop() {
  // put your main code here, to run repeatedly:

}

uint8_t setPeriodHertz(uint8_t hertz)
{
  REFRESH_CPS = hertz;
  setTimerWidth(timer_width);
  return 1;
}

void setTimerWidth(int value)
{
    // only allow values between 16 and 20
    if (value < 16)
      value = 16;
  else if (value > 20)
    value = 20;

  // Fix the current ticks value after timer width change
  // The user can reset the tick value with a write() or writeUs()
  int widthDifference = timer_width - value;
  // if positive multiply by diff; if neg, divide
  if (widthDifference > 0)
  {
    ticks = widthDifference * ticks;
  }
  else if (widthDifference < 0)
  {
    ticks = ticks / -widthDifference;
  }

  timer_width = value;
  timer_width_ticks = pow(2, timer_width);
}
