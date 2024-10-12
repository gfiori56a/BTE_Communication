// last edit: 2024-08-13 version 1.1

#include "AFlib.h"


//==================== CLASS FTIMER =========================

#define TIME_MASK 0x07fffff

FTIMER::FTIMER()
{
    tmr_start = 0;
    tmr_end = 0;
    tmr_done = true;
}
void FTIMER::start(long ms)
{
  tmr_start = millis() & TIME_MASK;
  tmr_end   = (tmr_start + ms) & TIME_MASK;
  tmr_done  = false;
}
bool FTIMER::done()
{
  if (tmr_done == true)
    return true;

  long now = millis() & TIME_MASK;
  if(tmr_start <= tmr_end) 
  {
      if(now < tmr_start || now >= tmr_end)
      {
        tmr_done = true;
      }
  } else {
      if(now > tmr_end && now < tmr_start) {
        tmr_done = true;
      }
  }
  return tmr_done;
}

//==================== CLASS TDELTA =========================

TDELTA::TDELTA()
{
  tmr_latch = 0;
}
void TDELTA::begin()
{
  tmr_latch = millis() & TIME_MASK;
}
long TDELTA::end()
{
  long tmr_now = millis() & TIME_MASK;
  long delta = tmr_now - tmr_latch;
  if(delta < 0) { 
    delta += TIME_MASK + 1;    
  }
  return delta;
}

//==================== CLASS HC_SR04 =========================

#define KAPPA                60.3865
#define MAX_DISTANCE_DEFAULT 500

HC_SR04::HC_SR04() 
{
  init_done = 0;
  polling_ms = 0;
  thr_distance = 0;  
  max_distance = MAX_DISTANCE_DEFAULT;

  distance = 0;
  distance_cpy = 0;
}
bool HC_SR04::setup(int outPin, int inPin) 
{
   if(init_done > 0 || outPin <= 0 || inPin <= 0)
     return false;

   trigPin_OUT = outPin;
   echoPin_IN = inPin;

   pinMode(trigPin_OUT, OUTPUT);
   pinMode(echoPin_IN, INPUT);
   dt.begin();
   init_done = 1;
   return true;
}
bool HC_SR04::setPolling(int msPoll)
{
  polling_ms = msPoll;
  if(polling_ms < 0)
   polling_ms = 0;
  return true;
}

bool HC_SR04::setThrDistance(int value)
{
  thr_distance = value;
  return true;
}

bool HC_SR04::setMaxDistance(int value)
{
  max_distance = value;
  if(max_distance < MAX_DISTANCE_DEFAULT)
    max_distance = MAX_DISTANCE_DEFAULT;
  return true;
}
bool HC_SR04::read(int * pDistance)
{
   if(!init_done) {
     * pDistance = distance;
     return false;
   }
   if(init_done == 1) {
   }
   long duration;

   if(dt.end() < polling_ms && init_done == 2)
   {
   // confirm previous value and wait for next acquisition
     * pDistance = distance;
      return false;
   }

   // Clear the trigPin by setting it LOW:
   digitalWrite(trigPin_OUT, LOW);
   delayMicroseconds(5);

   // Trigger the sensor by setting the trigPin high for 10 microseconds:
   digitalWrite(trigPin_OUT, HIGH);
   delayMicroseconds(10);
   digitalWrite(trigPin_OUT, LOW);

   // Read the echoPin, pulseIn() returns the duration (length of the pulse) in microseconds:
   unsigned long tout = max_distance * KAPPA; 
   duration = pulseIn(echoPin_IN, HIGH, tout);
   if(duration <= 0) {
     // work around:
     * pDistance = distance;
     return false;
   }
   dt.begin();

   // Calculate the distance:
   distance = duration / KAPPA;

   * pDistance = distance;
   if (distance_cpy != distance || init_done == 1) {
     init_done = 2;
     if(abs(distance_cpy - distance) >= thr_distance) {
       distance_cpy = distance;
       return true;
     }
   }
   return false;
}
//==================== CLASS MATRIX_RAM =========================

MATRIX_RAM::MATRIX_RAM() 
{
   LED_RAM[0] = 0;
   LED_RAM[1] = 0;
   LED_RAM[2] = 0;
   LED_CLEAR[0] = 0;
   LED_CLEAR[1] = 0;
   LED_CLEAR[2] = 0;
}
uint32_t * MATRIX_RAM::getRam()
{
 return LED_RAM;
}
void MATRIX_RAM::setNumber(int n, int max)
{
  if(n < 0 || n > 999) {
    LED_RAM[0] = LED_INV[0];
    LED_RAM[1] = LED_INV[1];
    LED_RAM[2] = LED_INV[2];
    return;
  }
  uint32_t * pattern1;
  uint32_t * pattern2;
  uint32_t * pattern3;
  char buf[10];

  sprintf(buf, "%03d", n);
  pattern1 = getPattern(buf[0]);
  pattern2 = getPattern(buf[1]);
  pattern3 = getPattern(buf[2]);
  set_number_core(pattern1, pattern2, pattern3);
  LED_RAM[2] |= n2bar(n, max);
} 
uint32_t * MATRIX_RAM::getPattern(char c) 
{
  switch(c) {
    case '0':
      return (uint32_t *) LED_3x0;
      break;
    case '1':
      return (uint32_t *) LED_3x1;
      break;
    case '2':
      return (uint32_t *) LED_3x2;
      break;
    case '3':
      return (uint32_t *) LED_3x3;
      break;
    case '4':
      return (uint32_t *) LED_3x4;
      break;
    case '5':
      return (uint32_t *) LED_3x5;
      break;
    case '6':
      return (uint32_t *) LED_3x6;
      break;
    case '7':
      return (uint32_t *) LED_3x7;
      break;
    case '8':
      return (uint32_t *) LED_3x8;
      break;
    case '9':
      return (uint32_t *) LED_3x9;
      break;
    default:
      break;
  }
  return LED_CLEAR;
}
void MATRIX_RAM::set_number_core(uint32_t * pattern1, uint32_t * pattern2, uint32_t * pattern3) 
{
  LED_RAM[0] = LED_MASK1[0] & pattern1[0];
  LED_RAM[1] = LED_MASK1[1] & pattern1[1];
  LED_RAM[2] = LED_MASK1[2] & pattern1[2];

  LED_RAM[0] |= LED_MASK2[0] & pattern2[0];
  LED_RAM[1] |= LED_MASK2[1] & pattern2[1];
  LED_RAM[2] |= LED_MASK2[2] & pattern2[2];

  LED_RAM[0] |= LED_MASK3[0] & pattern3[0];
  LED_RAM[1] |= LED_MASK3[1] & pattern3[1];
  LED_RAM[2] |= LED_MASK3[2] & pattern3[2];
}

#define UNO        0x00800000
#define FULL       0x00fff000
#define R12        12
#define MAX_POINTS 96
uint32_t MATRIX_RAM::n2bar(int value, int max)
{
    if(max <= 0 || value <= 0)
      return 0;
    if(value >= max)
      return FULL;

    value =  value * R12 / max;
    int a = UNO;   
    for (int i = 0; i < value - 1; i++)
    {
       a = a >> 1;
       a |= UNO;
    }
    return a;
}
void MATRIX_RAM::fillPoints(int n) {

  if(n >= MAX_POINTS) {
    for(int j = 0; j < 3; j++) {
      LED_RAM[j] = 0xffffffff;
    }
    return;  
  }
  
  for(int j = 0; j < 3; j++) {
    LED_RAM[j] = 0;
  }

  if(n <= 0)
     return;
  
  int cnt_row = 0;
  int cnt_bit = 0;
  for(int i = 0; i < n; i++) {
    LED_RAM[cnt_row] |= LED_SEQ[cnt_bit];
    cnt_bit++;
    if(cnt_bit >= 32) { cnt_bit = 0; cnt_row++; }
  }
}
void MATRIX_RAM::updown(uint32_t * pattern)
{
  uint32_t tmp[3];  
  tmp[0] = 0;
  tmp[1] = 0;
  tmp[2] = 0;

  int cnt_row = 0;
  int cnt_bit = 0;
  int inv_cnt_row = 2;
  int inv_cnt_bit = 31;

  for(int i = 0; i < 96; i++) {
    if(pattern[cnt_row] & LED_SEQ[cnt_bit])
      tmp[inv_cnt_row] |= LED_SEQ[inv_cnt_bit];
    cnt_bit++;
    inv_cnt_bit--;
    if(cnt_bit >= 32) { 
      cnt_bit = 0; 
      cnt_row++; 
      inv_cnt_bit = 31; 
      inv_cnt_row--; 
    }
  }
  LED_RAM[0] = tmp[0];
  LED_RAM[1] = tmp[1];
  LED_RAM[2] = tmp[2];
}
void MATRIX_RAM::writeRam(uint32_t * pattern)
{
  LED_RAM[0] = pattern[0];
  LED_RAM[1] = pattern[1];
  LED_RAM[2] = pattern[2];
}


