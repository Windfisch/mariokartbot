/*
 * joystick.h
 * 
 * Copyright 2012 Florian Jung <florian.a.jung@web.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#include "os.h"

#ifdef JOYSTICK_PATCHEDINPUTPLUGIN
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

#endif // JOYSTICK_PATCHEDINPUTPLUGIN


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
#ifdef JOYSTICK_PATCHEDINPUTPLUGIN
		BUTTONS buttons;
		void send_data();
		int fifo_fd;
#endif
#ifdef JOYSTICK_UINPUT
		int fd;
#endif

		float throt;
		int throttle_cnt;
		
};

#endif // __JOYSTICK_H__
