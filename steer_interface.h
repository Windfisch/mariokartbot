#ifndef __STEER_IFACE_H__
#define __STEER_IFACE_H__

#include <opencv2/opencv.hpp>

using namespace cv;

class SteerIface
{
	public:
		virtual void process_image(const Mat& img)=0;
		virtual double get_steer_data()=0;
		virtual double get_confidence()=0;
};


#endif
