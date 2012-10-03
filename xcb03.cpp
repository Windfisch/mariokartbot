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


#include <iostream>
#include <xcb/xcb.h>
#include <stdlib.h>
#include <string.h>
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
}

int main()
{
	XorgGrabber grabber("Mupen64Plus OpenGL Video");
	
	namedWindow("meh");
	Mat meh;
	while(1)
	{
		grabber.read(meh);
		imshow("meh",meh);
		waitKey(100);
	}
}
