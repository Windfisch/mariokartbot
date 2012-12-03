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
