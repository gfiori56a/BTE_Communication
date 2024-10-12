// last edit: 2024-08-13 version 1.1

#ifndef _AFLIB_
#define _AFLIB_
#include <Arduino.h>

class FTIMER {
  private:
  long tmr_start;
  long tmr_end;
  bool tmr_done;

  public:
    FTIMER();
    void start(long ms);
    bool done();
};

class TDELTA {
  private:
  long tmr_latch;

  public:
    TDELTA();
    void begin();
    long end();
};


class HC_SR04 {
  private:
    // configuration
    int trigPin_OUT;
    int echoPin_IN;
    int polling_ms;
    int thr_distance;
    int max_distance;

    // work
    int init_done;
    int distance;
    int distance_cpy;
    TDELTA dt;

  public:
    HC_SR04();
    bool setup(int outPin, int inPin);
    bool setPolling(int msPoll);
    bool setMaxDistance(int value);
    bool setThrDistance(int value);
    bool read(int * distance);

};

class MATRIX_RAM {
  private:
    // work
    uint32_t LED_RAM[3];
    uint32_t LED_CLEAR[3];

    const uint32_t LED_MASK1[3] = {
      0xf00f00f0,
      0xf00f00f,
      0xf00f00
    };
    const uint32_t LED_MASK2[3] = {
      0xf00f00f,
      0xf00f00,
      0xf00f00f0
    };
    const uint32_t LED_MASK3[3] = {
      0xf00f00,
      0xf00f00f0,
      0xf00f00f
    };
    const uint32_t LED_INV[3] = {
      0xa51b51ac,
      0xaa4aa440,
      0x0
    };
    const uint32_t LED_3x0[3] = {
      0x77755555,
      0x55557770,
      0x0
    };
    const uint32_t LED_3x1[3] = {
      0x11133311,
      0x11111110,
      0x0
    };
    const uint32_t LED_3x2[3] = {
      0x77711177,
      0x74447770,
      0x0
    };
    const uint32_t LED_3x3[3] = {
      0x77711177,
      0x71117770,
      0x0
    };
    const uint32_t LED_3x4[3] = {
      0x44455577,
      0x71111110,
      0x0
    };
    const uint32_t LED_3x5[3] = {
      0x77744477,
      0x71117770,
      0x0
    };
    const uint32_t LED_3x6[3] = {
      0x77744477,
      0x75557770,
      0x0
    };
    const uint32_t LED_3x7[3] = {
      0x77711111,
      0x11111110,
      0x0
    };
    const uint32_t LED_3x8[3] = {
      0x77755577,
      0x75557770,
      0x0
    };
    const uint32_t LED_3x9[3] = {
      0x77755577,
      0x71117770,
      0x0
    };

    const uint32_t LED_SEQ[32] = {
	    0x80000000,  // 31
	    0x40000000,  // 30
	    0x20000000,  // 29
	    0x10000000,  // 28
	    0x08000000,  // 27
	    0x04000000,  // 26
	    0x02000000,  // 25
	    0x01000000,  // 24
	    0x00800000,  // 23
	    0x00400000,  // 22
	    0x00200000,  // 21
	    0x00100000,  // 20
	    0x00080000,  // 19
	    0x00040000,  // 18
	    0x00020000,  // 17
	    0x00010000,  // 16
	    0x00008000,  // 15
	    0x00004000,  // 14
	    0x00002000,  // 13
	    0x00001000,  // 12
	    0x00000800,  // 11
	    0x00000400,  // 10
	    0x00000200,  // 9
	    0x00000100,  // 8
	    0x00000080,  // 7
	    0x00000040,  // 6
	    0x00000020,  // 5
	    0x00000010,  // 4
	    0x00000008,  // 3
	    0x00000004,  // 2
	    0x00000002,  // 1
	    0x00000001,  // 0
    };

    uint32_t * getPattern(char c);
    void set_number_core(uint32_t * pattern1, uint32_t * pattern2, uint32_t * pattern3); 
    uint32_t n2bar(int value, int max);

  public:
    MATRIX_RAM();
    uint32_t * getRam();
    void setNumber(int n, int max);
    void fillPoints(int n);
    void updown(uint32_t * pattern);
    void writeRam(uint32_t * pattern);
 };
#endif
