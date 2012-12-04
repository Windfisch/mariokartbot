/*
 * detect_road_borders_with_horizon_steerer.cpp
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

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <opencv2/opencv.hpp>
#include "horizon_steerer.h"
#include "util.h"

using namespace std;
using namespace cv;


#define AREA_HISTORY 10

int alertcnt=21;
int main(int argc, char* argv[])
{
	if (argc!=2) {printf("usage: %s videofile\n",argv[0]); exit(1);}
	VideoCapture capture(argv[1]);
	
	if (!capture.isOpened())
	{
		cout << "couldn't open file" << endl;
		exit(1);
	}
	
	Mat erode_kernel=circle_mat(10);
	
	
	Mat frame;
	capture >> frame;
	
	

	
	
	Mat thres(frame.rows, frame.cols, CV_8UC1);
	Mat tmp(frame.rows, frame.cols, CV_8UC1);
	
	HorizonSteerer* horizon_steerer = new HorizonSteerer(frame.cols, frame.rows);
	int frameno=0;
	
	while (1)
	{
		capture >> frame;
		
		if (frameno<190)
		{
			frameno++;
			continue;
		}
		
		cvtColor(frame, tmp, CV_RGB2GRAY);
		threshold(tmp, thres, 132, 255, THRESH_BINARY);
		dilate(thres,tmp,Mat());
		erode(tmp,thres,Mat());
		erode(thres,tmp,Mat());
		dilate(tmp,thres,Mat());
		
		
		horizon_steerer->process_image(thres);
		
		
		cout << "frame #"<<frameno<<endl;

		imshow("thres",thres);
		//imshow("drawing",drawing);
//		waitKey(100);
		waitKey();
		
		frameno++;
	}
  
}
