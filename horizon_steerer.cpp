/*
 * horizon_steerer.cpp
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

HorizonSteerer::HorizonSteerer(int xlen_, int ylen_)
{
	xlen=xlen_;
	ylen=ylen_;
	
	my_contour_map=new int*[xlen];
	for (int i=0;i<xlen;i++)
		my_contour_map[i]=new int[ylen];
		
	erode_kernel = circle_mat(10);
}


void HorizonSteerer::process_image(const Mat& img_)
{
	Mat img;
	img_.copyTo(img);
	
	int area_abs;
	double area_ratio = only_retain_largest_region(img, &area_abs);
	
	Mat tmp;
	dilate(img, tmp, erode_kernel);
	erode(tmp, img, erode_kernel);


	Mat drawing;
	double confidence;
	find_steering_point(img, Point(img.cols/2, img.rows-2*img.rows/5), my_contour_map, drawing, &confidence);
	imshow("drawing",drawing);
}

double HorizonSteerer::get_steer_data() { return 0.0; }
double HorizonSteerer::get_confidence() { return 1.0; }



static void set_pixel(Mat m, Point p, Scalar color)
{
	line(m,p,p,color);
}

int HorizonSteerer::find_intersection_index(int x0, int y0, int x1, int y1, int** contour_map, bool stop_at_endpoint) // bresenham aus der dt. wikipedia
// returns: the point's index where the intersection happened, or a negative number if no intersection.
{
  int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = dx+dy, e2; /* error value e_xy */
 
  for(;;)
  {
    
    //setPixel(x0,y0);
    if (contour_map[x0][y0]>0) return contour_map[x0][y0]; // found intersection?
    if (contour_map[x0][y0+1]>0) return contour_map[x0][y0+1];
    if (contour_map[x0+1][y0]>0) return contour_map[x0+1][y0];
    
		
    
    
    if (stop_at_endpoint && x0==x1 && y0==y1) break;
    e2 = 2*err;
    if (e2 > dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
    if (e2 < dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
  }
  
  return -1;
}


static void hue2rgb(float hue, int* r, int* g, int* b)
{
	double ff;
	int i;


	if (hue >= 360.0) hue = 0.0;
	hue /= 60.0;
	i = (int)hue;
	ff = hue - i;
	int x=ff*255;

	switch(i) {
	case 0:
		*r = 255;
		*g = x;
		*b = 0;
		break;
	case 1:
		*r = 255-x;
		*g = 255;
		*b = 0;
		break;
	case 2:
		*r = 0;
		*g = 255;
		*b = x;
		break;

	case 3:
		*r = 0;
		*g = 255-x;
		*b = 255;
		break;
	case 4:
		*r = x;
		*g = 0;
		*b = 255;
		break;
	case 5:
	default:
		*r = 255;
		*g = 0;
		*b = 255-x;
		break;
	}
}

static double linear(double x, double x1, double y1, double x2, double y2, bool clip=false, double clipmin=INFINITY, double clipmax=INFINITY)
{	
	if (clipmin==INFINITY) clipmin=y1;
	if (clipmax==INFINITY) clipmax=y2;
	if (clipmin>clipmax) { int tmp=clipmin; clipmin=clipmax; clipmax=tmp; }
	
	double result = (y2-y1)*(x-x1)/(x2-x1)+y1;
	
	if (clip)
	{
		if (result>clipmax) return clipmax;
		else if (result<clipmin) return clipmin;
		else return result;
	}
	else
		return result;
}


int HorizonSteerer::annotate_regions(Mat img) //img is treated as black/white (0, !=0)
// changes img, and returns the number of found areas
{
	int region_index=1; // "0" means "no area"
	for (int row = 0; row<img.rows; row++)
	{
		uchar* data=img.ptr<uchar>(row);

		for (int col=0; col<img.cols;col++)
		{
			if (*data==255)
			{
				floodFill(img, Point(col,row), region_index);
				region_index++;
			}

			data++;
		}
	}
	return region_index-1;
}

Mat HorizonSteerer::nicely_draw_regions(Mat annotated, int* area_cnt, int total_area_cnt, int largest_region)
{
	Mat result;
	annotated.copyTo(result);
	
	// Das ist nur zum schönsein.
	for (int row=0; row<result.rows; row++)
	{
		uchar* data=result.ptr<uchar>(row);

		for (int col=0; col<result.cols;col++)
		{
			if (*data)
			{
				long long tmp = (long long)30000*(long)area_cnt[*data-1]/(long)total_area_cnt + 64;
				if (tmp>200) tmp=200;
				if (*data==largest_region) tmp=255;
				*data=tmp;
			}
			
			data++;
		}
	}
}

double HorizonSteerer::only_retain_largest_region(Mat img, int* size)
// img is a binary image
// in *size, if non-NULL, the size of the largest area is stored.
// returns: ratio between the second-largest and largest region
//          0.0 means "that's the only region", 1.0 means "both had the same size!"
// can be interpreted as 1.0 - "confidence".
{
		int n_regions = annotate_regions(img);
		
		// calculate the area of each region
		int* area_cnt = new int[n_regions];
		for (int i=0;i<n_regions;i++) area_cnt[i]=0;
		int total_area_cnt=0;
		
		for (int row=0; row<img.rows; row++)
		{
			uchar* data=img.ptr<uchar>(row);

			for (int col=0; col<img.cols;col++)
			{
				if (*data)
				{
					area_cnt[*data-1]++;
					total_area_cnt++;
				}
				
				data++;
			}
		}


		
		
		// finde die größte und zweitgrößte fläche
		int maxi=0, maxa=area_cnt[0], maxi2=-1;
		for (int i=1;i<n_regions;i++)
		{
			if (area_cnt[i]>maxa)
			{
				maxa=area_cnt[i];
				maxi2=maxi;
				maxi=i;
			}
		}
		
		
		// lösche alle bis auf die größte fläche
		for (int row = 0; row<img.rows; row++)
		{
			uchar* data=img.ptr<uchar>(row);

			for (int col=0; col<img.cols;col++)
			{
				if (*data)
				{
					if (*data!=maxi+1) *data=0; else *data=255;
				}
				data++;
			}
		}
		
		
		if (size) *size=area_cnt[maxi];
		
		if (maxi2==-1) return 0;
		else           return (double)area_cnt[maxi2]/(double)area_cnt[maxi];
}


vector<Point>& HorizonSteerer::prepare_and_get_contour(vector< vector<Point> >& contours, const vector<Vec4i>& hierarchy,
                                       int* low_y, int* low_idx, int* high_y, int* first_nonbottom_idx)
{
	assert(low_y!=NULL);
	assert(low_idx!=NULL);
	assert(high_y!=NULL);
	assert(first_nonbottom_idx!=NULL);
	
	
	// find index of our road contour
	int road_contour_idx=-1;
	for (road_contour_idx=0; road_contour_idx<contours.size(); road_contour_idx++)
		if (hierarchy[road_contour_idx][3]<0) // this will be true for exactly one road_contour_idx.
			break;

	
	assert(road_contour_idx>=0 && road_contour_idx<contours.size());
	assert(contours[road_contour_idx].size()>0);
	vector<Point>& contour = contours[road_contour_idx]; // just a shorthand
	
	// our road is now in contour.
	
	
	// find highest and lowest contour point. (where "low" means high y-coordinate)
	*low_y=0; *low_idx=0;
	*high_y=ylen;
	
	for (int j=0;j<contour.size(); j++)
	{
		if (contour[j].y > *low_y)
		{
			*low_y=contour[j].y;
			*low_idx=j;
		}
		if (contour[j].y < *high_y)
		{
			*high_y=contour[j].y;
		}
	}
	

	// make the contour go "from bottom upwards and then downwards back to bottom".
	std::rotate(contour.begin(),contour.begin()+*low_idx,contour.end());

	*first_nonbottom_idx = 0;
	for (;*first_nonbottom_idx<contour.size();*first_nonbottom_idx++)
		if (contour[*first_nonbottom_idx].y < contour[0].y-1) break;

	// indices 0 to *first_nonbottom_idx-1 is now the bottom line of our contour.
	
	return contour;
}

void HorizonSteerer::init_contour_map(const vector<Point>& contour, int** contour_map)
{
	for (int j=0;j<xlen;j++) // zero it
		memset(contour_map[j],0,ylen*sizeof(**contour_map));
		
	for (int j=0;j<contour.size(); j++) // fill it
		contour_map[contour[j].x][contour[j].y]=j;
}

// returns a new double[]
double* HorizonSteerer::calc_contour_angles(const vector<Point>& contour, int first_nonbottom_idx, int smoothen_middle, int smoothen_bottom)
{
	// calculate directional angle for each nonbottom contour point
	double* angles = new double[contour.size()];
	for (int j=first_nonbottom_idx; j<contour.size(); j++)
	{
		int smoothen=linear(contour[j].y, ylen/2  ,smoothen_middle, ylen,smoothen_bottom, true);
		

		// calculate left and right point for the difference quotient, possibly wrap.
		int j1=(j+smoothen); while (j1 >= contour.size()) j1-=contour.size();
		int j2=(j-smoothen); while (j2 < 0) j2+=contour.size();


		// calculate angle, adjust it to be within [0, 360)
		angles[j] = atan2(contour[j1].y - contour[j2].y, contour[j1].x - contour[j2].x) * 180/3.141592654;
		if (angles[j]<0) angles[j]+=360;
	}
	return angles;
}

double* HorizonSteerer::calc_angle_deriv(double* angles, int first_nonbottom_idx, int size, int ang_smooth)
{
	// calculate derivative of angle for each nonbottom contour point
	double* angle_derivative = new double[size];
	for (int j=first_nonbottom_idx+ang_smooth; j<size-ang_smooth; j++)
	{
		// calculate angular difference, adjust to be within [0;360) and take the shorter way.
		double ang_diff = angles[j+ang_smooth]-angles[j-ang_smooth];
		while (ang_diff<0) ang_diff+=360;
		while (ang_diff>=360) ang_diff-=360;
		if (ang_diff>=180) ang_diff=360-ang_diff;
		
		angle_derivative[j] = (double)ang_diff / ang_smooth;
	}
	
	// poorly extrapolate the ang_smooth margins
	for (int j=first_nonbottom_idx; j<first_nonbottom_idx+ang_smooth; j++) angle_derivative[j]=angle_derivative[first_nonbottom_idx+ang_smooth];
	for (int j=size-ang_smooth; j<size; j++) angle_derivative[j]=angle_derivative[size-ang_smooth-1];
	
	return angle_derivative;
}



int HorizonSteerer::find_bestquality_index(const vector<Point>& contour, double* angle_derivative, int high_y, int first_nonbottom_idx, Mat& drawing,
                           int* bestquality_j_out, int* bestquality_width_out, int* bestquality_out, int* bestquality_max_out)
{
	assert(bestquality_out!=NULL);
	assert(bestquality_j_out!=NULL);
	assert(bestquality_width_out!=NULL);
	
	double lastmax=-999999;
	double bestquality=0.0;
	double bestquality_max=0.0;
	int bestquality_j=-1;
	int bestquality_width=-1;
	
	#define MAX_HYST 0.8
	// search for "maximum regions"; i.e. intervals [a,b] with
	// ang_deriv[i] >= MAX_HYST * max_deriv \forall i \in [a,b] and
	// ang_deriv[a-1,2,3], ang_deriv[b+1,2,3] < MAX_HYST * max_deriv
	// where max_deriv = max_{i \in [a,b]} ang_deriv[i];
	for (int j=3; j<contour.size()-3; j++)
	{
		// search forward for a maximum, and the end of a maximum region.
		if (angle_derivative[j] > lastmax) lastmax=angle_derivative[j];
		
		if (angle_derivative[j]   < MAX_HYST*lastmax && // found the end of the max. region
			angle_derivative[j+1] < MAX_HYST*lastmax && 
			angle_derivative[j+2] < MAX_HYST*lastmax)
		{
			if (lastmax > 7) // threshold the maximum.
			{
				// search backward for the begin of that maximum region
				int j0;
				for (j0=j-1; j0>=0; j0--)
					if (angle_derivative[j0] < MAX_HYST*lastmax &&
						angle_derivative[j0-1] < MAX_HYST*lastmax &&
						angle_derivative[j0-2] < MAX_HYST*lastmax)
						break;
				
				// maximum region is [j0; j]

				double median_of_max_region = (double)angle_derivative[(j+j0)/2];
				
				// calculate quality of that maximum. quality is high, if
				// 1) the maximum has a high value AND
				// 2) the corresponding point's y-coordinates are near the top image border AND
				// 3) the corresponding point's x-coordinates are near the middle of the image, if in doubt
				int middle_x = xlen/2;
				int distance_from_middle_x = abs(xlen/2 - contour[j].x);
				double quality = lastmax
								  *  linear( contour[j].y,               high_y,       1.0,       high_y+ (ylen-high_y)/10, 0.0,   true)  // excessively punish points far away from the top border
								  *  linear( distance_from_middle_x,     0.8*middle_x, 1.0,       middle_x,                         0.6,   true); // moderately punish point far away from the x-middle.
				
				// keep track of the best point
				if (quality>bestquality)
				{
					bestquality=quality;
					bestquality_max=lastmax;
					bestquality_j=(j+j0)/2;
					bestquality_width=j-j0;
				}
				
				
				// irrelevant drawing stuff
				int x=drawing.cols-drawing.cols*((j+j0)/2-first_nonbottom_idx)/(contour.size()-first_nonbottom_idx);
				line(drawing, Point(x,25+40-3*quality), Point(x, 25+40), Scalar(0,255,0));
				circle(drawing, contour[(j+j0)/2], 1, Scalar(128,0,0));
			}
			
			lastmax=-999999; // reset lastmax, so the search can go on
		}
	}
	// now bestquality_j holds the index of the point with the best quality.
	
	*bestquality_out = bestquality;
	*bestquality_max_out = bestquality_max;
	*bestquality_j_out = bestquality_j;
	*bestquality_width_out = bestquality_width;
}

int HorizonSteerer::find_ideal_line(vector<Point>& contour, Point origin_point, int** contour_map, int bestquality_j)
// TODO: this code is crappy, slow, and uses brute force. did i mention it's crappy and slow?
{
	int intersection = find_intersection_index(origin_point.x,           origin_point.y, 
											   contour[bestquality_j].x, contour[bestquality_j].y,        contour_map);
	int steering_point=-1;
	
	if (intersection<0)
	{
		cout << "THIS SHOULD NEVER HAPPEN" << endl;
		return -1;
	}
	else
	{
		int xx=contour[bestquality_j].x;
		int lastheight=-1;
		if (intersection < bestquality_j) // too far on the right == intersecting the right border
		{
			// rotate the line to the left till it gets better
			for (; xx>=0; xx--)
			{
				int intersection2 = find_intersection_index(origin_point.x, origin_point.y, xx, contour[bestquality_j].y, contour_map);
				if (intersection2<0) // won't happen anyway
					break;
				
				if (intersection2>=bestquality_j) // now we intersect the opposite (=left) border
				{
					if (contour[intersection2].y>=lastheight) // we intersect at a lower = worse point?
						xx++;                                 // then undo last step
						
					break;
				}
				lastheight=contour[intersection2].y;
			}
		}
		else if (intersection > bestquality_j) // too far on the left == intersecting the left border
		{
			// rotate the line to the right till it gets better
			for (; xx<xlen; xx++)
			{
				int intersection2 = find_intersection_index(origin_point.x, origin_point.y, xx, contour[bestquality_j].y, contour_map);
				if (intersection2<0)// won't happen anyway
					break;
				
				if (intersection2<=bestquality_j) // now we intersect the opposite (=right) border
				{
					if (contour[intersection2].y>=lastheight) // we intersect at a lower = worse point?
						xx--;                                 // then undo last step
						
					break;
				}
				lastheight=contour[intersection2].y;
			}
		}
		// else // we directly met the bestquality point, i.e. where we wanted to go to.
			// do nothing
		
		return find_intersection_index(origin_point.x,origin_point.y, xx, contour[bestquality_j].y, contour_map, false);
	}
}


void HorizonSteerer::draw_it_all(Mat drawing, vector< vector<Point> >& contours, const vector<Vec4i>& hierarchy, int first_nonbottom_idx, vector<Point>& contour,
                 double* angles, double* angle_derivative, int bestquality_j, int bestquality_width, int bestquality,
                 int steering_point, Point origin_point)
{
	// Draw contours
	drawContours(drawing, contours, -1, Scalar(255,0,0), 1, 8, hierarchy);

	// draw the angles
	for (int j=first_nonbottom_idx; j<contour.size(); j++)
	{
		int x=drawing.cols-drawing.cols*(j-first_nonbottom_idx)/(contour.size()-first_nonbottom_idx);
		
		// draw angle as color bar
		int r,g,b;
		hue2rgb(angles[j], &r, &g, &b);
		line(drawing,Point(x,0), Point(x,10), Scalar(b,g,r));

		// draw derivation of angle as color bar
		int c=abs(20* angle_derivative[j]);
		Scalar col=(c<256) ? Scalar(255-c,255-c,255) : Scalar(255,0,255);
		line(drawing, Point(x,12), Point(x,22), col);
		
		// and as x-y-graph
		int y=25+40-2*angle_derivative[j];
		set_pixel(drawing, Point(x,y), Scalar(255,255,255));
		
		// draw into contour
		//circle(drawing, contour[j], 2, col);
		set_pixel(drawing, contour[j], col);
	}
	
	// draw the point where the left touches the right road border
	circle(drawing, contour[bestquality_j], 3, Scalar(255,255,0));
	circle(drawing, contour[bestquality_j], 2, Scalar(255,255,0));
	circle(drawing, contour[bestquality_j], 1, Scalar(255,255,0));
	circle(drawing, contour[bestquality_j], 0, Scalar(255,255,0));

	// draw the detected left and right border. low saturation means
	// a worse detection result
	int antisaturation = 200-(200* bestquality/10.0);
	if (antisaturation<0) antisaturation=0;
	for (int j=0;j<bestquality_j-bestquality_width/2;j++)
		set_pixel(drawing, contour[j], Scalar(255,antisaturation,255));
	for (int j=bestquality_j+bestquality_width/2;j<contour.size();j++)
		set_pixel(drawing, contour[j], Scalar(antisaturation,255,antisaturation));
		
	// a direct line to where left touches right
	line(drawing, contour[bestquality_j], origin_point, Scalar(0,64,64));
	
	if (steering_point>=0) // should be always true
		line(drawing, contour[steering_point], origin_point, Scalar(0,255,255));
}

#define SMOOTHEN_BOTTOM 20
#define SMOOTHEN_MIDDLE 7
#define ANG_SMOOTH 9
// return the index of the point to steer to.
int HorizonSteerer::find_steering_point(Mat orig_img, Point origin_point, int** contour_map, Mat& drawing, double* confidence) // orig_img is a binary image
// confidence is between 0.0 (not sure at all) and 1.0 (definitely sure)
{
	assert(confidence!=NULL);
	
	Mat img;
	orig_img.copyTo(img); // this is needed because findContours destroys its input.
	drawing = Mat::zeros( img.size(), CV_8UC3 );
	
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));
	
	int low_y, low_idx, high_y, first_nonbottom_idx;
	vector<Point>& contour = prepare_and_get_contour(contours, hierarchy,
	                                                 &low_y, &low_idx, &high_y, &first_nonbottom_idx);
	                                                 
	init_contour_map(contour, contour_map);
	

	double* angles = calc_contour_angles(contour, first_nonbottom_idx, SMOOTHEN_MIDDLE, SMOOTHEN_BOTTOM);
	double* angle_derivative = calc_angle_deriv(angles, first_nonbottom_idx, contour.size(), ANG_SMOOTH);
	
	int bestquality, bestquality_j, bestquality_width, bestquality_max;
	find_bestquality_index(contour, angle_derivative, high_y, first_nonbottom_idx, drawing,
	                       &bestquality_j, &bestquality_width, &bestquality, &bestquality_max);
	
	// now we have a naive steering point. the way to it might lead
	// us offroad, however.

	int steering_point=find_ideal_line(contour, origin_point, contour_map, bestquality_j);

	
	draw_it_all(drawing, contours, hierarchy, first_nonbottom_idx, contour, angles, angle_derivative,bestquality_j,bestquality_width,bestquality_max,steering_point, origin_point);
	cout << bestquality << "\t" << bestquality_max<<endl;
	delete [] angle_derivative;
	delete [] angles;
	
	*confidence = (bestquality-1.0) / 3.0;
	if (*confidence<0.0) *confidence=0;
	if (*confidence>1.0) *confidence=1.0;
	return steering_point;
}

