// Sample_ONI_NiTE_OGL_OCV_01.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <NiTE.h>
#include <OpenNI.h>

using namespace std;

const int WINDOW_W = 640;
const int WINDOW_H = 480;
const int RANGE = 50;
const string WINDOW_NAME = "image";

nite::HandTracker handTracker;
int handDepth = 0;
int numberOfHands = 0;
int depthHistogram[65536];
uchar imgFrame[WINDOW_H][WINDOW_W];
cv::Mat img;

int handleKeyboardEvent(int key) {
	switch (key)
	{
	case 27 :	// ESC
		cv::destroyAllWindows();
		return 1;	// break Event Loop
		break;
	default:
		break;
	}
	return 0;
}

int handleNiteStatus(nite::Status status) {
	if (status == nite::STATUS_OK) {
		return 0;
	}
	else {
		printf("ERROR #%d, %s", status, openni::OpenNI::getExtendedError());
	}
	return 1;
}

int initialize() {
	nite::Status status = nite::STATUS_OK;

	status = nite::NiTE::initialize();
	if (handleNiteStatus(status) != 0)
		return 1;

	status = handTracker.create();
	if (handleNiteStatus(status) != 0)
		return 1;

	status = handTracker.startGestureDetection(nite::GESTURE_HAND_RAISE);
	if (handleNiteStatus(status) != 0)
		return 1;

	return 0;
}

void startHandTracker(nite::HandTrackerFrameRef handsFrame) {
	const nite::Array<nite::GestureData>& gestures = handsFrame.getGestures();
	for (int i = 0; i < gestures.getSize(); ++i) {
		if (gestures[i].isComplete()) {
			nite::HandId handId;
			handTracker.startHandTracking(gestures[i].getCurrentPosition(), &handId);
		}
	}
}

void calDepthHistogram(openni::VideoFrameRef depthFrame, int* numberOfPoints, int* numberOfHandPoints) {
	*numberOfPoints = 0;
	*numberOfHandPoints = 0;

	memset(depthHistogram, 0, sizeof(depthHistogram));
	for (int y = 0; y < depthFrame.getHeight(); ++y)
	{
		openni::DepthPixel* depthCell = (openni::DepthPixel*)
			(
				(char*) depthFrame.getData() +
				(y * depthFrame.getStrideInBytes())
			);
		for (int x = 0; x < depthFrame.getWidth(); ++x, ++depthCell)
		{
			if (*depthCell != 0)
			{
				depthHistogram[*depthCell]++;
				(*numberOfPoints)++;

				if (handDepth > 0 && numberOfHands > 0) {
					if (handDepth - RANGE <= *depthCell && *depthCell <= handDepth + RANGE)
						(*numberOfHandPoints)++;
				}
			} 
		}
	}
	for (int nIndex = 1; nIndex < sizeof(depthHistogram) / sizeof(int); nIndex++)
	{
		depthHistogram[nIndex] += depthHistogram[nIndex - 1];
	}
}

void modifyImage(openni::VideoFrameRef depthFrame, int numberOfPoints, int numberOfHandPoint) {
	//for (unsigned int y = 0; y < WINDOW_H; y++) {
	//	for (unsigned int x = 0; x < WINDOW_W; x++) {
	//		float c = ((float)(x*y)) / (WINDOW_H * WINDOW_W);
	//		//printf("c %.2f\n", c);
	//		imgFrame[y][x] = (uchar) ((((float)(x*y)) / (WINDOW_H * WINDOW_W)) * 255);
	//	}
	//}
	for (unsigned int y = 0; y < WINDOW_H; y++) {
		for (unsigned int x = 0; x < WINDOW_W; x++) {
			openni::DepthPixel* depthPixel = (openni::DepthPixel*)
				(
				(char*)depthFrame.getData() +
					(
					((int)(y / 1) *
						depthFrame.getStrideInBytes())
						)
					) + (int)(x / 1);
			
			if (*depthPixel != 0) {
				int depthValue = (uchar) (((float)depthHistogram[*depthPixel] / numberOfPoints) * 255);
				imgFrame[y][x] = 255 - depthValue;
			}
			else {
				imgFrame[y][x] = 0;

			}
		}
	}

}

int mainLoop() {
	if (handTracker.isValid()) {
		nite::Status status = nite::STATUS_OK;
		nite::HandTrackerFrameRef handsFrame;
		status = handTracker.readFrame(&handsFrame);

		if (status == nite::STATUS_OK && handsFrame.isValid()) {
			startHandTracker(handsFrame);

			openni::VideoFrameRef depthFrame = handsFrame.getDepthFrame();

			// build depth image
			int numberOfPoints = 0;
			int numberOfHandPoints = 0;
			calDepthHistogram(depthFrame, &numberOfPoints, &numberOfHandPoints);
			modifyImage(depthFrame, numberOfPoints, numberOfHandPoints);
			img = cv::Mat(WINDOW_H, WINDOW_W, CV_8UC1, &imgFrame);


			cv::imshow(WINDOW_NAME, img);
		}
	}
	return 0;
}



int tearDown() {
	return 0;
}

int main(int argc, char const *argv[]) {
	uchar data[12] = { 0,0,255,0,255,0,0,0,255,0,0,255 };
	uchar data2[4] = { 0, 127, 255, 255 };
	uchar data3[4][3] = { 
							{127, 0, 100},
							{0, 255, 100},
							{100, 100, 0},
							{0, 0, 0}
						};

	cv::Mat img = cv::Mat(4, 3, CV_8UC1, data3);
	cv::namedWindow(WINDOW_NAME, cv::WINDOW_NORMAL);
	
	if (initialize() != 0) {
		return 1;
	}

	while (true) {	// Event Loop
		//imshow(WINDOW_NAME, img);

		if (mainLoop() != 0) {
			return 1;
		}

		// Handle Event Phrase
		int key = cv::waitKey(5);
		if (handleKeyboardEvent(key) == 1) {
			break;
		}
	}

	if (tearDown() != 0) {
		return 1;
	}
	return 0;
}