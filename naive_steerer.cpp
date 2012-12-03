/*
 * naive_steerer.cpp
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


#include "naive_steerer.h"
#include "util.h"

using namespace std;

NaiveSteerer::NaiveSteerer(int chx, int chy)
{
	set_crosshair(chx,chy);
}

void NaiveSteerer::set_crosshair(int chx, int chy)
{
	crosshair_x=chx;
	crosshair_y=chy;
}

void NaiveSteerer::process_image(const Mat& img)
{
	int left_sum=0, right_sum=0;
	for (int row = 0; row<img.rows; row++)
	{
		const uchar* data=img.ptr<uchar>(row);
		
		for (int col=0; col<img.cols;col++)
		{
			if (*data)
			{
				if (row<=crosshair_y)
				{
					if (col < crosshair_x)
						left_sum++;
					else
						right_sum++;
				}
			}
			
			data+=img.step[1];
		}
	}
	
	steer = -4* flopow(  (((float)left_sum / (left_sum+right_sum))-0.5  )*2.0 , 1.6);
}

double NaiveSteerer::get_steer_data()
{
	return steer;
}

double NaiveSteerer::get_confidence()
{
	//return confidence;
	return 1.0; // TODO
}
