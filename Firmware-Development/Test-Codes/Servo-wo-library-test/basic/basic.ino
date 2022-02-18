#include "ESP32PWM.h"

#define MIN_PULSE_WIDTH       500     // the shortest pulse sent to a servo  
#define MAX_PULSE_WIDTH      2500     // the longest pulse sent to a servo 
#define DEFAULT_TIMER_WIDTH 16
#define DEFAULT_PULSE_WIDTH_TICKS 4825
#define DEFAULT_TIMER_WIDTH_TICKS 65536
#define REFRESH_USEC         20000

int ticks = DEFAULT_PULSE_WIDTH_TICKS; // current pulse width on this channel
int timer_width = DEFAULT_TIMER_WIDTH; // ESP32 allows variable width PWM timers
int timer_width_ticks = DEFAULT_TIMER_WIDTH_TICKS; // no. of ticks at rollover; varies with widt
int REFRESH_CPS = 50;
int min_global = 0;
int max_global = 0;


ESP32PWM pwm;

void setup() {
  // put your setup code here, to run once:
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  setPeriodHertz(50);
  attach(27, 600, 2300);

}

void loop() {
  // put your main code here, to run repeatedly:
  for (int pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    write(pos);    // tell servo to go to position in variable 'pos'
    delay(15);             // waits 15ms for the servo to reach the position
  }
  for (int pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    write(pos);    // tell servo to go to position in variable 'pos'
    delay(15);             // waits 15ms for the servo to reach the position
  }
}

void write(int value)
{
  // treat values less than MIN_PULSE_WIDTH (500) as angles in degrees (valid values in microseconds are handled as microseconds)
  if (value < MIN_PULSE_WIDTH)
  {
    if (value < 0)
      value = 0;
    else if (value > 180)
      value = 180;

    value = map(value, 0, 180, min_global, max_global);
  }
  writeMicroseconds(value);
}

void writeMicroseconds(int value)
{
  // calculate and store the values for the given channel
  if (value < min_global)          // ensure pulse width is valid
    value = min_global;
  else if (value > max_global)
    value = max_global;

  value = usToTicks(value);  // convert to ticks
  ticks = value;
  // do the actual write
  pwm.write( ticks);
}

int usToTicks(int usec)
{
  return (int)((double)usec / ((double)REFRESH_USEC / (double)timer_width_ticks) * (((double)REFRESH_CPS) / 50.0));
}

uint8_t setPeriodHertz(uint8_t hertz)
{
  REFRESH_CPS = hertz;
  setTimerWidth(timer_width);

  return 1;
}

int attach(int pin, int min, int max)
{
  ticks = DEFAULT_PULSE_WIDTH_TICKS;
  timer_width = DEFAULT_TIMER_WIDTH;
  timer_width_ticks = pow(2, timer_width);

  if (min < MIN_PULSE_WIDTH)
    min = MIN_PULSE_WIDTH;
  if (max > MAX_PULSE_WIDTH)
    max = MAX_PULSE_WIDTH;

  min_global = min;
  max_global = max;

  pwm.attachPin(pin, REFRESH_CPS, timer_width);

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
