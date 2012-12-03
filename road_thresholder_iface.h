#ifndef __ROAD_THRESHOLDER_IFACE_H__
#define __ROAD_THRESHOLDER_IFACE_H__

#include <opencv2/opencv.hpp>

using namespace cv;

class RoadThresholderIface
{
	public:
		virtual void process_image(const Mat& img)=0;
		virtual Mat& get_road()=0;
};


#endif
