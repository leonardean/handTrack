#include <stdafx.h>
#include <iostream>
#include "cv.h"
#include "highgui.h"

using namespace cv;
using namespace std;

int avg_cb = 120;//YCbCr顏色空間膚色cb的平均值
int avg_cr = 155;//YCbCr顏色空間膚色cr的平均值
int SkinRange = 22;//YCbCr顏色空間膚色的範圍

void RGBtoYCbCr(IplImage *);
void SkinColorDetection(IplImage *);
void main(int argc, char* argv[])
{
    IplImage* image = cvLoadImage("lena.jpg");
    IplImage* pImgCopy = cvCreateImage(cvGetSize(image), image->depth, image->nChannels);

    if(image) //判斷影像是否讀取成功
    {
        cvCopy(image, pImgCopy, NULL);
        RGBtoYCbCr(pImgCopy);//RGB轉YCbCr的轉換函式
        SkinColorDetection(pImgCopy);

        imshow("RGB", image);
        imshow("YCbCr", pImgCopy);
    }
    waitKey(0);
}
void RGBtoYCbCr(IplImage *image)
{
    CvScalar scalarImg;
    double cb, cr, y;
    for(int i=0; i<image->height; i++)
    for(int j=0; j<image->width; j++)
    {
        scalarImg = cvGet2D(image, i, j);//從影像中取RGB值
        y =  (16 + scalarImg.val[2]*0.257 + scalarImg.val[1]*0.504 
            + scalarImg.val[0]*0.098);
        cb = (128 - scalarImg.val[2]*0.148 - scalarImg.val[1]*0.291 
            + scalarImg.val[0]*0.439);
        cr = (128 + scalarImg.val[2]*0.439 - scalarImg.val[1]*0.368 
            - scalarImg.val[0]*0.071);
        //RGB顏色空間轉YCbCr顏色空間公式轉換
        cvSet2D(image, i, j, cvScalar( y, cr, cb));
    }
}

void SkinColorDetection(IplImage *image)
{
    CvScalar scalarImg;
    double cb, cr;
    for(int i=0; i<image->height; i++)
    for(int j=0; j<image->width; j++)
    {
        scalarImg = cvGet2D(image, i, j);
        cr = scalarImg.val[1];
        cb = scalarImg.val[2];
        if((cb > avg_cb-SkinRange && cb < avg_cb+SkinRange) &&
                  (cr > avg_cr-SkinRange && cr < avg_cr+SkinRange))
        cvSet2D(image, i, j, cvScalar( 255, 255, 255));
        else
            cvSet2D(image, i, j, cvScalar( 0, 0, 0));
    }
}