#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


int find_intersection_index(int x0, int y0, int x1, int y1, int** contour_map, bool stop_at_endpoint=true) // bresenham aus der dt. wikipedia
// returns: the point's index where the intersection happened, or a negative number if no intersection.
{
  int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = dx+dy, e2; /* error value e_xy */
 
  for(;;)
  {
    
    //setPixel(x0,y0);
    if (contour_map[x0][y0]>0) return contour_map[x0][y0]; // found intersection?
    if (contour_map[x0][y0+1]>0) return contour_map[x0][y0+1];
    if (contour_map[x0+1][y0]>0) return contour_map[x0+1][y0];
    
		
    
    
    if (stop_at_endpoint && x0==x1 && y0==y1) break;
    e2 = 2*err;
    if (e2 > dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
    if (e2 < dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
  }
  
  return -1;
}


Mat circle_mat(int radius)
{
	Mat result(radius*2+1, radius*2+1, CV_8U);
	for (int x=0; x<=result.cols/2; x++)
		for (int y=0; y<=result.rows/2; y++)
		{
			unsigned char& p1 = result.at<unsigned char>(result.cols/2 + x, result.rows/2 + y);
			unsigned char& p2 = result.at<unsigned char>(result.cols/2 - x, result.rows/2 + y);
			unsigned char& p3 = result.at<unsigned char>(result.cols/2 + x, result.rows/2 - y);
			unsigned char& p4 = result.at<unsigned char>(result.cols/2 - x, result.rows/2 - y);
			
			if ( x*x + y*y < radius*radius )
				p1=p2=p3=p4=255;
			else
				p1=p2=p3=p4=0;
		}
		
	return result;
}

void hue2rgb(float hue, int* r, int* g, int* b)
{
	double ff;
	int i;


	if (hue >= 360.0) hue = 0.0;
	hue /= 60.0;
	i = (int)hue;
	ff = hue - i;
	int x=ff*255;

	switch(i) {
	case 0:
		*r = 255;
		*g = x;
		*b = 0;
		break;
	case 1:
		*r = 255-x;
		*g = 255;
		*b = 0;
		break;
	case 2:
		*r = 0;
		*g = 255;
		*b = x;
		break;

	case 3:
		*r = 0;
		*g = 255-x;
		*b = 255;
		break;
	case 4:
		*r = x;
		*g = 0;
		*b = 255;
		break;
	case 5:
	default:
		*r = 255;
		*g = 0;
		*b = 255-x;
		break;
	}
}

double linear(double x, double x1, double y1, double x2, double y2, bool clip=false, double clipmin=INFINITY, double clipmax=INFINITY)
{	
	if (clipmin==INFINITY) clipmin=y1;
	if (clipmax==INFINITY) clipmax=y2;
	if (clipmin>clipmax) { int tmp=clipmin; clipmin=clipmax; clipmax=tmp; }
	
	double result = (y2-y1)*(x-x1)/(x2-x1)+y1;
	
	if (clip)
	{
		if (result>clipmax) return clipmax;
		else if (result<clipmin) return clipmin;
		else return result;
	}
	else
		return result;
}


int annotate_regions(Mat img) //img is treated as black/white (0, !=0)
// changes img, and returns the number of found areas
{
	int region_index=1; // "0" means "no area"
	for (int row = 0; row<img.rows; row++)
	{
		uchar* data=img.ptr<uchar>(row);

		for (int col=0; col<img.cols;col++)
		{
			if (*data==255)
			{
				floodFill(img, Point(col,row), region_index);
				region_index++;
			}

			data++;
		}
	}
	return region_index-1;
}

Mat nicely_draw_regions(Mat annotated, int* area_cnt, int total_area_cnt, int largest_region)
{
	Mat result;
	annotated.copyTo(result);
	
	// Das ist nur zum schönsein.
	for (int row=0; row<result.rows; row++)
	{
		uchar* data=result.ptr<uchar>(row);

		for (int col=0; col<result.cols;col++)
		{
			if (*data)
			{
				long long tmp = (long long)30000*(long)area_cnt[*data-1]/(long)total_area_cnt + 64;
				if (tmp>200) tmp=200;
				if (*data==largest_region) tmp=255;
				*data=tmp;
			}
			
			data++;
		}
	}
}

double only_retain_largest_region(Mat img, int* size)
// img is a binary image
// in *size, if non-NULL, the size of the largest area is stored.
// returns: ratio between the second-largest and largest region
//          0.0 means "that's the only region", 1.0 means "both had the same size!"
// can be interpreted as 1.0 - "confidence".
{
		int n_regions = annotate_regions(img);
		
		// calculate the area of each region
		int* area_cnt = new int[n_regions];
		for (int i=0;i<n_regions;i++) area_cnt[i]=0;
		int total_area_cnt=0;
		
		for (int row=0; row<img.rows; row++)
		{
			uchar* data=img.ptr<uchar>(row);

			for (int col=0; col<img.cols;col++)
			{
				if (*data)
				{
					area_cnt[*data-1]++;
					total_area_cnt++;
				}
				
				data++;
			}
		}


		
		
		// finde die größte und zweitgrößte fläche
		int maxi=0, maxa=area_cnt[0], maxi2=-1;
		for (int i=1;i<n_regions;i++)
		{
			if (area_cnt[i]>maxa)
			{
				maxa=area_cnt[i];
				maxi2=maxi;
				maxi=i;
			}
		}
		
		
		// lösche alle bis auf die größte fläche
		for (int row = 0; row<img.rows; row++)
		{
			uchar* data=img.ptr<uchar>(row);

			for (int col=0; col<img.cols;col++)
			{
				if (*data)
				{
					if (*data!=maxi+1) *data=0; else *data=255;
				}
				data++;
			}
		}
		
		
		if (size) *size=area_cnt[maxi];
		
		if (maxi2==-1) return 0;
		else           return (double)area_cnt[maxi2]/(double)area_cnt[maxi];
}

#define SMOOTHEN_BOTTOM 25
#define SMOOTHEN_MIDDLE 10
#define ANG_SMOOTH 9
void find_steering_point(Mat orig_img, int** contour_map, Mat& drawing) // orig_img is a binary image
{
	Mat img;
	orig_img.copyTo(img); // this is needed because findContours destroys its input.
	
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	findContours(img, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));
	
	// Draw contours
	drawing = Mat::zeros( img.size(), CV_8UC3 );
	
	for( int i = 0; i< contours.size(); i++ )
	{
		Scalar color;


		if (hierarchy[i][3]<0) // no parent
			color=Scalar(255,255,255);
		else // this is a sub-contour which is actually irrelevant for our needs
			color=Scalar(255,0,0);

		drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
	}



	for (int road_contour_idx=0; road_contour_idx<contours.size(); road_contour_idx++ )
		if (hierarchy[road_contour_idx][3]<0) // this will be true for exactly one road_contour_idx.
		{		
			vector<Point>& contour = contours[road_contour_idx]; // just a shorthand
			
			if (!contour.size()>0) continue; // should never happen.
			
			
			// find highest and lowest contour point. (where "low" means high y-coordinate)
			int low_y=0, low_idx=-1;
			int high_y=drawing.rows;
			
			for (int j=0;j<contour.size(); j++)
			{
				if (contour[j].y > low_y)
				{
					low_y=contour[j].y;
					low_idx=j;
				}
				if (contour[j].y < high_y)
					high_y=contour[j].y;
			}
			
			assert(low_idx!=0);
			
			
			
			

			// make the contour go "from bottom upwards and then downwards back to bottom".
			std::rotate(contour.begin(),contour.begin()+low_idx,contour.end());

			// create contour map
			for (int j=0;j<img.cols;j++) // zero it
				memset(contour_map[j],0,img.rows*sizeof(**contour_map));
			for (int j=0;j<contour.size(); j++) // fill it
				contour_map[contour[j].x][contour[j].y]=j;

			
			line(drawing, Point(0,high_y), Point(drawing.cols,high_y), Scalar(127,127,127));
			
			
			
		
			int first_nonbottom_idx = 0;
			for (;first_nonbottom_idx<contour.size();first_nonbottom_idx++)
				if (contour[first_nonbottom_idx].y < contour[0].y-1) break;
		
			
			// calculate directional angle for each nonbottom contour point
			double* angles = new double[contour.size()];
			for (int j=first_nonbottom_idx; j<contour.size(); j++)
			{
				int smoothen=linear(contour[j].y, img.rows/2  ,SMOOTHEN_MIDDLE, img.rows,SMOOTHEN_BOTTOM, true);
				
		
				// calculate left and right point for the difference quotient, possibly wrap.
				int j1=(j+smoothen); while (j1 >= contour.size()) j1-=contour.size();
				int j2=(j-smoothen); while (j2 < 0) j2+=contour.size();
		
		
				// calculate angle, adjust it to be within [0, 360)
				angles[j] = atan2(contour[j1].y - contour[j2].y, contour[j1].x - contour[j2].x) * 180/3.141592654;
				if (angles[j]<0) angles[j]+=360;
				
				
				// irrelevant drawing stuff
				int r,g,b;
				hue2rgb(angles[j], &r, &g, &b);
				circle(drawing, contour[j], 2, Scalar(b,g,r));
				int x=drawing.cols-drawing.cols*(j-first_nonbottom_idx)/(contour.size()-first_nonbottom_idx);
				line(drawing,Point(x,0), Point(x,10), Scalar(b,g,r));
			}
	
			// calculate derivative of angle for each nonbottom contour point
			double* angle_derivative = new double[contour.size()];
			for (int j=first_nonbottom_idx+ANG_SMOOTH; j<contour.size()-ANG_SMOOTH; j++)
			{
				// calculate angular difference, adjust to be within [0;360) and take the shorter way.
				double ang_diff = angles[j+ANG_SMOOTH]-angles[j-ANG_SMOOTH];
				while (ang_diff<0) ang_diff+=360;
				while (ang_diff>=360) ang_diff-=360;
				if (ang_diff>=180) ang_diff=360-ang_diff;
				
				angle_derivative[j] = (double)ang_diff / ANG_SMOOTH;
				
				

				
				// irrelevant drawing stuff
				int x=drawing.cols-drawing.cols*(j-first_nonbottom_idx)/(contour.size()-first_nonbottom_idx);
				int c=abs(20* ang_diff/ANG_SMOOTH);
				Scalar col=(c<256) ? Scalar(255-c,255-c,255) : Scalar(255,0,255);
				line(drawing, Point(x,12), Point(x,22), col);
				
				int y=25+40-2*ang_diff/ANG_SMOOTH;
				line(drawing, Point(x,y), Point(x,y), Scalar(255,255,255));
				circle(drawing, contour[j], 2, col);
			}
			
			// poorly extrapolate the ANG_SMOOTH margins
			for (int j=first_nonbottom_idx; j<first_nonbottom_idx+ANG_SMOOTH; j++) angle_derivative[j]=angle_derivative[first_nonbottom_idx+ANG_SMOOTH];
			for (int j=contour.size()-ANG_SMOOTH; j<contour.size(); j++) angle_derivative[j]=angle_derivative[contour.size()-ANG_SMOOTH-1];
			
			
			
			
			
			double lastmax=-999999;
			double bestquality=0.0;
			double bestquality_max=0.0;
			int bestquality_j=-1;
			int bestquality_width=-1;
			
			#define MAX_HYST 0.8
			// search for "maximum regions"; i.e. intervals [a,b] with
			// ang_deriv[i] >= MAX_HYST * max_deriv \forall i \in [a,b] and
			// ang_deriv[a-1,2,3], ang_deriv[b+1,2,3] < MAX_HYST * max_deriv
			// where max_deriv = max_{i \in [a,b]} ang_deriv[i];
			for (int j=3; j<contour.size()-3; j++)
			{
				// search forward for a maximum, and the end of a maximum region.
				if (angle_derivative[j] > lastmax) lastmax=angle_derivative[j];
				
				if (angle_derivative[j]   < MAX_HYST*lastmax && // found the end of the max. region
					angle_derivative[j+1] < MAX_HYST*lastmax && 
					angle_derivative[j+2] < MAX_HYST*lastmax)
				{
					if (lastmax > 5) // threshold the maximum.
					{
						// search backward for the begin of that maximum region
						int j0;
						for (j0=j-1; j0>=0; j0--)
							if (angle_derivative[j0] < MAX_HYST*lastmax &&
								angle_derivative[j0-1] < MAX_HYST*lastmax &&
								angle_derivative[j0-2] < MAX_HYST*lastmax)
								break;
						
						// maximum region is [j0; j]

						double median_of_max_region = (double)angle_derivative[(j+j0)/2];
						
						// calculate quality of that maximum. quality is high, if
						// 1) the maximum has a high value AND
						// 2) the corresponding point's y-coordinates are near the top image border AND
						// 3) the corresponding point's x-coordinates are near the middle of the image, if in doubt
						int middle_x = drawing.cols/2;
						int distance_from_middle_x = abs(drawing.cols/2 - contour[j].x);
						double quality = median_of_max_region
										  *  linear( contour[j].y,               high_y,       1.0,       high_y+ (drawing.rows-high_y)/10, 0.0,   true)  // excessively punish points far away from the top border
										  *  linear( distance_from_middle_x,     0.8*middle_x, 1.0,       middle_x,                         0.6,   true); // moderately punish point far away from the x-middle.
						
						// keep track of the best point
						if (quality>bestquality)
						{
							bestquality=quality;
							bestquality_max=lastmax;
							bestquality_j=(j+j0)/2;
							bestquality_width=j-j0;
						}
						
						
						// irrelevant drawing stuff
						int x=drawing.cols-drawing.cols*((j+j0)/2-first_nonbottom_idx)/(contour.size()-first_nonbottom_idx);
						line(drawing, Point(x,25+40-3*quality), Point(x, 25+40), Scalar(0,255,0));
						circle(drawing, contour[(j+j0)/2], 1, Scalar(128,0,0));
					}
					
					lastmax=-999999; // reset lastmax, so the search can go on
				}
			}
			
			// now bestquality_j holds the index of the point with the best quality.
			
			circle(drawing, contour[bestquality_j], 3, Scalar(255,255,0));
			circle(drawing, contour[bestquality_j], 2, Scalar(255,255,0));
			circle(drawing, contour[bestquality_j], 1, Scalar(255,255,0));
			circle(drawing, contour[bestquality_j], 0, Scalar(255,255,0));

			int antisaturation = 200-(200* bestquality/10.0);
			if (antisaturation<0) antisaturation=0;
			for (int j=0;j<bestquality_j-bestquality_width/2;j++)
				circle(drawing, contour[j], 2, Scalar(255,antisaturation,255));
			for (int j=bestquality_j+bestquality_width/2;j<contour.size();j++)
				circle(drawing, contour[j], 2, Scalar(antisaturation,255,antisaturation));
				
			line(drawing, contour[bestquality_j], Point(drawing.cols/2, drawing.rows-drawing.rows/5), Scalar(0,64,64));
			
			
			
			
			// TODO: the below code is crappy, slow, and uses brute force. did i mention it's crappy and slow?
			
			int intersection = find_intersection_index(drawing.cols/2,           drawing.rows-drawing.rows/5, 
													   contour[bestquality_j].x, contour[bestquality_j].y,        contour_map);
			if (intersection<0)
			{
				cout << "THIS SHOULD NEVER HAPPEN" << endl;
			}
			else
			{
				circle(drawing, contour[intersection], 2, Scalar(0,0,0));
				circle(drawing, contour[intersection], 1, Scalar(0,0,0));

				int xx=contour[bestquality_j].x;
				int lastheight=-1;
				if (intersection < bestquality_j) // too far on the right == intersecting the right border
				{
					// rotate the line to the left till it gets better
					for (; xx>=0; xx--)
					{
						int intersection2 = find_intersection_index(drawing.cols/2, drawing.rows-drawing.rows/5, xx, contour[bestquality_j].y, contour_map);
						if (intersection2<0) // won't happen anyway
							break;
						
						if (intersection2>=bestquality_j) // now we intersect the opposite (=left) border
						{
							if (contour[intersection2].y>=lastheight) // we intersect at a lower = worse point?
								xx++;                                 // then undo last step
								
							break;
						}
						lastheight=contour[intersection2].y;
					}
				}
				else if (intersection > bestquality_j) // too far on the left == intersecting the left border
				{
					// rotate the line to the right till it gets better
					for (; xx<drawing.cols; xx++)
					{
						int intersection2 = find_intersection_index(drawing.cols/2, drawing.rows-drawing.rows/5, xx, contour[bestquality_j].y, contour_map);
						if (intersection2<0)// won't happen anyway
							break;
						
						if (intersection2<=bestquality_j) // now we intersect the opposite (=right) border
						{
							if (contour[intersection2].y>=lastheight) // we intersect at a lower = worse point?
								xx--;                                 // then undo last step
								
							break;
						}
						lastheight=contour[intersection2].y;
					}
				}
				// else // we directly met the bestquality point, i.e. where we wanted to go to.
					// do nothing
				
				// drawing stuff:
				// now find the intrsection point of our line with the contour (just for drawing it nicely)
				int steering_point = find_intersection_index(drawing.cols/2, drawing.rows-drawing.rows/5, xx, contour[bestquality_j].y, contour_map, false);
				if (steering_point>=0) // should be always true
					line(drawing, contour[steering_point], Point(drawing.cols/2, drawing.rows-drawing.rows/5), Scalar(0,255,255));
			}
			
			cout << "bestquality_width="<<bestquality_width <<",\tquality="<<bestquality<<",\t"<<"raw max="<<bestquality_max
				 <<endl<<endl<<endl<<endl;
			
			
			delete [] angle_derivative;
			delete [] angles;
		}

	/*Point midpoint=Point(drawing.cols/2, 250); // color circle, uncomment if you want to understand the colored bar on the top border.
	for (int a=0; a<360; a++)
	{
		double s=sin((double)a*3.141592654/180.0);
		double c=cos((double)a*3.141592654/180.0);
		int r,g,b;
		hue2rgb(a, &r, &g, &b);
		line(drawing,midpoint-Point(c*5, s*5), midpoint-Point(c*30, s*30),Scalar(b,g,r) );
	}*/
}

#define AREA_HISTORY 10

int alertcnt=21;
int main(int argc, char* argv[])
{
	if (argc!=2) {printf("usage: %s videofile\n",argv[0]); exit(1);}
	VideoCapture capture(argv[1]);
	
	if (!capture.isOpened())
	{
		cout << "couldn't open file" << endl;
		exit(1);
	}
	
	Mat erode_kernel=circle_mat(10);
	
	
	Mat frame;
	capture >> frame;
	
	

	
	
	Mat thres(frame.rows, frame.cols, CV_8UC1);
	Mat tmp(frame.rows, frame.cols, CV_8UC1);
	int** contour_map;
	contour_map=new int*[frame.cols];
	for (int i=0;i<frame.cols;i++)
		contour_map[i]=new int[frame.rows];
	
	int area_history[AREA_HISTORY];
	for (int i=0;i<AREA_HISTORY;i++) area_history[i]=1;
	int area_history_ptr=0;
	int area_history_sum=AREA_HISTORY;
	cout << endl<<endl<<endl;
	int frameno=0;
	while (1)
	{
		capture >> frame;
		
		if (frameno<190)
		{
			frameno++;
			continue;
		}
		
		cvtColor(frame, tmp, CV_RGB2GRAY);
		threshold(tmp, thres, 132, 255, THRESH_BINARY);
		dilate(thres,tmp,Mat());
		erode(tmp,thres,Mat());
		erode(thres,tmp,Mat());
		dilate(tmp,thres,Mat());
		
		int area_abs;
		double area_ratio = only_retain_largest_region(thres, &area_abs);
		
		
		dilate(thres, tmp, erode_kernel);
		erode(tmp, thres, erode_kernel);


		Mat drawing;
		find_steering_point(thres, contour_map, drawing);

		
		
		area_history_sum-=area_history[area_history_ptr];
		area_history[area_history_ptr]=area_abs;
		area_history_sum+=area_abs;
		area_history_ptr=(area_history_ptr+1)%AREA_HISTORY;
		int prev_area=area_history_sum/AREA_HISTORY;
		
		cout << "\r\e[2A area = "<<area_abs<<",\tratio="<< area_ratio <<"                  \n" <<
		        "prev area = "<<prev_area<<",\tchange="<< (100*area_abs/prev_area -100) <<"%\n"<<flush;
		
		if (area_ratio>0.1)
		{
			cout << "\nALERT: possibly split road!\n\n\n" << flush;
			alertcnt=0;
		}
		
		if (abs(100*area_abs/prev_area -100) >=10)
		{
			cout << "\nALERT: too fast road area change!\n\n\n" << flush;
			alertcnt=0;
		}

		alertcnt++;
		if (alertcnt == 20) cout << "\n\n\n\n\n\n------------------------\n\n\n";

		cout << "frame #"<<frameno<<endl;

		imshow("input",thres);
		imshow("drawing",drawing);
//		waitKey(100);
		waitKey();
		
		frameno++;
	}
  
}
