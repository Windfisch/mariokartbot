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
		
		void add_steerer(SteerIface* steerer);
		
		virtual void process_image(const Mat& img);
		virtual double get_steer_data();
		virtual double get_confidence();
	
	private:
		std::list<SteerIface*> steerers;
};

#endif
