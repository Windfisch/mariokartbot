/*
 * unbenannt.cxx
 * 
 * Copyright 2012 Unknown <flo@archie>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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


#define FREEBSD
//#define LINUX

#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#ifdef LINUX
#include <linux/input.h>
#include <linux/uinput.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include "util.h"
#include "steer_interface.h"
#include "naive_steerer.h"
#include "road_thresholder_iface.h"
#include "road_thresholder.h"

#include <iostream>
#include <list>
#include <xcb/xcb.h>
#include <assert.h>

#include <opencv2/opencv.hpp>

using namespace std;
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

XorgGrabber::XorgGrabber(const char* win_title)
{
	conn=xcb_connect(NULL,NULL);
	
	bool found_win=false;
	grab_screen=NULL;
	img=NULL;
	
	/* Find configured screen */
	const xcb_setup_t* setup = xcb_get_setup(conn);
	for (xcb_screen_iterator_t i = xcb_setup_roots_iterator(setup);
		 i.rem > 0; xcb_screen_next (&i))
	{
		xcb_screen_t* scr = i.data;
		xcb_query_tree_reply_t* reply = xcb_query_tree_reply( conn, xcb_query_tree(conn, scr->root), NULL);
		if (reply)
		{
			int len = xcb_query_tree_children_length(reply);
			xcb_window_t* children = xcb_query_tree_children(reply);
			xcb_get_window_attributes_cookie_t* cookies = new xcb_get_window_attributes_cookie_t[len];
			for (int i=0; i<len; ++i)
				cookies[i]=xcb_get_window_attributes(conn, children[i]);
			
			for (int i=0; i<len; ++i)
			{
				xcb_get_window_attributes_reply_t* attr =
					xcb_get_window_attributes_reply (conn, cookies[i], NULL);
				
				
				xcb_get_property_reply_t* title_reply = xcb_get_property_reply(conn, 
					xcb_get_property(conn, false, children[i], XCB_ATOM_WM_NAME, XCB_GET_PROPERTY_TYPE_ANY, 0, 128),
					NULL );
				
				if (!attr->override_redirect && attr->map_state == XCB_MAP_STATE_VIEWABLE)
				{
					char* title=(char*)(title_reply+1);
					cout << title << endl;
					if (strstr(title, win_title))
					{
						xcb_get_geometry_reply_t* geo;
						geo = xcb_get_geometry_reply (conn, xcb_get_geometry (conn, children[i]), NULL);
						if (geo)
						{
							grab_width=geo->width;
							grab_height=geo->height;
							
							free(geo);

							grabbed_win=children[i];
							found_win=true;

							grab_screen=scr;						
						}
						else
						{
							cerr << "geo==NULL!" << endl;
						}
					}
				}

				free(title_reply);
				free(attr);
			}
			
			free(reply);
			delete[] cookies;
		}
		else
		{
			cout << "xcb_get_setup failed" << endl;
		}
	}
	
	if (found_win)
	{
		xcb_get_image_reply_t* img = xcb_get_image_reply (conn,
			xcb_get_image (conn, XCB_IMAGE_FORMAT_Z_PIXMAP, grabbed_win,
				0, 0, grab_width, grab_height, ~0), NULL);
		


		xcb_depth_iterator_t depth_iterator = xcb_screen_allowed_depths_iterator(grab_screen);
		
		int ndepths=xcb_screen_allowed_depths_length(grab_screen);
		
		for (int i=0; i<ndepths; ++i)
		{
			if (depth_iterator.data->depth == img->depth)
			{
				xcb_visualtype_t* visuals = xcb_depth_visuals(depth_iterator.data);
				int nvisuals = xcb_depth_visuals_length(depth_iterator.data);
				
				for (int j=0;j<nvisuals;j++)
				{
					if (visuals[j].visual_id==img->visual)
					{
						assert(visuals[j]._class==XCB_VISUAL_CLASS_TRUE_COLOR || visuals[j]._class==XCB_VISUAL_CLASS_DIRECT_COLOR);
						cout << (int)visuals[j]._class << "," << XCB_VISUAL_CLASS_TRUE_COLOR << endl;
						cout << visuals[j].red_mask << endl;
						cout << visuals[j].green_mask << endl;
						cout << visuals[j].blue_mask << endl;
						break;
					}
				}
				
				break;
			}
			
			xcb_depth_next(&depth_iterator);
		}
		
		
		
		int nformats = xcb_setup_pixmap_formats_length(setup);
		xcb_format_t* formats = xcb_setup_pixmap_formats(setup);
		for (int i=0;i<nformats;i++)
		{
			if (formats[i].depth==img->depth)
			{
				cout << (int)formats[i].bits_per_pixel << endl;
				cout << (int)formats[i].scanline_pad << endl;
				break;
			}
		}
		
		cout << grab_width << "x" << grab_height << endl;
		
		free(img);
		
	}
	else
	{
		throw string("FATAL: did not find window, exiting.");
	}
	
	
}

XorgGrabber::~XorgGrabber()
{
	if (img) free(img);
	xcb_disconnect(conn);
}

void XorgGrabber::read(Mat& mat)
{
	if (img) free(img);


	// mat gets invalid when the next read() is called!
	img = xcb_get_image_reply (conn,
	xcb_get_image (conn, XCB_IMAGE_FORMAT_Z_PIXMAP, grabbed_win,
		0, 0, grab_width, grab_height, ~0), NULL);
	
	mat = Mat(grab_height, grab_width, CV_8UC4, xcb_get_image_data(img));
	mat.addref();
}




#define THROTTLE_CNT_MAX 10


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


char* pack(const BUTTONS* buttons, char* buf)
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

#ifdef FREEBSD
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
#endif
#ifdef LINUX
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

#endif


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



#define HIST_SMOOTH 7

//#define NO_BRIGHTNESS // will man nicht, nur zu demonstrationszwecken

#define ERODE_RADIUS_2D 4


#define IMG_HISTORY 3





int crosshair_x=0, crosshair_y=0;

void mouse_callback(int event, int x, int y, int flags, void* userdata)
{
	if (event==EVENT_LBUTTONDOWN)
	{
		crosshair_x=x;
		crosshair_y=y;
	}
}





sem_t thread1_go;
sem_t thread1_done;
Mat thread1_img;

void* thread1_func(void*)
{
	Mat gray;
	static Mat gray_prev;

	static std::vector<cv::Point2f> points[2];
   
	std::vector<uchar> status; // status of tracked features
	std::vector<float> err;     // error in tracking

	
	cout << "thread 1 is alive :)" <<endl;
	while(1)
	{
		sem_wait(&thread1_go);
		sem_post(&thread1_done);
		
		sem_wait(&thread1_go);
		// now we have our private image at thread1_img.
/*		
		cvtColor(thread1_img, gray, CV_BGR2GRAY);
		
		
		if (points[0].size() <= 2000) // we need more points
		{
			std::vector<cv::Point2f> features; // detected features
			
			// detect the features
			goodFeaturesToTrack(gray, // the image
								features,   // the output detected features
								3000, // the maximum number of features
								0.2,     // quality level
								10);   // min distance between two features


			// add the detected features to
			// the currently tracked features
			points[0].insert(points[0].end(), features.begin(),features.end());
		}
		
		
		// for first image of the sequence
		if(gray_prev.empty())
			gray.copyTo(gray_prev);

		cv::calcOpticalFlowPyrLK(
			gray_prev, gray, // 2 consecutive images
			points[0], // input point positions in first image
			points[1], // output point positions in the 2nd image
			status,    // tracking success
			err);      // tracking error
		


		int k=0;
		for(int i=0; i < points[1].size(); i++) {
			// do we keep this point?
			if (status[i]) {
				// keep this point in vector
				points[0][k] = points[0][i];
				points[1][k++] = points[1][i];
			}
		}
		// eliminate unsuccesful points
		points[0].resize(k);
		points[1].resize(k);



		// for all tracked points
		for(int i= 0; i < points[1].size(); i++ ) {
			// draw line and circle
			cv::line(thread1_img,
					points[0][i], // initial position
					points[1][i],// new position
					cv::Scalar(255,255,255));
			
			cv::circle(thread1_img, points[1][i], 2,
					cv::Scalar(255,255,255),-1);
		}

		// 4. extrapolate movement
		for (int i=0;i<points[0].size();i++)
		{
			points[0][i].x = points[1][i].x+(points[1][i].x-points[0][i].x);
			points[0][i].y = points[1][i].y+(points[1][i].y-points[0][i].y);
		}

		cv::swap(gray_prev, gray);

		*/
		// now we must stop accessing thread1_img, main() may access it
		sem_post(&thread1_done);
	}
}


int main(int argc, char* argv[])
{
try {
  
  
  if (sem_init(&thread1_go, 0, 0)) throw string("sem_init failed");
  if (sem_init(&thread1_done, 0, 0)) throw string("sem_init failed");
  
  pthread_t thread1;
  if (pthread_create(&thread1, NULL, thread1_func, NULL)) throw string("pthread_create failed");
  
  
  string tmp;

#ifdef LINUX
	Joystick joystick;
	cout << "joystick initalized, now starting mupen." << endl;
#endif
  
  if (fork()==0) { system("mupen64plus --nogui --noask ~/MarioKart64.rom"); exit(0); }

#ifdef FREEBSD
  sleep(2);
  Joystick joystick;
#endif

  sleep(1);
  
  joystick.reset();
  cout << "successfully reset joystick." << endl;

/*  cout << "press enter to steer left" << endl;
  getchar();
  
  
  cout << "now steering left. press enter to continue." << endl;
  joystick.steer(-1.0);
  getchar();
  
  cout << "centered. press enter to steer right." << endl;
  joystick.steer(0.0);
  getchar();
  
  cout << "steering right" << endl;
  joystick.steer(1.0);
  getchar();

  cout << "centered. press enter to press a." << endl;
  joystick.steer(0.0);
  getchar();
  
  cout << "pressing A" << endl;
  joystick.press_a(true);
  getchar();
  
  cout << "A released." << endl;
  joystick.press_a(false);
  getchar();*/
  
  cout << "waiting for game to start, press enter when started." << endl;
  joystick.press_a(false);
  joystick.reset();
  getchar();
joystick.reset();

#ifdef LINUX
  XorgGrabber capture("glN64");
#endif
#ifdef FREEBSD
  XorgGrabber capture("Mupen64Plus OpenGL Video");
#endif

  Mat transform;
  
  bool first=true;
  int xlen, ylen;
  
  Mat erode_2d_small=circle_mat(3);
  Mat erode_2d_big=circle_mat(10);
  
  #define trans_width 600
  #define trans_height 400
  #define road_width 100
  
  namedWindow("edit");
  setMouseCallback("edit", mouse_callback, NULL);
  
  joystick.throttle(0.5);
  
  
  VideoWriter img_writer, thres_writer, thres2_writer;
  
	Mat img_;

	capture.read(img_);

	xlen=img_.cols;
	ylen=img_.rows;

	crosshair_x=xlen/2;
	crosshair_y=0.58*ylen;

	Point2f src_pts[4] =  { Point2f(0, ylen), Point2f(xlen, ylen),        Point2f(xlen* (.5 - 0.13), ylen/2), Point2f(xlen* (.5 + .13), ylen/2) };
	//Point2f dest_pts[4] = { Point2f(0, ylen), Point2f(trans_width, ylen), Point2f(0, trans_height),       Point2f(0, trans_height) };
	Point2f dest_pts[4] = { Point2f(trans_width/2 - road_width/2, trans_height), Point2f(trans_width/2 + road_width/2, trans_height),        Point2f(trans_width/2 - road_width/2, trans_height/2), Point2f(trans_width/2 + road_width/2, trans_height/2) };
	transform=getPerspectiveTransform(src_pts, dest_pts);

	//img_writer.open("img.mpg", CV_FOURCC('P','I','M','1'), 30, Size(xlen,ylen), false);
	img_writer.open("img.mpg", CV_FOURCC('P','I','M','1'), 30, Size(xlen,ylen), false);
	thres_writer.open("thres.mpg", CV_FOURCC('P','I','M','1'), 30, Size(xlen,ylen), false);
	thres2_writer.open("thres2.mpg", CV_FOURCC('P','I','M','1'), 30, Size(xlen,ylen), false);

	if (!img_writer.isOpened() || !thres_writer.isOpened() || !thres2_writer.isOpened())
	{
		cout << "ERROR: could not open video files!" << !img_writer.isOpened() << !thres_writer.isOpened() << !thres2_writer.isOpened() <<endl;
	}

  
  SteerIface* steerer = new NaiveSteerer(xlen/2, 0.58*ylen);
  RoadThresholderIface* road_thresholder = new RoadThresholder();
  
  while(1)
  {
    capture.read(img_);
  
  assert ((img_.cols==xlen) && (img_.rows==ylen));
    
  Mat img;
  img_.convertTo(img, CV_8UC3, 1); //FINDMICH
  img.copyTo(thread1_img); sem_post(&thread1_go);
  
  #ifdef NO_BRIGHTNESS
  //assert(img.type()==CV_8UC3);
  for (int row = 0; row<img.rows; row++)
  {
    uchar* data=img.ptr<uchar>(row);
    
    for (int col=0; col<img.cols;col++)
    {
      int sum=data[0] + data[1] + data[2];
      if (sum>0)
      {
        data[0]=(int)data[0] * 256 / sum;
        data[1]=(int)data[1] * 256 / sum;
        data[2]=(int)data[2] * 256 / sum;
      }
      else
      {
        data[0]=255/3;
        data[1]=255/3;
        data[2]=255/3;
      }
      data+=img.step[1];
    }
  }
  #endif
  
  
  road_thresholder->process_image(img);
  Mat img_thres = road_thresholder->get_road();
  
  Mat img_eroded(img_thres.rows, img_thres.cols, img_thres.type());
  Mat img_tmp(img_thres.rows, img_thres.cols, img_thres.type());
  Mat img_thres2(img_thres.rows, img_thres.cols, img_thres.type());
  
  erode(img_thres, img_tmp, erode_2d_small);
  dilate(img_tmp, img_thres, erode_2d_small);
  dilate(img_thres, img_tmp, erode_2d_big);
  erode(img_tmp, img_thres2, erode_2d_big);









  
  assert(img.rows==img_eroded.rows);
  assert(img.cols==img_eroded.cols);

    
  


/*
  Mat img_perspective(trans_height, trans_width, img_thres.type());
  warpPerspective(img_thres, img_perspective, transform, img_perspective.size());


  threshold(img_perspective, img_perspective, 127, 255, THRESH_BINARY);
  Mat img_perspective_temp(img_perspective.rows, img_perspective.cols, img_perspective.type());
  Mat img_perspective_temp2(img_perspective.rows, img_perspective.cols, img_perspective.type());
  erode(img_perspective, img_perspective_temp, circle_mat(7));
  dilate(img_perspective_temp, img_perspective_temp2, circle_mat(7 + 15));
  erode(img_perspective_temp2, img_perspective, circle_mat(15));
  */

  
  img.row(crosshair_y)=Scalar(255,0,0);
  img.col(crosshair_x)=Scalar(255,0,0);
  img_thres2.row(crosshair_y)=128;
  img_thres2.col(crosshair_x)=128;

	steerer->process_image(img_thres2);
	double steer_value = steerer->get_steer_data();

  Mat steer=Mat::zeros(20,1920,CV_8U);
  steer.col( steer.cols /2 )=128;
  if (steerer->get_confidence()>0)
  {
	  int x = (steer_value/2.0 + 0.5)*steer.cols;
	  
	  if (x<1) x=1;
	  if (x>=steer.cols-1) x=steer.cols-2;
	  
	  steer.col(x) = 255;
	  steer.col(x-1)=240;
	  steer.col(x+1)=240;
	  
	  
	  joystick.steer( steer_value,0.05);
  }
  else
    joystick.steer(0.0);




  sem_wait(&thread1_done); // wait for thread1 to finish


  //imshow("orig", img);
  imshow("edit", img);
  //imshow("perspective", img_perspective);
  //imshow("diff", img_diff);
  
  imshow("thres", img_thres);
  imshow("thres2", img_thres2);
  imshow("tracked", thread1_img);
  imshow("steer", steer);
  


  //img_writer << img_diff;
  thres_writer << img_thres;
  thres2_writer << img_thres2;

  waitKey(1000/50);  
  joystick.process();
}

} //try
catch(string meh)
{
	cout << "error: "<<meh<< endl;
}
catch(...)
{
	cout << "error!" << endl;
}

sem_destroy(&thread1_go);
sem_destroy(&thread1_done);

}
