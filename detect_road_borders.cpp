#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


int find_intersection_index(const vector<Point>& pts, int* i1, int* i2, int x0, int y0, int x1, int y1) // bresenham aus der dt. wikipedia
// returns: 0 if the only intersection is the last point, 1 if there are more intersections
// in the latter case, the first two indexes are stored in *i1 and *i2
{
  int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = dx+dy, e2; /* error value e_xy */
 
  int n_intersections=0;
  for(;;){  /* loop */
    
    //setPixel(x0,y0);
    for (int i=0; i<pts.size();i++)
    {
		if (abs(pts[i].x-x0)+abs(pts[i].y-y0)<=1) // found intersection?
		{
			if (n_intersections==0) { *i1=i; n_intersections=1; break; }
			else if (abs(*i1-i)>5)  { *i2=i; return 1; }
		}
	}
    
    
    if (x0==x1 && y0==y1) break;
    e2 = 2*err;
    if (e2 > dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
    if (e2 < dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
  }
  
  return 0;
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
		
		int area_index=1; // "0" means "no area"
		for (int row = 0; row<thres.rows; row++)
		{
			uchar* data=thres.ptr<uchar>(row);

			for (int col=0; col<thres.cols;col++)
			{
				if (*data==255)
				{
					floodFill(thres, Point(col,row), area_index);
					area_index++;
				}

				data++;
			}
		}
		
		
		int* area_cnt = new int[area_index-1];
		int total_area_cnt=0;
		for (int i=0;i<area_index-1;i++) area_cnt[i]=0;
		
		for (int row = 0; row<thres.rows; row++)
		{
			uchar* data=thres.ptr<uchar>(row);

			for (int col=0; col<thres.cols;col++)
			{
				if (*data)
				{
					area_cnt[*data-1]++;
					total_area_cnt++;
				}
				
				data++;
			}
		}


		/*
		// Das ist nur zum schönsein. man wird einfach den größten area_cnt nehmen wollen und den rest nullen.
		for (int row = 0; row<thres.rows; row++)
		{
			uchar* data=thres.ptr<uchar>(row);

			for (int col=0; col<thres.cols;col++)
			{
				if (*data)
				{
					long long tmp = (long long )30000*(long)area_cnt[*data-1]/(long)total_area_cnt + 64;
					if (tmp>200) tmp=255;
					*data=tmp;
				}
				
				data++;
			}
		}
		*/
		
		
		// finde die größte und zweitgrößte fläche
		int maxi=0, maxa=area_cnt[0], maxi2=-1;
		for (int i=1;i<area_index-1;i++)
		{
			if (area_cnt[i]>maxa)
			{
				maxa=area_cnt[i];
				maxi2=maxi;
				maxi=i;
			}
		}
		
		
		// lösche alle bis auf die größte fläche
		for (int row = 0; row<thres.rows; row++)
		{
			uchar* data=thres.ptr<uchar>(row);

			for (int col=0; col<thres.cols;col++)
			{
				if (*data)
				{
					if (*data!=maxi+1) *data=0; else *data=255;
				}
				data++;
			}
		}
		
		
		
		
		dilate(thres, tmp, erode_kernel);
		erode(tmp, thres, erode_kernel);


		
		Mat thres_tmp, thres_tmp_;
		thres.copyTo(thres_tmp);
		//thres_tmp=thres_tmp_.rowRange(0, 0.6*thres_tmp_.rows );
		
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		findContours(thres_tmp, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE, Point(0, 0));
		/// Draw contours
		Mat drawing = Mat::zeros( thres_tmp.size(), CV_8UC3 );
		
		//drawContours( drawing, contours, -1, Scalar(250,0,0) , 2,8, hierarchy);
		
		for( int i = 0; i< contours.size(); i++ )
		{
		  //if (hierarchy[i][3]<0) // no parent
		  Scalar color = Scalar( 255 ,(hierarchy[i][3]<0)?255:0, (hierarchy[i][3]<0)?255:0 );
		  drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
		}

		for( int i = 0; i< contours.size(); i++ )
		{
			if (hierarchy[i][3]<0)
			{
				int lowy=0, lowj=-1;
				int highy=drawing.rows;
				
				for (int j=0;j<contours[i].size(); j++)
				{
					if (contours[i][j].y > lowy)
					{
						lowy=contours[i][j].y;
						lowj=j;
					}
					if (contours[i][j].y < highy)
						highy=contours[i][j].y;
				}
				
				if (lowj!=-1)
				{
					std::rotate(contours[i].begin(),contours[i].begin()+lowj,contours[i].end());
					
					int j;
					for (j=0;j<contours[i].size();j++)
						if (contours[i][j].y < contours[i][0].y-1) break;
					for (;j<contours[i].size();j++)
						circle(drawing, contours[i][j], 2, Scalar(	0,255-( j *255/contours[i].size()),( j *255/contours[i].size())));
					
					line(drawing, Point(0,highy), Point(drawing.cols,highy), Scalar(127,127,127));
					
					#define SMOOTHEN_BOTTOM 25
					#define SMOOTHEN_MIDDLE 10
					
					
					for (j=0;j<contours[i].size();j++)
						if (contours[i][j].y < contours[i][0].y-1) break;
		        
					int init_j=j;
					
					double* angles = new double[contours[i].size()];
					
					for (;j<contours[i].size();j++)
					{
						int smoothen;
						if (contours[i][j].y - thres.rows/2	 < 0)
							smoothen=SMOOTHEN_MIDDLE;
						else
							smoothen= SMOOTHEN_MIDDLE + (SMOOTHEN_BOTTOM-SMOOTHEN_MIDDLE) * (contours[i][j].y - thres.rows/2) / (thres.rows/2);
		        
						int j1=(j+smoothen); if (j1 >= contours[i].size()) j1-=contours[i].size();
						int j2=(j-smoothen); if (j2 < 0) j2+=contours[i].size();
		        
		        
						angles[j] = atan2(contours[i][j1].y - contours[i][j2].y, contours[i][j1].x - contours[i][j2].x) * 180 /3.141592654;
						if (angles[j]<0) angles[j]+=360;
						int r,g,b;
						hue2rgb(angles[j], &r, &g, &b);
		        
						circle(drawing, contours[i][j], 2, Scalar(b,g,r));
		        
						int x=drawing.cols-drawing.cols*(j-init_j)/(contours[i].size()-init_j);
						line(drawing,Point(x,0), Point(x,10), Scalar(b,g,r));
					}
			
					#define ANG_SMOOTH 9
					double* angle_derivative = new double[contours[i].size()];
					for (j=init_j+ANG_SMOOTH;j<contours[i].size()-ANG_SMOOTH;j++)
					{
						int x=drawing.cols-drawing.cols*(j-init_j)/(contours[i].size()-init_j);
						double ang_diff = angles[j+ANG_SMOOTH]-angles[j-ANG_SMOOTH];
						
						while (ang_diff<0) ang_diff+=360;
						while (ang_diff>=360) ang_diff-=360;
						if (ang_diff>=180) ang_diff=360-ang_diff;
						
						int c=abs(20* ang_diff/ANG_SMOOTH);
						Scalar col=(c<256) ? Scalar(255-c,255-c,255) : Scalar(255,0,255);
						line(drawing, Point(x,12), Point(x,22), col);
						
						int y=25+40-2*ang_diff/ANG_SMOOTH;
						
						angle_derivative[j] = (double)ang_diff / ANG_SMOOTH;
						
						double quality = ((double)ang_diff/ANG_SMOOTH) * linear(contours[i][j].y, highy, 1.0, highy+ (drawing.rows-highy)/10, 0.0, true) 
						                                               * linear( abs(drawing.cols/2 - contours[i][j].x), 0.8*drawing.cols/2, 1.0, drawing.cols/2, 0.6, true);
						//int y2=25+40+100-5*quality;
						
						line(drawing, Point(x,y), Point(x,y), Scalar(255,255,255));
						//line(drawing, Point(x,25+40+100), Point(x,25+40+100), Scalar(127,127,127));
						//line(drawing, Point(x,y2), Point(x,y2), Scalar(255,255,255));
						
						circle(drawing, contours[i][j], 2, col);
					}
					
					for (int j=init_j; j<init_j+ANG_SMOOTH; j++) angle_derivative[j]=angle_derivative[init_j+ANG_SMOOTH];
					for (int j=contours[i].size()-ANG_SMOOTH; j<contours[i].size(); j++) angle_derivative[j]=angle_derivative[contours[i].size()-ANG_SMOOTH-1];
					
					double lastmax=-999999;
					double bestquality=0.0;
					double bestquality_max=0.0;
					int bestquality_j=-1;
					int bestquality_width=-1;
					
					#define MAX_HYST 0.8
					for (int j=3;j<contours[i].size()-3;j++)
					{
						if (angle_derivative[j] > lastmax) lastmax=angle_derivative[j];
						if (angle_derivative[j] < MAX_HYST*lastmax && angle_derivative[j+1] < MAX_HYST*lastmax && angle_derivative[j+2] < MAX_HYST*lastmax)
						{
							int j0=-1;
							for (j0=j-1; j0>=0; j0--)
								if (angle_derivative[j0] < MAX_HYST*lastmax && angle_derivative[j0-1] < MAX_HYST*lastmax && angle_derivative[j0-2] < MAX_HYST*lastmax)
									break;
							
							if (lastmax > 5)
							{
								// the maximum area goes from j0 to j
								int x=drawing.cols-drawing.cols*((j+j0)/2-init_j)/(contours[i].size()-init_j);
								
								double quality = ((double)angle_derivative[(j+j0)/2]) * linear(contours[i][j].y, highy, 1.0, highy+ (drawing.rows-highy)/10, 0.0, true) 
																			   * linear( abs(drawing.cols/2 - contours[i][j].x), 0.8*drawing.cols/2, 1.0, drawing.cols/2, 0.6, true);
								
								if (quality>bestquality)
								{
									bestquality=quality;
									bestquality_max=lastmax;
									bestquality_j=(j+j0)/2;
									bestquality_width=j-j0;
								}
								
								line(drawing, Point(x,25+40-3*quality), Point(x, 25+40), Scalar(0,255,0));
								circle(drawing, contours[i][(j+j0)/2], 1, Scalar(128,0,0));
							}
							lastmax=-999999;
						}
					}
					
					circle(drawing, contours[i][bestquality_j], 3, Scalar(255,255,0));
					circle(drawing, contours[i][bestquality_j], 2, Scalar(255,255,0));
					circle(drawing, contours[i][bestquality_j], 1, Scalar(255,255,0));
					circle(drawing, contours[i][bestquality_j], 0, Scalar(255,255,0));

					int antisaturation = 200-(200* bestquality/10.0);
					if (antisaturation<0) antisaturation=0;
					for (int j=0;j<bestquality_j-bestquality_width/2;j++)
						circle(drawing, contours[i][j], 2, Scalar(255,antisaturation,255));
					for (int j=bestquality_j+bestquality_width/2;j<contours[i].size();j++)
						circle(drawing, contours[i][j], 2, Scalar(antisaturation,255,antisaturation));
						
					line(drawing, contours[i][bestquality_j], Point(drawing.cols/2, drawing.rows-drawing.rows/5), Scalar(0,255,255));
					
					int i1,i2;
					if (find_intersection_index(contours[i], &i1,&i2, drawing.cols/2, drawing.rows-drawing.rows/5, contours[i][bestquality_j].x, contours[i][bestquality_j].y))
					{
						circle(drawing, contours[i][i1], 2, Scalar(0,0,0));
						circle(drawing, contours[i][i1], 1, Scalar(0,0,0));
						circle(drawing, contours[i][i2], 2, Scalar(0,0,0));
						circle(drawing, contours[i][i2], 1, Scalar(0,0,0));

						int xx;
						if (i1 < bestquality_j) // im pinken bereich, also zu weit rechts
						{
							for (xx = contours[i][bestquality_j].x; xx>=0; xx--)
							{
								int result = find_intersection_index(contours[i], &i1,&i2, drawing.cols/2, drawing.rows-drawing.rows/5, xx, contours[i][bestquality_j].y);
								if (!result)
									break;
								if (i1>=bestquality_j /*&& contours[i][i1].y<contours[i][bestquality_j].y*/)
								{
									xx++; // undo last step
									break;
								}
							}
						}
						else if (i1 > bestquality_j) // im grünen bereich, also zu weit links
						{
							for (xx = contours[i][bestquality_j].x; xx<drawing.cols; xx++)
							{
								int result = find_intersection_index(contours[i], &i1,&i2, drawing.cols/2, drawing.rows-drawing.rows/5, xx, contours[i][bestquality_j].y);
								if (!result)
									break;
								if (i1<=bestquality_j /*&& contours[i][i1].y<contours[i][bestquality_j].y*/)
								{
									xx--; // undo last step
									break;
								}
							}
						}
						
						line(drawing, Point(xx, contours[i][bestquality_j].y), Point(drawing.cols/2, drawing.rows-drawing.rows/5), Scalar(127,255,255));
					}
					
					cout << "bestquality_width="<<bestquality_width <<",\tquality="<<bestquality<<",\t"<<"raw max="<<bestquality_max
						 <<endl<<endl<<endl<<endl;
					
					
					delete [] angle_derivative;
					delete [] angles;
				}
			}
		}

		/*Point midpoint=Point(drawing.cols/2, 250); // farbkreis
		for (int a=0; a<360; a++)
		{
			double s=sin((double)a*3.141592654/180.0);
			double c=cos((double)a*3.141592654/180.0);
			int r,g,b;
			hue2rgb(a, &r, &g, &b);
			line(drawing,midpoint-Point(c*5, s*5), midpoint-Point(c*30, s*30),Scalar(b,g,r) );
		}*/


		
		
		area_history_sum-=area_history[area_history_ptr];
		area_history[area_history_ptr]=area_cnt[maxi];
		area_history_sum+=area_cnt[maxi];
		area_history_ptr=(area_history_ptr+1)%AREA_HISTORY;
		int prev_area=area_history_sum/AREA_HISTORY;
		
		cout << "\r\e[2A area = "<<area_cnt[maxi]<<", \tvize = "<<((maxi2!=-1)?area_cnt[maxi2]:-1)<<", \tratio="<< ((maxi2!=-1)?(area_cnt[maxi]/area_cnt[maxi2]):-1 )<<"                  \n" <<
		        "prev area = "<<prev_area<<",\tchange="<< (100*area_cnt[maxi]/prev_area -100) <<"%\n"<<flush;
		
		if (maxi!=-1 && maxi2!=-1 && (area_cnt[maxi]/area_cnt[maxi2]) < 10)
		{
			cout << "\nALERT: possibly split road!\n\n\n" << flush;
			alertcnt=0;
		}
		
		if (abs(100*area_cnt[maxi]/prev_area -100) >=10)
		{
			cout << "\nALERT: too fast road area change!\n\n\n" << flush;
			alertcnt=0;
		}

		alertcnt++;
		if (alertcnt == 20) cout << "\n\n\n\n\n\n------------------------\n\n\n";

		cout << "frame #"<<frameno<<endl;

		imshow("input",thres);
		imshow("contours",drawing);
//		waitKey(100);
		waitKey();
		
		frameno++;
	}
  
}
