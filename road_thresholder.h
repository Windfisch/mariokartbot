/*
 * road_thresholder.h
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


#ifndef __ROAD_THRESHOLDER_H__
#define __ROAD_THRESHOLDER_H__

#include <opencv2/opencv.hpp>
#include "road_thresholder_iface.h"

using namespace cv;

class RoadThresholder : public RoadThresholderIface
{
	public:
		RoadThresholder();
		RoadThresholder(int r, int g, int b);
		virtual void process_image(const Mat& img);
		virtual Mat& get_road();
	
	private:
		Mat mask_raw;
		int road_0;
		int road_1;
		int road_2;
		Mat erode_2d_small;
		
		Mat create_diff_image_and_fill_histogram(const Mat& img, int* histogram);
		void calc_road_color(const Mat& img, const Mat& mask_eroded);
		static void smoothen_histogram(int* hist, int* hist_smooth, int smoothness);
		
};


#endif
