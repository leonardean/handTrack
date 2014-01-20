// Simple FingerTips Detection
// Author : Zouxy
// Date   : 2013-3-23
// HomePage : http://blog.csdn.net/zouxy09
// Email  : zouxy09@qq.com

#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

void skinExtract(const Mat &frame, Mat &skinArea);
Mat GetSkin(Mat const &src);
bool R1(int R, int G, int B);
bool R2(float Y, float Cr, float Cb);
bool R3(float H, float S, float V);

int main(int argc, char* argv[])
{	
	string filename = "/Users/leonardo/handTrack/hand3.mp4";
	Mat frame, skinArea;
	VideoCapture capture(filename);

	while (1)
	{
		// cout<<"entered"<<endl;
		capture >> frame;
		//Mat frame = imread("fingertips(1).jpg");
		if (frame.empty())
			break;

		skinArea.create(frame.rows, frame.cols, CV_8UC1);
		skinExtract(frame, skinArea);
		// imshow("skin", skinArea);
		// skinArea = GetSkin(frame);
		Mat show_img;
		frame.copyTo(show_img, skinArea);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		//寻找轮廓
		findContours(skinArea, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

		if (contours.size() == 0) {
			continue;
		}
		// 找到最大的轮廓
		int index;
		double area, maxArea(0);
		for (int i=0; i < contours.size(); i++)
		{
			area = contourArea(Mat(contours[i]));
			if (area > maxArea)
			{
				maxArea = area;
				index = i;
			}			
		}

		vector<vector<Point> > tcontours;
		if(contourArea(contours[index])>=10000){
			tcontours.push_back(contours[index]);
			drawContours(show_img,tcontours,-1,cv::Scalar(0,0,255),2);
		}


		// drawContours(frame, contours, index, Scalar(0, 0, 255), 2, 8, hierarchy );
		// drawContours(show_img,contours,-1,cv::Scalar(0,0,255),2);
		Moments moment = moments(skinArea, true);
		Point center(moment.m10/moment.m00, moment.m01/moment.m00);
		if(contourArea(contours[index])>=10000){			
			center.y = frame.rows - 1;
			circle(show_img, center, 8 ,Scalar(0, 0, 255), CV_FILLED);
		}

		// cout<<"contours size: "<<contours.size()<<endl;
		// cout<<"index: "<<index<<endl;
		
		// 寻找指尖
		vector<Point> couPoint = contours[index];

		vector<Point> fingerTips;
		Point tmp;

		int max(0), count(0), notice(0);
		for (int i = 0; i < couPoint.size(); i++)
		{

			tmp = couPoint[i];
			//calculate distance of current point to center
			int dist = (tmp.x - center.x) * (tmp.x - center.x) + (tmp.y - center.y) * (tmp.y - center.y);
			if (dist > max)
			{
				max = dist;
				notice = i;
			}

			// 计算最大值保持的点数，如果大于40（这个值需要设置，本来想根据max值来设置，
			// 但是不成功，不知道为何），那么就认为这个是指尖
			if (dist != max)
			{
				count++;
				if (count > 80)
				{
					count = 0;
					max = 0;
					bool flag = false;
					// 低于手心的点不算
					if (center.y < couPoint[notice].y )
						continue;
					// 离得太近的不算
					for (int j = 0; j < fingerTips.size(); j++)
					{
						if (abs(couPoint[notice].x - fingerTips[j].x) < 40)
						{
							flag = true;
							break;
						}
					}
					if (flag) continue;
					fingerTips.push_back(couPoint[notice]);
					circle(show_img, couPoint[notice], 6 ,Scalar(0, 255, 0), CV_FILLED);
					line(show_img, center, couPoint[notice], Scalar(255, 0, 0), 2);				
				}
			}
		}

		//finger tracking 2
		// vector<Point> couPoint = contours[index];
		// 		vector<Point> fingerTips;
		// 		Point p,q,r;
		// 		int max(0), count(0), notice(0);

		// 		for(int k = 5; (k < (couPoint.size()-5)) && couPoint.size(); k ++)
		// 		{
		// 			q = couPoint[index - 5];  
  //           		p = couPoint[index];  
  //           		r = couPoint[index + 5];  
  //           		int dot = (q.x - p.x ) * (r.x - p.x) + (q.y - p.y ) * (r.y - p.y);  
  //           		if (dot < 20 && dot > -20)  
  //          			{  
  //               		int cross = (q.x - p.x ) * (r.y - p.y) - (r.x - p.x ) * (q.y - p.y);  
  //               		if (cross > 0)  
  //               		{  	
  //               			cout<<"finger tip pushed"<<endl;
  //                   		fingerTips.push_back(p);  
  //                   		circle(frame, p, 5 ,Scalar(255, 0, 0), CV_FILLED);  
  //                   		line(frame, center, p, Scalar(255, 0, 0), 2);      
  //               		}  
  //           		} 
		// 		}


		imshow("show_img", show_img);

		if ( cvWaitKey(20) == 'q' )
			break;

	}

	return 0;
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

	//region_value(dst, 10);
	
	
    return dst;
}

bool R1(int R, int G, int B) {
    bool e1 = (R>130) && (G>60) && (B>40) && ((max(R,max(G,B)) - min(R, min(G,B)))>15) && (abs(R-G)>15) && (R>G) && (R>B);
    bool e2 = (R>240) && (G>220) && (B>170) && (abs(R-G)<=15) && (R>B) && (G>B);
    return (e1||e2);
}

bool R2(float Y, float Cr, float Cb) {
	int avg_cb = 120;//YCbCr顏色空間膚色cb的平均值
	int avg_cr = 155;//YCbCr顏色空間膚色cr的平均值
	int SkinRange = 22;//YCbCr顏色空間膚色的範圍
    bool e3 = Cr <= avg_cr+SkinRange;
    bool e4 = Cr >= avg_cr-SkinRange;
    bool e5 = Cb >= avg_cb-SkinRange;
    bool e6 = Cb <= avg_cb+SkinRange;
    return e3 && e4 && e5 && e6;
}

bool R3(float H, float S, float V) {
    return (H < 35 ) || (H > 180);
}

//肤色提取，skinArea为二值化肤色图像
void skinExtract(const Mat &frame, Mat &skinArea)
{
	int avg_cb = 120;//YCbCr顏色空間膚色cb的平均值
	int avg_cr = 155;//YCbCr顏色空間膚色cr的平均值
	int SkinRange = 22;//YCbCr顏色空間膚色的範圍
	Mat YCbCr;
	vector<Mat> planes;

	//转换为YCrCb颜色空间
	cvtColor(frame, YCbCr, CV_RGB2YCrCb);
	//将多通道图像分离为多个单通道图像
	split(YCbCr, planes); 

	//运用迭代器访问矩阵元素
	MatIterator_<uchar> it_Cb = planes[1].begin<uchar>(),
						it_Cb_end = planes[1].end<uchar>();
	MatIterator_<uchar> it_Cr = planes[2].begin<uchar>();
	MatIterator_<uchar> it_skin = skinArea.begin<uchar>();

	//人的皮肤颜色在YCbCr色度空间的分布范围:100<=Cb<=127, 138<=Cr<=170
	for( ; it_Cb != it_Cb_end; ++it_Cr, ++it_Cb, ++it_skin)
	{
		if (avg_cr-SkinRange <= *it_Cr &&  *it_Cr <= avg_cr+SkinRange && avg_cb-SkinRange <= *it_Cb &&  *it_Cb <= avg_cb+SkinRange)
			*it_skin = 255;
		else
			*it_skin = 0;
	}

	//膨胀和腐蚀，膨胀可以填补凹洞（将裂缝桥接），腐蚀可以消除细的凸起（“斑点”噪声）
	dilate(skinArea, skinArea, Mat(5, 5, CV_8UC1), Point(-1, -1), 6);

	erode(skinArea, skinArea, Mat(5, 5, CV_8UC1), Point(-1, -1));
	medianBlur(skinArea, skinArea, 5);
}
