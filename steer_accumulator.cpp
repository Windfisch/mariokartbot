#include "steer_accumulator.h"

using namespace std;

void SteerAccumulator::add_steerer(SteerIface* steerer)
{
	steerers.push_back(steerer);
}

void SteerAccumulator::process_image(const Mat& img)
{
	for (list<SteerIface*>::iterator it = steerers.begin(); it!=steerers.end(); ++it)
		(*it)->process_image(img);
}

double SteerAccumulator::get_steer_data()
{
	double sum=0;
	for (list<SteerIface*>::iterator it = steerers.begin(); it!=steerers.end(); ++it)
		sum+=(*it)->get_steer_data() * (*it)->get_confidence();
	
	double confidence = get_confidence();
	
	return (confidence==0) ? 0.0 : (sum/confidence);
}

double SteerAccumulator::get_confidence()
{
	double sum=0;
	for (list<SteerIface*>::iterator it = steerers.begin(); it!=steerers.end(); ++it)
		sum+=(*it)->get_confidence();
	return sum;
}
