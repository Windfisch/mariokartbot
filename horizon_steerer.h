#ifndef __HORIZON_STEERER_H__
#define __HORIZON_STEERER_H__

#include <opencv2/opencv.hpp>
#include "steer_interface.h"

using namespace cv;

class HorizonSteerer : SteerIface
{
public:
	HorizonSteerer(int xlen_, int ylen_);
	virtual ~HorizonSteerer();
	int find_ideal_line(vector<Point>& contour, Point origin_point, int** contour_map, int bestquality_j);

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
	                 int steering_point, Point origin_point);
	int find_steering_point(Mat orig_img, Point origin_point, int** contour_map, Mat& drawing, double* confidence);

	int xlen;
	int ylen;
};

#endif
