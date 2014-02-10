Password Steal
==============
##Intro
This password stealer outputs the keyboard typing in a video sequence. Key algorithms are:
* Hand tracking
	* Background Subtraction
	* Skin Color Segmentation
* Finger tracking
	* Local Maxima
	* Convex Hulls and Defects
	* K-curvature
* Keyboard and Key Recognition
* Key Value Identification using Tesseract
* Finger press classification
##Deliverables
* Keyboard and key recognition with key value identification:
![key](http://pan.baidu.com/s/1kTnyNpP)
* Hand and finger tip tracking
![hand](http://pan.baidu.com/s/1mgr080S)
* Pressed key value output
![value](http://pan.baidu.com/s/1sjO0s5B)
##Compilation and Running
To compile the code, the following libraries need to be pre-installed:
* OpenCV 2.4 or higher (http://opencv.org/)
* Tesseract-ORC (http://code.google.com/p/tesseract-ocr/)

There is a build executable file that can be used to compile the code by command:
$ ./build
in the directory of the code.

However, whether the build file works depends on the running system. It has been tested on Ubuntu Linux with OpenCV installed in pkg-config. Below is the command for compilation if the build file does not work:
* In Linux system with OpenCV installed in pkg-config:
$ g++ main.cpp -o a.out `pkg-config --libs opencv` -I /usr/include/leptonica/ -I /usr/include/tesseract/ -llept -ltesseract
* In Mac OS with OpenCV and Tesseract-ORC Installed via brew (http://brew.sh/), they will both be installed in pkg-config:
$ g++ main.cpp -o a.out `pkg-config --libs opencv tesseract`

To run the application:
$ ./a.out [path to video]
or using the provided video:
$ ./a.out password.mp4 