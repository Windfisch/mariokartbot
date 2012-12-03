#include <opencv2/opencv.hpp>
#include "road_thresholder.h"
#include "util.h"

using namespace cv;

RoadThresholder::RoadThresholder()
{
	road_0=77;
	road_1=77;
	road_2=77;
	
	erode_2d_small = circle_mat(3);
}
RoadThresholder::RoadThresholder(int r0, int r1, int r2)
{
	road_0=r0;
	road_1=r1;
	road_2=r2;
	
	erode_2d_small = circle_mat(3);
}

Mat RoadThresholder::create_diff_image_and_fill_histogram(const Mat& img, int* hist)
{
	Mat img_diff(img.rows, img.cols, CV_8U);
	for (int i=0; i<256; i++) hist[i]=0; // zero the histogram
	
	// iterate through every pixel of img
	for (int row=0; row<img.rows; row++)
	{
		const uchar* data=img.ptr<uchar>(row);
		uchar* data_out=img_diff.ptr<uchar>(row);
		
		for (int col=0; col<img.cols;col++)
		{
			int diff = (abs((int)data[0] - road_0) + abs((int)data[1] - road_1) + abs((int)data[2] - road_2)) /3;
			if (diff>255) { printf("error, diff is %i\n",diff); diff=255; }
			
			*data_out=diff;
			hist[diff]++;
			
			data+=img.step[1];
			data_out++;
		}
	}
	
	return img_diff;
}

void RoadThresholder::smoothen_histogram(int* hist, int* hist_smooth, int smoothness)
{
	for (int i=0; i<256; i++)
	{
		int sum=0;
		for (int j=-smoothness; j<=smoothness; j++)
		{
			if (i+j < 0 || i+j > 255) continue;
			sum+=hist[i+j];
		}
		hist_smooth[i]=sum;
	}
}


void RoadThresholder::calc_road_color(const Mat& img, const Mat& mask_eroded)
{
	int avg_sum=0;
	int r0=0, r1=0, r2=0;
	for (int row = 0; row<img.rows; row++)
	{
		const uchar* data=img.ptr<uchar>(row);
		const uchar* mask=mask_eroded.ptr<uchar>(row);
		
		for (int col=0; col<img.cols;col++)
		{
			if (*mask)
			{
				avg_sum++;
				r0+=data[0];
				r1+=data[1];
				r2+=data[2];
			}
			
			data+=img.step[1];
			mask++;
		}
	}
	
	if (avg_sum>20)
	{
		road_0=r0/avg_sum;
		road_1=r1/avg_sum;
		road_2=r2/avg_sum;
	}
}

void RoadThresholder::process_image(const Mat& img)
{
	int hist_unsmoothened[256];
	Mat img_diff = create_diff_image_and_fill_histogram(img, hist_unsmoothened);
	
	int hist_smoothened[256];
	smoothen_histogram(hist_unsmoothened, hist_smoothened, 7);
	
	// road has to occupy at least 1% of the image
	int cumul=0;
	int x_begin=0;
	for (x_begin=0;x_begin<256;x_begin++)
	{
		cumul+=hist_unsmoothened[x_begin];
		if (cumul > img.rows*img.cols/100) break;
	}
	
	// find first valley in the histogram, and set the threshold at the way down.
	int hist_max=0;
	int thres=0;
	for (int i=0; i<256; i++)
	{
		if (hist_smoothened[i]>hist_max) hist_max=hist_smoothened[i];
		if ((hist_smoothened[i] < hist_max/2) && (i>x_begin))
		{
			thres=i;
			break;
		}
	}
	

	// drawing stuff
	Mat img_hist(100,256, CV_8U);
	for (int row = 0; row<img_hist.rows; row++)
	{
		uchar* data=img_hist.ptr<uchar>(row);
		
		for (int col=0; col<img_hist.cols;col++)
		{
			*data=255;
			if (col==thres) *data=127;
			if (col==x_begin) *data=0;
			if (hist_smoothened[col] > (img_hist.rows-row)*800) *data=0;
			data++;
		}
	}
	imshow("hist", img_hist);
	
	
	mask_raw = Mat(img_diff.rows, img_diff.cols, img_diff.type());
	threshold(img_diff, mask_raw, thres, 255, THRESH_BINARY_INV);
	
	
	
	
	
	
	
	Mat mask_eroded(mask_raw.rows, mask_raw.cols, mask_raw.type());
	erode(mask_raw, mask_eroded, erode_2d_small);

	calc_road_color(img, mask_eroded); // update our road color
	
	
}

Mat& RoadThresholder::get_road()
{
	return mask_raw;
}
