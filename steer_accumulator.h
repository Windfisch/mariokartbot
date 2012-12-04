/*
 * steer_accumulator.h
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


#ifndef __STEER_ACCUMULATOR_H__
#define __STEER_ACCUMULATOR_H__

#include <opencv2/opencv.hpp>
#include "steer_interface.h"
#include <list>

using namespace cv;


class SteerAccumulator : SteerIface
{
	public:
		virtual ~SteerAccumulator() {};
		
		void add_steerer(SteerIface* steerer, double weight=1.0);
		
		virtual void process_image(const Mat& img);
		virtual double get_steer_data();
		virtual double get_confidence();
	
	private:
		struct entry
		{
			SteerIface* st;
			double weight;
			
			entry(SteerIface* s, double w) { st=s; weight=w; }
		};
		std::list<entry> steerers;
};

#endif
