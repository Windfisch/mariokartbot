#ifndef __NAIVE_STEERER_H__
#define __NAIVE_STEERER_H__

#include <opencv2/opencv.hpp>
#include "steer_interface.h"
#include <list>

using namespace cv;


class NaiveSteerer : public SteerIface
{
	public:
		NaiveSteerer(int chx, int chy);
		void set_crosshair(int chx, int chy);
		
		virtual ~NaiveSteerer() {};
		
		virtual void process_image(const Mat& img);
		virtual double get_steer_data();
		virtual double get_confidence();
	
	private:
		double steer;
		double confidence;
		int crosshair_x;
		int crosshair_y;
};

#endif
