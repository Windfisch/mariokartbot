/*
 * naive_steerer.h
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


#ifndef __NAIVE_STEERER_H__
#define __NAIVE_STEERER_H__

#include <opencv2/opencv.hpp>
#include "steer_interface.h"
#include <list>

using namespace cv;


class NaiveSteerer : public SteerIface
{
	public:
		NaiveSteerer(int chx, int chy);
		void set_crosshair(int chx, int chy);
		
		virtual ~NaiveSteerer() {};
		
		virtual void process_image(const Mat& img);
		virtual double get_steer_data();
		virtual double get_confidence();
	
	private:
		double steer;
		double confidence;
		int crosshair_x;
		int crosshair_y;
};

#endif
