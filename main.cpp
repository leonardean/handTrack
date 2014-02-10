#include "opencv2/opencv.hpp"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "iostream"
#include <sstream>
#include <string>
#include <tesseract/baseapi.h>
#include <tesseract/strngs.h>

using namespace cv;
using namespace std;

//hand extract based on skin color in YCbCr color space
void skinExtract(const Mat &frame, Mat &skinArea);
//hand extract based on skin color in multiple color space
Mat GetSkin(Mat const &src);
//threshold of color in RGB color space
bool R1(int R, int G, int B);
//threshold of color in YCbCr color space
bool R2(float Y, float Cr, float Cb);
//threshold of color in HSV color space
bool R3(float H, float S, float V);
//calculate distance of two points
double calcDist(Point p1, Point p2);
//find finger tips
vector<Point> findFingers(vector<vector<Point> > contours, int index, Point center);
//analyse password
void passwordAnalysis(Mat frame, Mat skinArea, VideoCapture capture);
//used for point ordering
bool toLeft(Point i, Point j);
//whether a finger presses a key
void press_listener(vector<vector<Point> > fingerTrace, Mat& frame);
//calculate angle of three points
static double angle(Point pt1, Point pt2, Point pt0);
//improve color contrast of an image
void improvecontrast(Mat& im);
//set image into 2 colors
void black_areadetection(Mat& im, int threshold);
//display text at contour center
void setLabel(Mat& im, const string label, vector<Point>& contour);
//label character 
int Labeling(Mat& dst, int flag);
//label on contours
void Label_map(Mat& out, vector<Point>& contour,Mat& im, int label);
//output characters
void Key_express(Point pt, Mat& out, Mat& dst, Mat& frame);

#define MAX_STEP 150
#define BLACK_THR 85

//main function
int main(int argc, char* argv[])
{	
	if (argc != 2){
		cout<<"Missing input file."<<endl;
		exit(-1);
	}
	//input video sequence file
	string filename = argv[1];
	//Mat to store images, frame for current frame, skinArea for extracted hand
	Mat frame, skinArea;
	//video capture for video sequence
	VideoCapture capture(filename);
	//loop through video to analyse password
	passwordAnalysis(frame, skinArea, capture);
	
	return 0;
}
vector<int> pressTrace;
Mat out;
Mat dst;
//password analysis
void passwordAnalysis(Mat frame, Mat skinArea, VideoCapture capture){
	//store finger trace locally
	vector<vector<Point> > fingerTrace(5);
	
	//frame counter
	int frame_num = 0;
	while (1)
	{
		//assign the current frame
		capture >> frame;
		if (frame.empty())
			break;
		if (frame_num ==0){
			Mat gray;
			cvtColor(frame, gray, CV_BGR2GRAY);
			black_areadetection(gray, BLACK_THR);
			
			int size[] = {gray.rows-1, gray.cols-1};
			out.create(2, size, CV_8U);
			out = Scalar(0);
			Mat bw;
			Canny(gray, bw, 0, 50, 5);
			vector<vector<Point> > contours;
			vector<vector<Point> > tcontours;
			findContours(bw.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

			vector<Point> approx;
			dst = frame.clone();
			double max_area = 0;
			int max_idx = 0;

			int count = 0; 
			for (int i = 0; i < contours.size(); i++)
			{
				// Approximate contour with accuracy proportional
				// to the contour perimeter
				approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

				// Skip small or non-convex objects 
				if (fabs(contourArea(contours[i])) < 100 || !isContourConvex(approx))
					continue;
				else if (approx.size() == 4)
				{
					// Number of vertices of polygonal curve
					int vtc = approx.size();

					// Get the cosines of all corners
					vector<double> cos;
					for (int j = 2; j < vtc+1; j++)
						cos.push_back(angle(approx[j%vtc], approx[j-2], approx[j-1]));

					// Sort ascending the cosine values
					sort(cos.begin(), cos.end());

					// Get the lowest and the highest cosine
					double mincos = cos.front();
					double maxcos = cos.back();

					// Use the degrees obtained above and the number of vertices
					// to determine the shape of the contour
					if (vtc == 4 && mincos >= -0.1 && maxcos <= 0.3) {			
						double area = contourArea(contours[i]);
						max_idx  = area > max_area ? i : max_idx;
						max_area = area > max_area ? area : max_area;
						//for rectangles only
						tcontours.push_back(contours[i]);
						count ++;
						Label_map(out, contours[i], gray, count);
						// drawContours(dst,tcontours,-1,Scalar(0,0,255),2);
					}
				}		
			}
			imshow("gray", gray);
		}		

		frame_num ++;
		//create Mat of the same size in single channel
		skinArea.create(frame.rows, frame.cols, CV_8UC1);
		//get extracted hand
		skinExtract(frame, skinArea);
		//imshow("skin", skinArea);

		//make a copy of the skin area image
		Mat show_img;
		frame.copyTo(show_img, skinArea);

		//list of contours
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		//obtain a list of contours
		findContours(skinArea, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

		//if no contours found, skip this frame
		if (contours.size() == 0) {
			continue;
		}		
		
		int index;
		double area;
		for (int i=0; i < contours.size(); i++)
		{
			area = contourArea(Mat(contours[i]));
			//find big enough contours 
			if (area > 10000)
			{				
				index = i;

				//draw hand contour in red
				vector<vector<Point> > tcontours;
				tcontours.push_back(contours[index]);
				drawContours(frame,tcontours,-1,cv::Scalar(0,0,255),2);
				drawContours(show_img,tcontours,-1,cv::Scalar(0,0,255),2);				

				//calculate center of hand and draw circle
				Moments moment = moments(skinArea, true);
				Point center(moment.m10/moment.m00, moment.m01/moment.m00);
				if(contourArea(contours[index])>=10000){			
					center.y += frame.rows/5;
					circle(show_img, center, 8 ,Scalar(0, 0, 255), CV_FILLED);
				}

				// find fingertips
				vector<Point> fingerTips = findFingers(contours, index, center);

				//store fingers locally
				if (fingerTips.size()== 5) {
					//sort finger tips 
					sort(fingerTips.begin(), fingerTips.end(), toLeft);
					//draw fingers
					for (int i = 0; i < 5; i ++) {
						fingerTrace[i].push_back(fingerTips[i]);
						if (fingerTrace[i].size() > 15) {
							press_listener(fingerTrace, frame);
						}
						stringstream ss;
						ss<<i;
						putText(show_img, ss.str(), fingerTips[i], CV_FONT_HERSHEY_COMPLEX,0.7,Scalar(0,255,0));
						circle(show_img, fingerTips[i], 6 ,Scalar(0, 255, 0), CV_FILLED);
						circle(frame, fingerTips[i], 6 ,Scalar(0, 255, 0), CV_FILLED);
						line(show_img, center, fingerTips[i], Scalar(255, 0, 0), 2);
					}	
					// cout<<fingerTrace.size()<<endl;
					// cout<<"Point 1: "<<fingerTrace[1][fingerTrace[1].size()-1].x
					// <<" "<<fingerTrace[1][fingerTrace[1].size()-1].y<<endl;				
				}
			}			
		}		

		imshow("keyboard", show_img);
		imshow("frame", frame);

		if ( cvWaitKey(20) == 'q' )
			break;
	}
}

//point order
bool toLeft(Point i, Point j){
	return (i.x < j.x);
}

//find finger tips
vector<Point> findFingers(vector<vector<Point> > contours, int index, Point center){
	//obtain the contour of hand
	vector<Point> couPoint = contours[index];
	//create vector to store finger tips
	vector<Point> fingerTips;
	//tmp point
	Point tmp;

	int max(0), count(0), notice(0);
	vector<int> notices;

	//loop through hand contour points 
	for (int i = 0; i < couPoint.size(); i++)
	{
		tmp = couPoint[i];
		//calculate distance of current point to center
		int dist = (tmp.x - center.x) * (tmp.x - center.x) 
		+ (tmp.y - center.y) * (tmp.y - center.y);
		//switch the focus to the current maxmum point
		if (dist > max)	{
			max = dist;
			notice = i;
		}

		//find local maxima by observing how far a maximum point keeps
		if (dist != max)
		{		
			//skip if tmp point is below hand center
			if (center.y < tmp.y )					
				continue;	
			//add to counter for each forward point that is not greater than
			//current maximum 							
			count++;

			//if a maximum point keeps its throne for a threshold,
			//then it is the local maxima
			if (count > MAX_STEP)
			{
				//flag for skip
				bool flag = false;
				//reset counter and max
				count = 0;
				max = 0;

				//maximum that is below center is not considered
				if (center.y < couPoint[notice].y )
					continue;				
				
				//maximum point too closed to the previous is not considered
				for (int j = 0; j < fingerTips.size(); j++){
					if (abs(couPoint[notice].x - fingerTips[j].x) < 20){
						flag = true;
						break;
					}
				}
				if (flag) continue;

				//if satisfied, store the index
				notices.push_back(notice);
				//store finger tip if the local maxima is far away from the last one
				if ((notices.size() >=2 
					&&(notices[notices.size()-1] - notices[notices.size() - 2]) > 150)
					||notices.size()==1){
					fingerTips.push_back(couPoint[notice]);		
			}		
		}
	}
}
return fingerTips;
}

//calculate distance of two points
double calcDist(Point p1, Point p2) {
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

//using good threshold for skin-detection 
Mat GetSkin(Mat const &src) {
	
    // allocate the result matrix
	Mat dst = src.clone();
	//improvecontrast(dst);
	Vec3b cwhite = Vec3b::all(255);
	Vec3b cblack = Vec3b::all(0);

	//Hsv color Space
	Mat src_ycrcb, src_hsv;
	cvtColor(src, src_ycrcb, CV_BGR2YCrCb);

	src.convertTo(src_hsv, CV_32FC3);
	cvtColor(src_hsv, src_hsv, CV_BGR2HSV);

    // Now scale the values between [0,255]:
	normalize(src_hsv, src_hsv, 0.0, 255.0, NORM_MINMAX, CV_32FC3);

	for(int i = 0; i < src.rows; i++) {
		for(int j = 0; j < src.cols; j++) {

			Vec3b pix_bgr = src.ptr<Vec3b>(i)[j];
			int B = pix_bgr.val[0];
			int G = pix_bgr.val[1];
			int R = pix_bgr.val[2];
            // apply rgb rule
			bool a = R1(R,G,B);

			Vec3b pix_ycrcb = src_ycrcb.ptr<Vec3b>(i)[j];
			int Y = pix_ycrcb.val[0];
			int Cr = pix_ycrcb.val[1];
			int Cb = pix_ycrcb.val[2];
            // apply ycrcb rule
			bool b = R2(Y,Cr,Cb);

			Vec3f pix_hsv = src_hsv.ptr<Vec3f>(i)[j];
			float H = pix_hsv.val[0];
			float S = pix_hsv.val[1];
			float V = pix_hsv.val[2];
            // apply hsv rule
			bool c = R3(H,S,V);

			if(!(a&&b&&c))
				dst.ptr<Vec3b>(i)[j] = cblack;
			else
				dst.ptr<Vec3b>(i)[j] = cwhite;
		}
	}

	cvtColor(dst, dst, CV_BGR2GRAY);
	threshold(dst, dst, 60, 255, CV_THRESH_BINARY);	
	morphologyEx(dst, dst, CV_MOP_ERODE, Mat1b(3,3,1), Point(-1, -1), 1);
	morphologyEx(dst, dst, CV_MOP_OPEN, Mat1b(7,7,1), Point(-1, -1), 1);
	morphologyEx(dst, dst, CV_MOP_CLOSE, Mat1b(9,9,1), Point(-1, -1), 1);
	medianBlur(dst, dst, 5);
	
	return dst;
}

bool R1(int R, int G, int B) {
	bool e1 = (R>130) && (G>60) && (B>40) && ((max(R,max(G,B)) - min(R, min(G,B)))>15) && (abs(R-G)>15) && (R>G) && (R>B);
	bool e2 = (R>240) && (G>220) && (B>170) && (abs(R-G)<=15) && (R>B) && (G>B);
	return (e1||e2);
}

bool R2(float Y, float Cr, float Cb) {
	int avg_cb = 120;
	int avg_cr = 155;
	int SkinRange = 22;
	bool e3 = Cr <= avg_cr+SkinRange;
	bool e4 = Cr >= avg_cr-SkinRange;
	bool e5 = Cb >= avg_cb-SkinRange;
	bool e6 = Cb <= avg_cb+SkinRange;
	return e3 && e4 && e5 && e6;
}

bool R3(float H, float S, float V) {
	return (H < 35 ) || (H > 180);
}

//obtain skin area image
void skinExtract(const Mat &frame, Mat &skinArea)
{
	int avg_cb = 120;//average of Cb
	int avg_cr = 155;//average of Cr
	int SkinRange = 18;//color range of skin

	Mat YCbCr;
	vector<Mat> planes;

	//color space conversion
	cvtColor(frame, YCbCr, CV_RGB2YCrCb);
	//split image into single channels
	split(YCbCr, planes); 

	//use Mat Iterator to get Cb, Cr
	MatIterator_<uchar> it_Cb = planes[1].begin<uchar>(),
	it_Cb_end = planes[1].end<uchar>();
	MatIterator_<uchar> it_Cr = planes[2].begin<uchar>();
	MatIterator_<uchar> it_skin = skinArea.begin<uchar>();

	//set pixel color according to whether it is the color of skin
	for( ; it_Cb != it_Cb_end; ++it_Cr, ++it_Cb, ++it_skin)
	{
		if (avg_cr-SkinRange <= *it_Cr &&  *it_Cr <= avg_cr+SkinRange && avg_cb-SkinRange <= *it_Cb &&  *it_Cb <= avg_cb+SkinRange)
			*it_skin = 255;
		else
			*it_skin = 0;
	}

	//noise cancellation by dilation, erosion and blur
	erode(skinArea, skinArea, Mat(5, 5, CV_8UC1), Point(-1, -1));
	medianBlur(skinArea, skinArea, 5);
	dilate(skinArea, skinArea, Mat(5, 5, CV_8UC1), Point(-1, -1));	
	medianBlur(skinArea, skinArea, 5);
}

//whether a finger presses a key
void press_listener(vector<vector<Point> > fingerTrace, Mat& frame)
{
	int frame_range = 15;
	// Have possibilities that already pressed
	for (int i = 0; i < fingerTrace.size(); i ++) {
		if ((fingerTrace[i][fingerTrace[i].size() - 1].y 
			- fingerTrace[i][fingerTrace[i].size() - 7].y > 0)) {
			if (fingerTrace[i][fingerTrace[i].size() - 2].y - fingerTrace[i][fingerTrace[i].size() - 2 -frame_range].y > 20) {
				pressTrace.push_back(i);
				int traceSize = pressTrace.size();
				if (traceSize >=5){
					bool pressed = true;
					for (int j = traceSize - 1; j >= traceSize - 4; j --){
						if (pressTrace[j] != pressTrace[j-1])
							pressed = false;
					}
					if (pressed == true){
						Point pressedKey = fingerTrace[i][fingerTrace[i].size() - 2];
						Key_express(pressedKey, out, dst, frame);					
					}
				}
			}
		}
	}
}

static double angle(Point pt1, Point pt2, Point pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}


void improvecontrast(Mat& im)
{
	// for color image

	double alpha = 5; // contrast control
	int beta = -5; // lightness control

	Mat new_image = Mat::zeros( im.size(), im.type());

	//do the operation new(i, j) = alpha * image(i,j) + beta
	for (int y = 0; y < im.rows; y ++)
	{
		for (int x = 0; x < im.cols; x ++)
		{
			for (int c = 0; c< 3; c++)
			{
				new_image.at<Vec3b>( y, x)[c] = saturate_cast<uchar>( alpha * (im.at<Vec3b>(y, x)[c]) + beta);
			}
		}
	}
	im = new_image;
}


void black_areadetection(Mat& im, int threshold)
{
	Mat new_image = im;
	for (int y = 0; y < im.rows; y ++)
	{
		for (int x = 0; x < im.cols; x ++)
		{
			int value = (int)new_image.at<uchar> ( y, x);
			if (value < threshold)
			{
				value = 0; 
			}	
			else
			{
				value = 255;
			}
			new_image.at<uchar> ( y, x) = (uchar) value;
		}
	}
	im = new_image;

}

 //Helper function to display text in the center of a contour 
 void setLabel(Mat& im, const string label, vector<Point>& contour)
 {
 	int fontface = FONT_HERSHEY_SIMPLEX;
 	double scale = 0.8;
 	int thickness = 2;
 	int baseline = 0;

 	Size text = getTextSize(label, fontface, scale, thickness, &baseline);
 	Rect r = boundingRect(contour);

 	Point px(r.x, r.y);
 	Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));


 	rectangle(im, pt + Point(0, baseline), pt + Point(text.width, -text.height), CV_RGB(255,255,255), CV_FILLED);
 	putText(im, label, pt, fontface, scale, CV_RGB(0,0,0), thickness, 8);
 }



 int Labeling(Mat& dst, int flag)
 {
 	tesseract::TessBaseAPI tess;
 	tess.Init(NULL, "eng", tesseract::OEM_DEFAULT);
 	if (flag == 1) 
 	{
 		tess.SetVariable("tessedit_char_whitelist", "0123456789");
 	}
 	else
 	{
 		tess.SetVariable("tessedit_char_whitelist", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
 	}
 	tess.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);

 	Mat clr;
 	cvtColor(dst, clr, CV_GRAY2RGB);

 	Mat gray;
 	cvtColor(clr, gray, CV_BGR2GRAY);

 	// imshow("lalala", dst);
 	tess.SetImage((uchar*)gray.data, gray.cols, gray.rows, 1, gray.cols);

 	char* out = tess.GetUTF8Text();

 	char character = out[0];
 	int label = (int) character;

	//cout<<character<<"*"<<label<<endl;
 	return label;	
 }

 void Label_map(Mat& out, vector<Point>& contour,Mat& im, int label)
 {

 	Rect r = boundingRect(contour);

	int flag = 0; // character only
	int space_key_test = 0;
	Mat dst = im(Rect(r)).clone();

	int chara_label = 0;

	double scale = 0.8;
	int table = 3;
	int max_x = 0.5 * dst.rows - scale * 0.5 * dst.rows;
	int min_x =  0.5 * dst.rows + scale * 0.5 * dst.rows;
	int max_y = 0.5 * dst.cols - scale * 0.5 * dst.cols;
	int min_y =  0.5 * dst.cols + scale * 0.5 * dst.cols;
	
	for (int i = 0.5 * dst.rows - scale * 0.5 * dst.rows; i < 0.5 * dst.rows + scale * 0.5 * dst.rows; i ++)
	{
		for (int j = 0.5 * dst.cols - scale * 0.5 * dst.cols; j < 0.5 * dst.cols + scale * 0.5 * dst.cols; j ++)
		{
			if (dst.at<uchar>(i, j) == (uchar)255)
			{
				max_x  = i > max_x ? i : max_x;
				max_y = j > max_y ? j : max_y;
				min_x  = i < min_x ? i : min_x;
				min_y  = j < min_y ? j : min_y;
				space_key_test++;
			}
		}	
	}	

	if (space_key_test != 0)
	{
		
		if ((max_x - min_x) > 0.4 * dst.rows)
		{
			min_x = 0.6 * dst.rows;	
				flag = 1; // number only		
				
			}
			Mat character = dst(Range(min_x-table, max_x+ table), Range(min_y - table, max_y + table));
			chara_label = Labeling(character, flag);
		}

		// nothing has been detected
		if(chara_label == 32) {chara_label = 0;} 



		// char to string in order to set label for the keys
		char chara = (char)chara_label;
		stringstream trans_s;
		string s;
		char c = chara;
		trans_s << c;
		trans_s >> s;
		setLabel(im, s, contour);

		for (int i = r.x+ 1; i < r.x + r.width; i ++){
			for (int j = r.y + 1; j < r.y + r.height; j ++){
				out.at<uchar>(j, i) = (uchar) chara_label;
			}	
		}		
	}

	void Key_express(Point pt, Mat& out, Mat& dst, Mat& frame){
		int fontface = FONT_HERSHEY_SIMPLEX;
		double scale = 4;
		int thickness = 2;

		char key_num = (char)out.at<uchar>(pt.y + 8, pt.x);
		if(key_num == NULL){			
			key_num = (char)out.at<uchar>(pt.y+8, pt.x+4);
			if(key_num == NULL){
				key_num = (char)out.at<uchar>(pt.y+8, pt.x-4);
			}			
		}
		stringstream ss;
		ss << key_num;
		putText(frame, ss.str(), pt, fontface, scale, CV_RGB(0,0,255), thickness, 8);
	}