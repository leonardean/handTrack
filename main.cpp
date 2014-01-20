#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
using namespace cv;
//This function returns the square of the euclidean distance between 2 points.
double dist(Point x,Point y)
{
	return (x.x-y.x)*(x.x-y.x)+(x.y-y.y)*(x.y-y.y);
}
//This function returns the radius and the center of the circle given 3 points
//If a circle cannot be formed , it returns a zero radius circle centered at (0,0)
pair<Point,double> circleFromPoints(Point p1, Point p2, Point p3)
{
	double offset = pow((double)(p2.x),2) + pow((double)(p2.y),2);
	double bc =   ( pow((double)(p1.x),2) + pow((double)(p1.y),2) - offset )/2.0;
	double cd =   (offset - pow((double)(p3.x), 2) - pow((double)(p3.y), 2))/2.0;
	double det =  (p1.x - p2.x) * (p2.y - p3.y) - (p2.x - p3.x)* (p1.y - p2.y); 
	double TOL = 0.0000001;
	if (abs(det) < TOL) { cout<<"POINTS TOO CLOSE"<<endl;return make_pair(Point(0,0),0); }

	double idet = 1/det;
	double centerx =  (bc * (p2.y - p3.y) - cd * (p1.y - p2.y)) * idet;
	double centery =  (cd * (p1.x - p2.x) - bc * (p2.x - p3.x)) * idet;
	double radius = sqrt( pow((double)(p2.x) - centerx,2) + pow((double)(p2.y)-centery,2));

	return make_pair(Point(centerx,centery),radius);
}

//The main function :D
int main(int argc, char *argv[])
{
	string filename = "/Users/leonardo/handTrack/hand1.mp4";
//	string filename = "/Users/new-worker/Desktop/hand2.mov";
	Mat frame;
	Mat back;
	Mat fore;
	Mat prevFrame;
	vector<pair<Point,double> > palm_centers;
	VideoCapture cap(filename);
	BackgroundSubtractorMOG2 bg;
	bg.set("nmixtures",3);
	bg.set("detectShadows",false);


	namedWindow("Frame");
	namedWindow("Background");
	int backgroundFrame=400;


	for(;;)
	{
		vector<vector<Point> > contours;
		//Get the frame
		cap >> frame;

		//Update the current background model and get the foreground
		if(backgroundFrame>0)
		{bg.operator ()(frame,fore);backgroundFrame--;}
		else
		{bg.operator()(frame,fore,0);}

		//Get background image to display it
		bg.getBackgroundImage(back);


		//Enhance edges in the foreground by applying erosion and dilation
		erode(fore,fore,Mat(), Point(-1, -1), 2);
		dilate(fore,fore,Mat(), Point(-1, -1));
		medianBlur(fore, fore, 5);


		//Find the contours in the foreground
		findContours(fore,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_NONE);
		int contournum = 0;
		for(int i=0;i<contours.size();i++)
			//Ignore all small insignificant areas
			if(contourArea(contours[i])>=10000)		    
			{
				//find the center of palm my computing moments
				Moments moment = moments(fore, true);
				Point center(moment.m10/moment.m00, moment.m01/moment.m00);
				circle(frame,center,8,Scalar(0,0,255),CV_FILLED);

				double dist_avg = 0;
				vector<Point> conPoint = contours[i];
				for(int k=0; k<conPoint.size();k++) {
					dist_avg += sqrt(dist(conPoint[k], center));
					
				}
				dist_avg /= conPoint.size();
				dist_avg /=1.5;
				cout<<dist_avg<<endl;


				//find the finger tips circle
				/*
				vector<Point> couPoint = contours[i];
				vector<Point> fingerTips;
				Point p,q,r;
				int max(0), count(0), notice(0);

				for(int k = 5; (k < (couPoint.size()-5)) && couPoint.size(); k ++)
				{
					q = couPoint[i - 5];  
            		p = couPoint[i];  
            		r = couPoint[i + 5];  
            		int dot = (q.x - p.x ) * (r.x - p.x) + (q.y - p.y ) * (r.y - p.y);  
            		if (dot < 20 && dot > -20)  
           			{  
                		int cross = (q.x - p.x ) * (r.y - p.y) - (r.x - p.x ) * (q.y - p.y);  
                		if (cross > 0)  
                		{  	
                			cout<<"finger tip pushed"<<endl;
                    		fingerTips.push_back(p);  
                    		circle(frame, p, 5 ,Scalar(255, 0, 0), CV_FILLED);  
                    		line(frame, center, p, Scalar(255, 0, 0), 2);      
                		}  
            		} 
				}*/



				//find the finger tips curvature
				/*
				vector<Point> couPoint = contours[i];
				vector<Point> fingerTips;
				Point tmp;
				int max(0), count(0), notice(0);
				for (int k = 0; k < couPoint.size(); k ++) {
					tmp = couPoint[k];
					int dist = (tmp.x - center.x) * (tmp.x - center.x) + (tmp.y - center.y) * (tmp.y - center.y);
					if (dist > max) {
						max = dist;
						notice = k;
					}

					if (dist != max) {
						count ++;
						if (count > 40) {
							count = 0;
							max = 0;
							bool flag = false;
							//points lower than the center of hand does not count
							// if (center.y < couPoint[notice].y)
							if (center.x < couPoint[notice].x)
								continue;

							//points too closed to current stored fingerTip does not count
							for (int j=0; j < fingerTips.size(); j ++) {
								if (abs(couPoint[notice].x - fingerTips[j].x) < 20) {
									flag = true;
									break;
								}
							}

							if (flag)
								continue;

							cout<<"finger tip pushed"<<endl;
							fingerTips.push_back(couPoint[notice]);
							circle(frame, couPoint[notice], 6, Scalar(0, 255, 0), CV_FILLED);
							line(frame, center, couPoint[notice], Scalar(255, 0, 0), 2);
						}
					}
				}*/





				contournum ++;
				//Draw contour
				vector<vector<Point> > tcontours;
				tcontours.push_back(contours[i]);
                //Scalar(B,G,R)
				drawContours(frame,tcontours,-1,cv::Scalar(0,0,255),2);

				//Detect Hull in current contour
				vector<vector<Point> > hulls(1);
				vector<vector<int> > hullsI(1);
				convexHull(Mat(tcontours[0]),hulls[0],false);
				convexHull(Mat(tcontours[0]),hullsI[0],false);
				drawContours(frame,hulls,-1,cv::Scalar(0,255,0),2);

				//Find minimum area rectangle to enclose hand
				RotatedRect rect=minAreaRect(Mat(tcontours[0]));

				//Find Convex Defects
				vector<Vec4i> defects;
                
				if(hullsI[0].size()>0)
				{
                    //returns the 4 points of rect into rect_points
					Point2f rect_points[4]; rect.points( rect_points );
                    //draw outter rectangle
					// for( int j = 0; j < 4; j++ )
					// 	line( frame, rect_points[j], rect_points[(j+1)%4], Scalar(255,0,0), 2, 8 );
                    
					Point rough_palm_center;
                    //convexityDefects(inputcoutour, inputhullindex, outputdefects)
					convexityDefects(tcontours[0], hullsI[0], defects);
                    
					if(defects.size()>=3)
					{
                        double max_R = 0;
						vector<Point> palm_points;
						for(int j=0;j<defects.size();j++)
						{
							int startidx=defects[j][0];
                            Point ptStart( tcontours[0][startidx] );
//                            line( frame, ptStart, ptStart, Scalar(0,0,255), 2, 8 );
							int endidx=defects[j][1];
                            Point ptEnd( tcontours[0][endidx] );
//                            line( frame, ptEnd, ptEnd, Scalar(0,0,255), 4, 8 );

							int faridx=defects[j][2];
                            Point ptFar( tcontours[0][faridx] );
//                            line( frame, ptStart, ptFar, Scalar(0,0,255), 2, 8 );
                            
                            if (dist(ptStart,ptFar) > max_R) {
                                max_R += sqrt(dist(ptStart,ptFar));
                            }
                            
                            
							//Sum up all the hull and defect points to compute average
							rough_palm_center+=ptFar+ptStart+ptEnd;
							palm_points.push_back(ptFar);
							palm_points.push_back(ptStart);
							palm_points.push_back(ptEnd);
						}
                        max_R /= defects.size();
                        
                        cout<<"max_R: "<<max_R<<endl;

						//Get palm center by 1st getting the average of all defect points, this is the rough palm center,
						//Then U chose the closest 3 points ang get the circle radius and center formed from them which is the palm center.
						rough_palm_center.x/=defects.size()*3;
						rough_palm_center.y/=defects.size()*3;
                        
                        //which to choose
						Point closest_pt=palm_points[0];
                        
						vector<pair<double,int> > distvec;
						for(int i=0;i<palm_points.size();i++)
							distvec.push_back(make_pair(dist(rough_palm_center,palm_points[i]),i));
						sort(distvec.begin(),distvec.end());

						//Keep choosing 3 points till you find a circle with a valid radius
						//As there is a high chance that the closes points might be in a linear line or too close that it forms a very large circle
						pair<Point,double> soln_circle;
						for(int i=0;i+2<distvec.size();i++)
						{
							Point p1=palm_points[distvec[i+0].second];
							Point p2=palm_points[distvec[i+1].second];
							Point p3=palm_points[distvec[i+2].second];
							soln_circle=circleFromPoints(p1,p2,p3);//Final palm center,radius
							if(soln_circle.second!=0)
								break;
						}

						//Find avg palm centers for the last few frames to stabilize its centers, also find the avg radius
						palm_centers.push_back(soln_circle);
						if(palm_centers.size()>10)
							palm_centers.erase(palm_centers.begin());
						
						Point palm_center;
						double radius=0;
						for(int i=0;i<palm_centers.size();i++)
						{
							palm_center+=palm_centers[i].first;
							radius+=palm_centers[i].second;
						}
						palm_center.x/=palm_centers.size();
						palm_center.y/=palm_centers.size();
						radius/=palm_centers.size();

						//Draw the palm center and the palm circle
						//The size of the palm gives the depth of the hand
						circle(frame,center,5,Scalar(144,144,255),3);
						circle(frame,center,dist_avg,Scalar(144,144,255),2);

                        /**
                         */
						//Detect fingers by finding points that form an almost isosceles triangle with certain thesholds
						int no_of_fingers=0;
						for(int j=0;j<defects.size();j++)
						{
							int startidx=defects[j][0]; Point ptStart( tcontours[0][startidx] );
							int endidx=defects[j][1]; Point ptEnd( tcontours[0][endidx] );
							int faridx=defects[j][2]; Point ptFar( tcontours[0][faridx] );
							//X o--------------------------o Y
							double Xdist=sqrt(dist(center,ptFar));
							double Ydist=sqrt(dist(center,ptStart));
							double length=sqrt(dist(ptFar,ptStart));

							double retLength=sqrt(dist(ptEnd,ptFar));
							//Play with these thresholds to improve performance
							if(length<=3*dist_avg&&
                              Ydist>=0.4*dist_avg&&
                               length>=10&&
                               retLength>=10&&
                               max(length,retLength)/min(length,retLength)>=0.8)
								if(min(Xdist,Ydist)/max(Xdist,Ydist)<=0.8)
								{
									if((Xdist>=0.1*dist_avg&&Xdist<=1.3*dist_avg&&Xdist<Ydist)||(Ydist>=0.1*dist_avg&&Ydist<=1.3*radius&&Xdist>Ydist)){
										line( frame, ptEnd, ptFar, Scalar(0,255,0), 2 ),no_of_fingers++;
//                                        circle(frame,ptEnd,3,Scalar(0,0,255),2);
                                    }
								}


						}
						
						no_of_fingers=min(5,no_of_fingers);
						cout<<"NO OF FINGERS: "<<no_of_fingers<<endl;						
					}
				}

			}	

		if(backgroundFrame>0)
			putText(frame, "Recording Background", cvPoint(30,30), FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(200,200,250), 1, CV_AA);
		imshow("Frame",frame);
		imshow("Background",back);
		if(waitKey(10) >= 0) break;
		cout<<"number of contour: "<<contournum<<endl;

	}
	return 0;
}
