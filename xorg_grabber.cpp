/*
 * xorg_grabber.cpp
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

#include "xorg_grabber.h"
#include "xcb/xcb_icccm.h"
#include <iostream>
using namespace std;


// below code was used and slightly adapted from xwininfo
// the following license applies.
/*
 * Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/*
 * Window_With_Name: routine to locate a window with a given name on a display.
 *                   If no window with the given name is found, 0 is returned.
 *                   If more than one window has the given name, the first
 *                   one found will be returned.  Only top and its subwindows
 *                   are looked at.  Normally, top should be the RootWindow.
 */

struct wininfo_cookies {
    xcb_get_property_cookie_t get_net_wm_name;
    xcb_get_property_cookie_t get_wm_name;
    xcb_query_tree_cookie_t query_tree;
};

#define False 0

int names_match(const char* haystack, int hslen, const char* needle, int nlen)
{
	char temph[1024];
	char tempn[1024];

	strncpy(temph, haystack, 1023);
	temph[1023]=0;
	if (hslen < 1024-1)
		temph[hslen+1]=0;
	
	strncpy(tempn, needle, 1023);
	tempn[1023]=0;
	if (nlen < 1024-1)
		tempn[nlen+1]=0;
	
	if (strstr(temph, tempn))
		return 1;
	else
		return 0;
}
#define BUFSIZ 42

#ifndef USE_XCB_ICCCM
# define xcb_icccm_get_wm_name(Dpy, Win) \
    xcb_get_property (Dpy, False, Win, XCB_ATOM_WM_NAME, \
		      XCB_GET_PROPERTY_TYPE_ANY, 0, BUFSIZ)
#endif

static xcb_atom_t atom_net_wm_name, atom_utf8_string;

# define xcb_get_net_wm_name(Dpy, Win)			 \
    xcb_get_property (Dpy, False, Win, atom_net_wm_name, \
		      atom_utf8_string, 0, BUFSIZ)


static xcb_window_t
recursive_Window_With_Name  (xcb_connection_t *dpy, xcb_window_t window, struct wininfo_cookies *cookies, const char *name, size_t namelen)
{
    xcb_window_t *children;
    unsigned int nchildren;
    int i;
    xcb_window_t w = 0;
    xcb_generic_error_t *err;
    xcb_query_tree_reply_t *tree;
    struct wininfo_cookies *child_cookies;
    xcb_get_property_reply_t *prop;

    if (cookies->get_net_wm_name.sequence) {
	prop = xcb_get_property_reply (dpy, cookies->get_net_wm_name, &err);

	if (prop) {
	    if (prop->type == atom_utf8_string) {
		const char *prop_name = (const char*) xcb_get_property_value (prop);
		int prop_name_len = xcb_get_property_value_length (prop);

		/* can't use strcmp, since prop.name is not null terminated */
		if (names_match(prop_name, prop_name_len, name, namelen))
		    w = window;
	    }
	    free (prop);
	} else if (err) {
	    if (err->response_type == 0)
		throw string("error");
	    return 0;
	}
    }

    if (w) {
	xcb_discard_reply (dpy, cookies->get_wm_name.sequence);
    } else {
	prop = xcb_get_property_reply (dpy, cookies->get_wm_name, &err);

	if (prop) {
	    if (prop->type == XCB_ATOM_STRING) {
		const char *prop_name = (const char*) xcb_get_property_value (prop);
		int prop_name_len = xcb_get_property_value_length (prop);

		/* can't use strcmp, since prop.name is not null terminated */
		if (names_match(prop_name, prop_name_len, name, namelen))
		    w = window;
	    }
	    free (prop);
	}
	else if (err) {
	    if (err->response_type == 0)
		throw string("error");
	    return 0;
	}
    }

    if (w)
    {
	xcb_discard_reply (dpy, cookies->query_tree.sequence);
	return w;
    }

    tree = xcb_query_tree_reply (dpy, cookies->query_tree, &err);
    if (!tree) {
	if (err->response_type == 0)
	   throw string("error"); 
	return 0;
    }

    nchildren = xcb_query_tree_children_length (tree);
    children = xcb_query_tree_children (tree);
    child_cookies = (struct wininfo_cookies*) calloc(nchildren, sizeof(struct wininfo_cookies));

    if (child_cookies == NULL)
	throw string("Failed to allocate memory in recursive_Window_With_Name");

    for (i = 0; i < nchildren; i++) {
	if (atom_net_wm_name && atom_utf8_string)
	    child_cookies[i].get_net_wm_name =
		xcb_get_net_wm_name (dpy, children[i]);
	child_cookies[i].get_wm_name = xcb_icccm_get_wm_name (dpy, children[i]);
	child_cookies[i].query_tree = xcb_query_tree (dpy, children[i]);
    }
    xcb_flush (dpy);

    for (i = 0; i < nchildren; i++) {
	w = recursive_Window_With_Name (dpy, children[i],
					&child_cookies[i], name, namelen);
	if (w)
	    break;
    }

    if (w)
    {
	/* clean up remaining replies */
	for (/* keep previous i */; i < nchildren; i++) {
	    if (child_cookies[i].get_net_wm_name.sequence)
		xcb_discard_reply (dpy,
				   child_cookies[i].get_net_wm_name.sequence);
	    xcb_discard_reply (dpy, child_cookies[i].get_wm_name.sequence);
	    xcb_discard_reply (dpy, child_cookies[i].query_tree.sequence);
	}
    }

    free (child_cookies);
    free (tree); /* includes storage for children[] */
    return (w);
}

xcb_window_t Window_With_Name (xcb_connection_t *dpy, xcb_window_t top, const char *name)
{
    struct wininfo_cookies cookies;

    atom_net_wm_name = 0; // Get_Atom (dpy, "_NET_WM_NAME");
    atom_utf8_string = 0; // Get_Atom (dpy, "UTF8_STRING");

    if (atom_net_wm_name && atom_utf8_string)
	cookies.get_net_wm_name = xcb_get_net_wm_name (dpy, top);
    cookies.get_wm_name = xcb_icccm_get_wm_name (dpy, top);
    cookies.query_tree = xcb_query_tree (dpy, top);
    xcb_flush (dpy);
    return recursive_Window_With_Name(dpy, top, &cookies, name, strlen(name));
}

// END xwininfo stolen code


XorgGrabber::XorgGrabber(const char* win_title)
{
	conn=xcb_connect(NULL,NULL);

	cerr << "Connected to X server" << endl;
	
	bool found_win=false;
	grab_screen=NULL;
	img=NULL;
	
	/* Find configured screen */
	const xcb_setup_t* setup = xcb_get_setup(conn);
	for (xcb_screen_iterator_t i = xcb_setup_roots_iterator(setup);
		 i.rem > 0; xcb_screen_next (&i))
	{
		cerr << "let's check that screen" << endl;
		xcb_screen_t* scr = i.data;

		grabbed_win = Window_With_Name(conn, scr->root, win_title);
		cout << "GOTCHA" << endl;
		xcb_get_geometry_reply_t* geo;
		geo = xcb_get_geometry_reply (conn, xcb_get_geometry (conn, grabbed_win), NULL);
		if (geo)
		{
			grab_width=geo->width;
			grab_height=geo->height;
			
			free(geo);

			found_win=true;

			grab_screen=scr;						
		}
		else
		{
			cerr << "geo==NULL!" << endl;
		}

#if 0
		xcb_query_tree_reply_t* reply = xcb_query_tree_reply( conn, xcb_query_tree(conn, scr->root), NULL);
		if (reply)
		{
			int len = xcb_query_tree_children_length(reply);

			cout << "screen has "<<len<<" windows"<<endl;

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
				
				char* title=(char*)(title_reply+1);
				cout << "Considering window with title "<< title << "override_redirect="<<int(attr->override_redirect)<<" map_state="<<int(attr->map_state) <<" ==?"<<XCB_MAP_STATE_VIEWABLE<<endl;
				//if (!attr->override_redirect && attr->map_state == XCB_MAP_STATE_VIEWABLE)
				if (attr->map_state == XCB_MAP_STATE_VIEWABLE)
				//if (!attr->override_redirect) 
				{
					cout << "Checking window with title "<< title << endl;
					if (strstr(title, win_title))
					{
						cout << "GOTCHA" << endl;
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
	#endif
	}
	
	if (found_win)
	{
		xcb_get_image_reply_t* img = xcb_get_image_reply (conn,
			xcb_get_image (conn, XCB_IMAGE_FORMAT_Z_PIXMAP, grabbed_win,
				0, 0, grab_width, grab_height, ~0), NULL);
		cout << img << endl;
		


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

