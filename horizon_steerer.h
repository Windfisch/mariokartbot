/*
 * horizon_steerer.h
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


#ifndef __HORIZON_STEERER_H__
#define __HORIZON_STEERER_H__

#include <opencv2/opencv.hpp>
#include "steer_interface.h"

using namespace cv;

class HorizonSteerer : public SteerIface
{
public:
	HorizonSteerer(int xlen_, int ylen_);
	virtual ~HorizonSteerer() {};
	int find_ideal_line(vector<Point>& contour, Point origin_point, int** contour_map, int bestquality_j);
	
	virtual void process_image(const Mat& img);
	virtual double get_steer_data();
	virtual double get_confidence();


private:
	int find_intersection_index(int x0, int y0, int x1, int y1, int** contour_map, bool stop_at_endpoint=true);
	int annotate_regions(Mat img);
	Mat nicely_draw_regions(Mat annotated, int* area_cnt, int total_area_cnt, int largest_region);
	double only_retain_largest_region(Mat img, int* size);
	vector<Point>& prepare_and_get_contour(vector< vector<Point> >& contours, const vector<Vec4i>& hierarchy,
	                                       int* low_y, int* low_idx, int* high_y, int* first_nonbottom_idx);
	void init_contour_map(const vector<Point>& contour, int** contour_map);
	double* calc_contour_angles(const vector<Point>& contour, int first_nonbottom_idx, int smoothen_middle, int smoothen_bottom);
	double* calc_angle_deriv(double* angles, int first_nonbottom_idx, int size, int ang_smooth);
	int find_bestquality_index(const vector<Point>& contour, double* angle_derivative, int high_y, int first_nonbottom_idx, Mat& drawing,
	                           int* bestquality_j_out, int* bestquality_width_out, int* bestquality_out, int* bestquality_max_out);
	void draw_it_all(Mat drawing, vector< vector<Point> >& contours, const vector<Vec4i>& hierarchy, int first_nonbottom_idx, vector<Point>& contour,
	                 double* angles, double* angle_derivative, int bestquality_j, int bestquality_width, int bestquality,
	                 int steering_point, Point origin_point, double confidence);
	void draw_angles_and_contour(Mat drawing, vector< vector<Point> >& contours, const vector<Vec4i>& hierarchy, int first_nonbottom_idx, vector<Point>& contour,
	                             double* angles, double* angle_derivative);
	Point find_steering_point(Mat orig_img, Point origin_point, int** contour_map, Mat& drawing, double* confidence);

	int xlen;
	int ylen;
	Mat erode_kernel;
	int** my_contour_map;
	
	double confidence;
	double steer_value;
};

#endif
