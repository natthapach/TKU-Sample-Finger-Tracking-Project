// Sample_OpenCV.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <iostream>
#include <opencv2/opencv.hpp>
//
using namespace cv;

int main(int argc, char const *argv[]) {
	Mat img = imread("1.jpg");
	namedWindow("image", WINDOW_NORMAL);
	imshow("image", img);
	waitKey(0);
	return 0;
}