#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "os.h"

#ifdef FREEBSD
typedef union {
    unsigned int Value;
    struct {
        unsigned R_DPAD       : 1;
        unsigned L_DPAD       : 1;
        unsigned D_DPAD       : 1;
        unsigned U_DPAD       : 1;
        unsigned START_BUTTON : 1;
        unsigned Z_TRIG       : 1;
        unsigned B_BUTTON     : 1;
        unsigned A_BUTTON     : 1;

        unsigned R_CBUTTON    : 1;
        unsigned L_CBUTTON    : 1;
        unsigned D_CBUTTON    : 1;
        unsigned U_CBUTTON    : 1;
        unsigned R_TRIG       : 1;
        unsigned L_TRIG       : 1;
        unsigned Reserved1    : 1;
        unsigned Reserved2    : 1;

        signed   X_AXIS       : 8;
        signed   Y_AXIS       : 8;
    };
} BUTTONS;

#endif // FREEBSD


class Joystick
{
	public:
		Joystick();
		~Joystick();
		void steer(float dir, float dead_zone=0.0);
		void throttle(float t);
		void press_a(bool);
		
		
		void process();
		void reset();
	
	private:
#ifdef FREEBSD
		BUTTONS buttons;
		void send_data();
		int fifo_fd;
#endif
#ifdef LINUX
		int fd;
#endif

		float throt;
		int throttle_cnt;
		
};

#endif // __JOYSTICK_H__
