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
