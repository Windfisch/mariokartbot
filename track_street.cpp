#include <iostream>

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


   cv::Mat gray;          // current gray-level image
   cv::Mat gray_prev;       // previous gray-level image
   // tracked features from 0->1
   std::vector<cv::Point2f> points[2];
   // initial position of tracked points
   std::vector<cv::Point2f> initial;
   std::vector<cv::Point2f> features; // detected features
   int max_count=500;      // maximum number of features to detect
   double qlevel=0.01;     // quality level for feature detection
   double minDist=3.3;    // min distance between two points
   std::vector<uchar> status; // status of tracked features
   std::vector<float> err;     // error in tracking



// feature point detection
void detectFeaturePoints() {
   // detect the features
   cv::goodFeaturesToTrack(gray, // the image
      features,   // the output detected features
      max_count, // the maximum number of features
      qlevel,     // quality level
      minDist);   // min distance between two features
}

// determine if new points should be added
bool addNewPoints() {
   // if too few points
   return points[0].size()<=300;
}

// determine which tracked point should be accepted
bool acceptTrackedPoint(int i) {
   return status[i];/* &&
      // if point has moved
      (abs(points[0][i].x-points[1][i].x)+
      (abs(points[0][i].y-points[1][i].y))>0);*/
}

// handle the currently tracked points
void handleTrackedPoints(cv:: Mat &frame,
                         cv:: Mat &output) {
   // for all tracked points
   for(int i= 0; i < points[1].size(); i++ ) {
      // draw line and circle
      cv::line(output,
               points[0][i], // initial position
               points[1][i],// new position
               cv::Scalar(255,255,255));
      cv::circle(output, points[1][i], 2,
                 cv::Scalar(255,255,255),-1);
   }
}


int main(int argc, char** argv)
{
if (argc<2) {printf("usage: %s videofile [scale]\n",argv[0]); exit(1);}
  VideoCapture capture(argv[1]);
  if (!capture.isOpened())
  {
    cout << "couldn't open file" << endl;
    exit(1);
  }

//VideoCapture capture(0);

//  capture.set(CV_CAP_PROP_FRAME_WIDTH, 320);
//  capture.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
//  capture.set(CV_CAP_PROP_FPS, 15);

//Mat tmp; for (int i=0;i<1000;i++) capture.read(tmp);

namedWindow("input");
namedWindow("output");

while(1)
{
  Mat frame;
  capture.read(frame);
  
  Mat output;
  
   // convert to gray-level image
   cv::cvtColor(frame, gray, CV_BGR2GRAY);
   frame.copyTo(output);
   // 1. if new feature points must be added
   if(addNewPoints())
   {
      // detect feature points
      detectFeaturePoints();


      // add the detected features to
      // the currently tracked features
      points[0].insert(points[0].end(),
                        features.begin(),features.end());
      initial.insert(initial.end(),
                      features.begin(),features.end());

   }
   // for first image of the sequence
   if(gray_prev.empty())
        gray.copyTo(gray_prev);
   // 2. track features
   cv::calcOpticalFlowPyrLK(
      gray_prev, gray, // 2 consecutive images
      points[0], // input point positions in first image
      points[1], // output point positions in the 2nd image
      status,    // tracking success
      err);      // tracking error
   // 2. loop over the tracked points to reject some
   int k=0;
   for( int i= 0; i < points[1].size(); i++ ) {
      // do we keep this point?
      if (acceptTrackedPoint(i)) {
         // keep this point in vector
         initial[k]= initial[i];
         points[0][k] = points[0][i];
         points[1][k++] = points[1][i];
      }
    }
    // eliminate unsuccesful points
    points[0].resize(k);
    points[1].resize(k);
  initial.resize(k);
  // 3. handle the accepted tracked points
  handleTrackedPoints(frame, output);
  
  
  //// 4. current points and image become previous ones
  //std::swap(points[1], points[0]);
  
  // 4. extrapolate movement
  for (int i=0;i<points[0].size();i++)
  {
	  points[0][i].x = points[1][i].x+(points[1][i].x-points[0][i].x);
	  points[0][i].y = points[1][i].y+(points[1][i].y-points[0][i].y);
  }
  
  
  
  cv::swap(gray_prev, gray);
  
  
  if (argc==3)
  {
	  Mat out2;
	  
	  pyrUp(output, out2, Size(output.cols*2, output.rows*2));
	  
	  imshow("input",frame);
	  imshow("output", out2);
	}
	else
{
		  imshow("input",frame);
	  imshow("output", output);
}	
  
  waitKey(1000/30.);
}

}
