#pragma once
#include "Activator.h"
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <OpenNI.h>

class RGBActivator : virtual public Activator {
public:
	RGBActivator();
	void onInitial();
	void onPrepare();
	void onReadFrame();
	void onModifyFrame();
	void onDraw(cv::Mat canvas);
	void onPerformKeyboardEvent(int key);
	void onDie();
	cv::Mat getImageFrame();
	std::string getName();
protected:
	openni::Device device;
	openni::VideoStream sensor;
	uchar img[480][640][3];
};
