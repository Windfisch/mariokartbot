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



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <iostream>
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




#define THROTTLE_CNT_MAX 20

class Joystick
{
	public:
		Joystick();
		~Joystick();
		
		void steer(float dir);
		void throttle(float t);
		void press_a(bool);
		
		
		void process();
		void reset();
		
	private:
		float throt;
		int throttle_cnt;
		
		int fd;
};

Joystick::Joystick()
{
	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
		cerr << "open failed" << endl;
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

void Joystick::steer(float dir)
{
	if (dir<-1.0) dir=-1.0;
	if (dir>1.0) dir=1.0;
	
	struct input_event ev;
	ev.type=EV_ABS;
	ev.code=ABS_X;
	ev.value=5000+dir*5000;
	write(fd, &ev, sizeof(ev));
}

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



#define HIST_SMOOTH 7

//#define NO_BRIGHTNESS // will man nicht, nur zu demonstrationszwecken

#define ERODE_RADIUS_2D 4



Mat circle_mat(int radius)
{
  Mat result(radius*2+1, radius*2+1, CV_8U);
  for (int x=0; x<=result.cols/2; x++)
    for (int y=0; y<=result.rows/2; y++)
    {
      unsigned char& p1 = result.at<unsigned char>(result.cols/2 + x, result.rows/2 + y);
      unsigned char& p2 = result.at<unsigned char>(result.cols/2 - x, result.rows/2 + y);
      unsigned char& p3 = result.at<unsigned char>(result.cols/2 + x, result.rows/2 - y);
      unsigned char& p4 = result.at<unsigned char>(result.cols/2 - x, result.rows/2 - y);
      
      if ( x*x + y*y < radius*radius )
        p1=p2=p3=p4=255;
      else
        p1=p2=p3=p4=0;
    }
    
  return result;
}



int crosshair_x=0, crosshair_y=0;

void mouse_callback(int event, int x, int y, int flags, void* userdata)
{
	if (event==EVENT_LBUTTONDOWN)
	{
		crosshair_x=x;
		crosshair_y=y;
	}
}


float flopow(float b, float e)
{
	return (b>=0.0) ? (powf(b,e)) : (-powf(-b,e));
}

int main(int argc, char* argv[])
{
  
  string tmp;
  Joystick joystick;

  cout << "joystick initalized, now starting mupen." << endl;
  
  
  if (fork()==0) system("mupen64plus --nogui --noask ~/MarioKart64.rom");
  
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
  getchar();

  XorgGrabber capture("glN64");//("Mupen64Plus OpenGL Video");

  int road_0=77, road_1=77, road_2=77;
  
  Mat transform;
  
  bool first=true;
  int xlen, ylen;
  
  Mat erode_2d_big=circle_mat(10);
  
  #define trans_width 600
  #define trans_height 400
  #define road_width 100
  
  namedWindow("edit");
  setMouseCallback("edit", mouse_callback, NULL);
  
  joystick.throttle(0.5);
  
  while(1)
  {
    
  Mat img_;
  capture.read(img_);
  
  if (first)
  {
    xlen=img_.cols;
    ylen=img_.rows;
    
    Point2f src_pts[4] =  { Point2f(0, ylen), Point2f(xlen, ylen),        Point2f(xlen* (.5 - 0.13), ylen/2), Point2f(xlen* (.5 + .13), ylen/2) };
    //Point2f dest_pts[4] = { Point2f(0, ylen), Point2f(trans_width, ylen), Point2f(0, trans_height),       Point2f(0, trans_height) };
    Point2f dest_pts[4] = { Point2f(trans_width/2 - road_width/2, trans_height), Point2f(trans_width/2 + road_width/2, trans_height),        Point2f(trans_width/2 - road_width/2, trans_height/2), Point2f(trans_width/2 + road_width/2, trans_height/2) };
    transform=getPerspectiveTransform(src_pts, dest_pts);

    
    first=false;
  }
  assert ((img_.cols==xlen) && (img_.rows==ylen));
    
  Mat img, img2;
  img_.convertTo(img, CV_8UC3, 1); //FINDMICH
  img.copyTo(img2);
  
  #ifdef NO_BRIGHTNESS
  //assert(img2.type()==CV_8UC3);
  for (int row = 0; row<img2.rows; row++)
  {
    uchar* data=img2.ptr<uchar>(row);
    
    for (int col=0; col<img2.cols;col++)
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
      data+=img2.step[1];
    }
  }
  #endif
  
  
  Mat img_diff(img2.rows, img2.cols, CV_8U);
  int hist[256];
  for (int i=0; i<256; i++) hist[i]=0;
  for (int row = 0; row<img2.rows; row++)
  {
    uchar* data=img2.ptr<uchar>(row);
    uchar* data_out=img_diff.ptr<uchar>(row);
    
    for (int col=0; col<img2.cols;col++)
    {
      int diff = (abs((int)data[0] - road_0) + abs((int)data[1] - road_1) + abs((int)data[2] - road_2)) /3;
      if (diff>255) { cout << "error, diff is" << diff << endl; diff=255; }
      *data_out=diff;
      hist[diff]++;
      data+=img2.step[1];
      data_out++;
    }
  }
  
  int hist2[256];
  for (int i=0; i<256; i++)
  {
    int sum=0;
    for (int j=-HIST_SMOOTH; j<=HIST_SMOOTH; j++)
    {
      if (i+j < 0 || i+j > 255) continue;
      sum+=hist[i+j];
    }
    hist2[i]=sum;
  }
  
  int cumul=0;
  int x_begin=0;
  for (x_begin=0;x_begin<256;x_begin++)
  {
    cumul+=hist[x_begin];
    if (cumul > img2.rows*img2.cols/100) break;
  }
  
  int hist_max=0;
  int thres;
  for (int i=0; i<256; i++)
  {
    if (hist2[i]>hist_max) hist_max=hist2[i];
    if ((hist2[i] < hist_max/2) && (i>x_begin))
    {
      thres=i;
      break;
    }
  }
  
  //thres-=thres/4;
  
  
  Mat img_hist(100,256, CV_8U);
  for (int row = 0; row<img_hist.rows; row++)
  {
    uchar* data=img_hist.ptr<uchar>(row);
    
    for (int col=0; col<img_hist.cols;col++)
    {
      *data=255;
      if (col==thres) *data=127;
      if (col==x_begin) *data=0;
      if (hist2[col] > (img_hist.rows-row)*800) *data=0;
      data++;
    }
  }
  
  Mat img_thres(img_diff.rows, img_diff.cols, img_diff.type());
  threshold(img_diff, img_thres, thres, 255, THRESH_BINARY_INV);
  
  Mat img_eroded(img_thres.rows, img_thres.cols, img_thres.type());
  Mat img_thres2(img_thres.rows, img_thres.cols, img_thres.type());
  Mat img_stddev(img_thres.rows, img_thres.cols, img_thres.type());
  img_stddev=Mat::zeros(img_thres.rows, img_thres.cols,  img_thres.type());
  
  erode(img_thres, img_eroded, Mat::ones(3, 3, CV_8U));
  dilate(img_eroded, img_thres2, Mat::ones(3, 3, CV_8U));
  dilate(img_thres2, img_eroded, erode_2d_big);
  erode(img_eroded, img_thres2, erode_2d_big);
  
  assert(img.rows==img_eroded.rows);
  assert(img.cols==img_eroded.cols);
  int avg_sum=0;
  int r0=0, r1=0, r2=0;
  int left_sum=0, right_sum=0;
  for (int row = 0; row<img2.rows; row++)
  {
    uchar* data=img2.ptr<uchar>(row);
    uchar* mask=img_eroded.ptr<uchar>(row);
    
    int mean_value_line_sum=0;
    int mean_value_line_cnt=0;
    int mean_value_line;
    for (int col=0; col<img2.cols;col++)
    {
      if (*mask)
      {
		if (row<=crosshair_y)
		{
			if (col < crosshair_x)
				left_sum++;
			else
				right_sum++;
		}
		  
        avg_sum++;
        mean_value_line_sum+=col;
        mean_value_line_cnt++;
        r0+=data[0];
        r1+=data[1];
        r2+=data[2];
      }
      
      data+=img.step[1];
      mask++;
    }
    
    if (mean_value_line_cnt)
    {
		mean_value_line=mean_value_line_sum/mean_value_line_cnt;
		int variance_line=0;
		int stddev_line;
		uchar* mask=img_eroded.ptr<uchar>(row);
		data=img2.ptr<uchar>(row);
		for (int col=0; col<img2.cols;col++)
		{
		  if (*mask)
			variance_line+=(col-mean_value_line)*(col-mean_value_line);
		  
		  data+=img2.step[1];
		  mask++;
		}
		variance_line/=mean_value_line_cnt;
		stddev_line=sqrt(variance_line);
		if (mean_value_line>1 && mean_value_line < img2.cols-2)
		{
			img_stddev.ptr<uchar>(row)[mean_value_line-1]=255;
			img_stddev.ptr<uchar>(row)[mean_value_line]=255;
			img_stddev.ptr<uchar>(row)[mean_value_line+1]=255;
		}
		if ((mean_value_line+stddev_line)>1 && (mean_value_line+stddev_line) < img2.cols-2)
		{
			img_stddev.ptr<uchar>(row)[mean_value_line-1+stddev_line]=255;
			img_stddev.ptr<uchar>(row)[mean_value_line+stddev_line]=255;
			img_stddev.ptr<uchar>(row)[mean_value_line+1+stddev_line]=255;
		}
		if ((mean_value_line-stddev_line)>1 && (mean_value_line-stddev_line) < img2.cols-2)
		{
			img_stddev.ptr<uchar>(row)[mean_value_line-1-stddev_line]=255;
			img_stddev.ptr<uchar>(row)[mean_value_line-stddev_line]=255;
			img_stddev.ptr<uchar>(row)[mean_value_line+1-stddev_line]=255;
		}
	}
  }
  
  if (avg_sum>20)
  {
	  road_0=r0/avg_sum;
	  road_1=r1/avg_sum;
	  road_2=r2/avg_sum;
	}

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

  
  img2.row(crosshair_y)=Scalar(255,0,0);
  img2.col(crosshair_x)=Scalar(255,0,0);
  img_diff.row(crosshair_y)=255;
  img_diff.col(crosshair_x)=255;
  img_thres2.row(crosshair_y)=128;
  img_thres2.col(crosshair_x)=128;
  img_stddev.row(crosshair_y)=255;
  img_stddev.col(crosshair_x)=255;

  Mat steer=Mat::zeros(20,1920,CV_8U);
  steer.col( steer.cols /2 )=128;
  if (left_sum+right_sum>0)
  {
	  int x = (steer.cols-2) * left_sum / (left_sum+right_sum)    +1;
	  
	  if (x<1) x=1;
	  if (x>=steer.cols-1) x=steer.cols-2;
	  
	  steer.col(x) = 255;
	  steer.col(x-1)=240;
	  steer.col(x+1)=240;
	  
	  
	  joystick.steer(- 5* flopow(  (((float)left_sum / (left_sum+right_sum))-0.5  )*2.0 , 1.1)  );
  }
  else
	joystick.steer(0.0);

  //imshow("orig", img);
  imshow("edit", img2);
  //imshow("perspective", img_perspective);
  //imshow("diff", img_diff);
  imshow("hist", img_hist);
  imshow("thres", img_thres);
  imshow("thres2", img_thres2);
  //imshow("stddev", img_stddev);
  imshow("steer", steer);
  
  Mat road_color(100,100, CV_8UC3, Scalar(road_0, road_1, road_2));
  imshow("road_color", road_color);
  waitKey(1000/50);
  
  joystick.process();
  //waitKey();
}
}
