/*
 * joystick.cpp
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

#include "joystick.h"
#include "os.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>
#include <iostream>
#include <fcntl.h>

#ifdef JOYSTICK_UINPUT
#include <linux/input.h>
#include <linux/uinput.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

using namespace std;

#define THROTTLE_CNT_MAX 10

#ifdef JOYSTICK_PATCHEDINPUTPLUGIN
static char* pack(const BUTTONS* buttons, char* buf)
{
	buf[0]= (buttons->A_BUTTON     ?  1 : 0) +
	        (buttons->B_BUTTON     ?  2 : 0) +
	        (buttons->L_TRIG       ?  4 : 0) +
	        (buttons->R_TRIG       ?  8 : 0) +
	        (buttons->Z_TRIG       ? 16 : 0) +
	        (buttons->START_BUTTON ? 32 : 0) +
	                                128;
	                                 
	buf[1]= (buttons->R_DPAD ?  1 : 0) +
	        (buttons->L_DPAD ?  2 : 0) +
	        (buttons->U_DPAD ?  4 : 0) +
	        (buttons->D_DPAD ?  8 : 0) +
	        ((buttons->X_AXIS & 128) ? 16 : 0) +
	        ((buttons->Y_AXIS & 128) ? 32 : 0);
	        
	buf[2]= (buttons->R_CBUTTON ?  1 : 0) +
	        (buttons->L_CBUTTON ?  2 : 0) +
	        (buttons->U_CBUTTON ?  4 : 0) +
	        (buttons->D_CBUTTON ?  8 : 0);
	        
	buf[3]= (buttons->X_AXIS & 127);
	
	buf[4]= (buttons->Y_AXIS & 127);
	
	return buf;
}
#endif



#ifdef JOYSTICK_PATCHEDINPUTPLUGIN
Joystick::Joystick()
{
	if ((fifo_fd=open("/var/tmp/mupen64plus_ctl", O_WRONLY )) == -1) {throw string(strerror(errno));}
	cout << "opened" << endl;
	if (fcntl(fifo_fd, F_SETFL, O_NONBLOCK) == -1) throw string("failed to set nonblocking io");
	
	reset();
}

void Joystick::press_a(bool state)
{
	buttons.A_BUTTON=state;
	send_data();
}

void Joystick::send_data()
{
	char buf[5];
	pack(&buttons, buf);
	write(fifo_fd, buf, 5);
}



void Joystick::steer(float dir, float dead_zone)
{
	if (dir<-1.0) dir=-1.0;
	if (dir>1.0) dir=1.0; 
	
	if (fabs(dir)<dead_zone) dir=0.0;
	
	buttons.X_AXIS = (signed) (127.0*dir);
	send_data();
}

void Joystick::reset()
{
	memset(&buttons, sizeof(buttons), 0);
	buttons.Z_TRIG=0;
	buttons.R_TRIG=0;
	buttons.L_TRIG=0;
	buttons.A_BUTTON=0;
	buttons.B_BUTTON=0;
	send_data();
}

Joystick::~Joystick()
{
	close(fifo_fd);
}
#endif // JOYSTICK_PATCHEDINPUTPLUGIN
#ifdef JOYSTICK_UINPUT
Joystick::Joystick()
{
	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
		cerr << "open uinput failed. do you have privilegies to access it? (try chown flo:root /dev/uinput)" << endl;
		exit(EXIT_FAILURE);
	}
	
	int ret;
	
	ret=ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ret=ioctl(fd, UI_SET_EVBIT, EV_SYN);
	ret=ioctl(fd, UI_SET_KEYBIT	, BTN_A);
	ioctl(fd, UI_SET_EVBIT, EV_ABS);
	ioctl(fd, UI_SET_ABSBIT, ABS_X);
	ioctl(fd, UI_SET_ABSBIT, ABS_Y);
	
	struct uinput_user_dev meh;
	memset(&meh,0,sizeof(meh));
	
	strcpy(meh.name, "flotest");
	meh.id.bustype=BUS_USB;
	meh.id.vendor=0xdead;
	meh.id.product=0xbeef;
	meh.id.version=1;
	meh.absmin[ABS_X]=0;
	meh.absmax[ABS_X]=10000;
	meh.absmin[ABS_Y]=0;
	meh.absmax[ABS_Y]=10000;
	
	
	ret=write(fd, &meh, sizeof(meh));
	
	ioctl(fd,UI_DEV_CREATE);
	
	reset();
}

Joystick::~Joystick()
{
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
}

void Joystick::steer(float dir, float dead_zone)
{
	if (dir<-1.0) dir=-1.0;
	if (dir>1.0) dir=1.0; 
	
	if (fabs(dir)<dead_zone) dir=0.0;
	
	struct input_event ev;
	ev.type=EV_ABS;
	ev.code=ABS_X;
	ev.value=5000+dir*5000;
	write(fd, &ev, sizeof(ev));

	cout << "steering" << dir << endl;
}


void Joystick::press_a(bool a)
{
	struct input_event ev;
	ev.type=EV_KEY;
	ev.code=BTN_A;
	ev.value =  a ? 1 : 0;
	write(fd, &ev, sizeof(ev));
}

void Joystick::reset()
{
	struct input_event ev;
	ev.type=EV_ABS;
	ev.code=ABS_Y;
	ev.value=5001;
	write(fd, &ev, sizeof(ev));
	ev.value=5000;
	write(fd, &ev, sizeof(ev));
	
	cout << "Y zeroed" << endl;
	
	steer(0.1);
	steer(0);
	cout << "X zeroed" << endl;
	
	press_a(true);
	press_a(false);
	cout << "A zeroed" << endl;
}

#endif // JOYSTICK_UINPUT


void Joystick::throttle(float t)
{
	if (t<0.0) t=0.0;
	if (t>1.0) t=1.0;
	
	throt=t;
}

void Joystick::process()
{
	throttle_cnt++;
	if (throttle_cnt>=THROTTLE_CNT_MAX) throttle_cnt=0;
	
	press_a((throttle_cnt < throt*THROTTLE_CNT_MAX));
}

