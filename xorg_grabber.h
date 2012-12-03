#ifndef __XORG_GRABBER_H__
#define __XORG_GRABBER_H__

#include <xcb/xcb.h>
#include <opencv2/opencv.hpp>

using namespace cv;

class XorgGrabber
{
	public:
		XorgGrabber(const char* win_title);
		~XorgGrabber();
		void read(Mat& mat);
	
	private:
		xcb_connection_t* conn;
		xcb_window_t grabbed_win;
		int grab_width, grab_height;
		xcb_screen_t* grab_screen;
		xcb_get_image_reply_t* img;

};

#endif
