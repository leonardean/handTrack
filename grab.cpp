// Simple FingerTips Detection
// Author : Zouxy
// Date   : 2013-3-23
// HomePage : http://blog.csdn.net/zouxy09
// Email  : zouxy09@qq.com

#include "opencv2/opencv.hpp"

using namespace cv;
using namespace std;

void skinExtract(const Mat &frame, Mat &skinArea);



int main(int argc, char* argv[])
{	
	string filename = "/Users/new-worker/OpenCVHandGuesture/hand.mov";
	Mat frame, skinArea;
	VideoCapture capture(filename);


	// capture.open(0);
	// if (!capture.isOpened())
	// {
	// 	cout<<"No camera!\n"<<endl;
	// 	return -1;
	// }

	while (1)
	{
		// cout<<"entered"<<endl;
		capture >> frame;
		//Mat frame = imread("fingertips(1).jpg");
		if (frame.empty())
			break;

		skinArea.create(frame.rows, frame.cols, CV_8UC1);
		skinExtract(frame, skinArea);
		Mat show_img;
		frame.copyTo(show_img, skinArea);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		//寻找轮廓
		findContours(skinArea, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

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



		//drawContours(frame, contours, index, Scalar(0, 0, 255), 2, 8, hierarchy );
		
		Moments moment = moments(skinArea, true);
		Point center(moment.m10/moment.m00, moment.m01/moment.m00);
		circle(show_img, center, 8 ,Scalar(0, 0, 255), CV_FILLED);

		// cout<<"contours size: "<<contours.size()<<endl;
		// cout<<"index: "<<index<<endl;
		if (contours.size() == 0) {
			continue;
		}
		// 寻找指尖
		vector<Point> couPoint = contours[index];

		vector<Point> fingerTips;
		Point tmp;

		int max(0), count(0), notice(0);
		for (int i = 0; i < couPoint.size(); i++)
		{

			tmp = couPoint[i];
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
				if (count > 40)
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
						if (abs(couPoint[notice].x - fingerTips[j].x) < 20)
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



		imshow("show_img", show_img);

		if ( cvWaitKey(20) == 'q' )
			break;

	}

	return 0;
}

//肤色提取，skinArea为二值化肤色图像
void skinExtract(const Mat &frame, Mat &skinArea)
{
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
		if (138 <= *it_Cr &&  *it_Cr <= 170 && 100 <= *it_Cb &&  *it_Cb <= 127)
			*it_skin = 255;
		else
			*it_skin = 0;
	}

	//膨胀和腐蚀，膨胀可以填补凹洞（将裂缝桥接），腐蚀可以消除细的凸起（“斑点”噪声）
	dilate(skinArea, skinArea, Mat(5, 5, CV_8UC1), Point(-1, -1));
	erode(skinArea, skinArea, Mat(5, 5, CV_8UC1), Point(-1, -1));
}
