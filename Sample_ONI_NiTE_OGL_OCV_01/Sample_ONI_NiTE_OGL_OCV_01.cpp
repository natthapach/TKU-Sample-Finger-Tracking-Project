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
const int RANGE = 40;
const string WINDOW_NAME = "image";

nite::HandTracker handTracker;
openni::VideoStream sensor;
openni::Device device;
int handDepth = 0;
int numberOfHands = 0;
int depthHistogram[65536];
uchar imgFrame[WINDOW_H][WINDOW_W][3];	// bgr
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

int initializeRGB() {
	openni::Status status = openni::STATUS_OK;
	status = openni::OpenNI::initialize();
	device.open(openni::ANY_DEVICE);
	bool isValid = device.isValid();
	if (status != openni::STATUS_OK)
		return 1;
	status = sensor.create(device, openni::SENSOR_COLOR);
	openni::VideoMode vmod;
	vmod.setFps(30);
	vmod.setPixelFormat(openni::PIXEL_FORMAT_RGB888);
	vmod.setResolution(640, 480);
	status = sensor.setVideoMode(vmod);
	if (status != openni::STATUS_OK) 
		return 1;
	printf("Done.\r\n");
	printf("Starting stream ...\r\n");
	status = sensor.start();
	if (status != openni::STATUS_OK) 
		return 1;
	printf("Done.\r\n");
	return 0;
}

openni::VideoFrameRef readRGBFrame() {
	openni::Status status = openni::STATUS_OK;
	openni::VideoStream* streamPointer = &sensor;
	int streamReadyIndex;
	status = openni::OpenNI::waitForAnyStream(&streamPointer, 1, &streamReadyIndex, 100);

	openni::VideoFrameRef newFrame;
	if (streamReadyIndex != 0)
		return newFrame;
	status = sensor.readFrame(&newFrame);
	return newFrame;
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

void modifyImageHandThreshold(int depthPixel, int x, int y, int numberOfHandPoints) {
	if (depthPixel != 0 && (handDepth-RANGE <= depthPixel && depthPixel <= handDepth+RANGE)) {
		uchar depthValue = (uchar)(((float)depthHistogram[depthPixel] / numberOfHandPoints) * 255);
		imgFrame[y][x][0] = 255 - depthValue;
		imgFrame[y][x][1] = 255 - depthValue;
		imgFrame[y][x][2] = 255 - depthValue;
	}
	else {
		imgFrame[y][x][0] = 0;
		imgFrame[y][x][1] = 255;
		imgFrame[y][x][2] = 0;
	}
}
void modifyImageNormal(int depthPixel, int x, int y, int numberOfPoints) {
	if (depthPixel != 0) {
		uchar depthValue = (uchar)(((float)depthHistogram[depthPixel] / numberOfPoints) * 255);
		imgFrame[y][x][0] = 255 - depthValue;
		imgFrame[y][x][1] = 255 - depthValue;
		imgFrame[y][x][2] = 255 - depthValue;
	}
	else {
		imgFrame[y][x][0] = 0;
		imgFrame[y][x][1] = 0;
		imgFrame[y][x][2] = 0;
	}
}

void modifyImageRGB(openni::VideoFrameRef newFrame) {
	for (unsigned int y = 0; y < WINDOW_H; y++) {
		for (unsigned int x = 0; x < WINDOW_W; x++) {
			OniRGB888Pixel* streamPixel = (OniRGB888Pixel*) ((char*)newFrame.getData() + (y * newFrame.getStrideInBytes())) + x;
			imgFrame[y][x][0] = streamPixel->b;
			imgFrame[y][x][1] = streamPixel->g;
			imgFrame[y][x][2] = streamPixel->r;
		}
	}
}

void modifyImage(openni::VideoFrameRef depthFrame, int numberOfPoints, int numberOfHandPoint) {
	for (unsigned int y = 0; y < WINDOW_H; y++) {
		for (unsigned int x = 0; x < WINDOW_W; x++) {
			openni::DepthPixel* depthPixel = (openni::DepthPixel*)
				((char*)depthFrame.getData() + (y*depthFrame.getStrideInBytes())) + x;
			
			if (handDepth != 0 && numberOfHands > 0) {
				modifyImageHandThreshold((int)*depthPixel, x, y, numberOfHandPoint);
			}
			else {
				modifyImageNormal((int)*depthPixel, x, y, numberOfPoints);
			}
		}
	}
}

void settingHandValue(nite::HandTrackerFrameRef handsFrame) {
	const nite::Array<nite::HandData>& hands = handsFrame.getHands();

	for (int i = 0; i < hands.getSize(); i++) {
		nite::HandData hand = hands[i];

		if (hand.isTracking()) {
			nite::Point3f position = hand.getPosition();
			float x, y;
			handTracker.convertHandCoordinatesToDepth(
				hand.getPosition().x,
				hand.getPosition().y,
				hand.getPosition().z,
				&x, &y
			);
			openni::VideoFrameRef depthFrame = handsFrame.getDepthFrame();
			openni::DepthPixel* depthPixel = (openni::DepthPixel*) ((char*)depthFrame.getData() + ((int)y * depthFrame.getStrideInBytes())) + (int)x;
			handDepth = *depthPixel;
		}

		if (hand.isLost())
			numberOfHands--;
		if (hand.isNew())
			numberOfHands++;
	}
}

void findFingerPoint() {
	if (numberOfHands > 0) {
		cv::Mat blur = cv::Mat(WINDOW_H, WINDOW_W, CV_8UC3);
		cv::Mat corner = cv::Mat::zeros(WINDOW_H, WINDOW_W, CV_8UC3);
		cv::Mat corner_norm;
		cv::cvtColor(img, blur, CV_BGR2GRAY);
		cv::GaussianBlur(blur, blur, cv::Size(13, 13), 0, 0);
		cv::cornerHarris(blur, corner, 2, 3, 0.04, cv::BORDER_DEFAULT);
		cv::normalize(corner, corner_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());

		//return corner_norm;
		//return corner;
	}
}

void drawHandsPoint(nite::HandTrackerFrameRef handsFrame) {
	const nite::Array<nite::HandData>& hands = handsFrame.getHands();

	for (int i = 0; i < hands.getSize(); i++) {
		nite::HandData hand = hands[i];
		
		if (hand.isTracking()) {
			float posX, posY;
			handTracker.convertHandCoordinatesToDepth(
				hand.getPosition().x,
				hand.getPosition().y,
				hand.getPosition().z,
				&posX, &posY
			);

			cv::circle(img, cv::Point((int)posX, (int)posY), 5, cv::Scalar(255, 0, 0), -1, cv::LINE_8);
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
			img = cv::Mat(WINDOW_H, WINDOW_W, CV_8UC3, &imgFrame);

			settingHandValue(handsFrame);
			findFingerPoint();

			drawHandsPoint(handsFrame);

			cv::imshow(WINDOW_NAME, img);
		}
	}
	return 0;
}

int mainLoopRGB() {

	openni::Status status = openni::STATUS_OK;
	openni::VideoStream* streamPointer = &sensor;
	int streamReadyIndex;
	status = openni::OpenNI::waitForAnyStream(&streamPointer, 1, &streamReadyIndex, 500);

	if (status != openni::STATUS_OK && streamReadyIndex != 0)
		return 0;
	openni::VideoFrameRef newFrame;
	status = sensor.readFrame(&newFrame);

	if (status != openni::STATUS_OK && !newFrame.isValid())
		return 0;

	modifyImageRGB(newFrame);
	img = cv::Mat(WINDOW_H, WINDOW_W, CV_8UC3, &imgFrame);
	cv::imshow(WINDOW_NAME, img);
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
	
	if (initializeRGB() != 0) {
		return 1;
	}

	while (true) {	// Event Loop
		//imshow(WINDOW_NAME, img);

		if (mainLoopRGB() != 0) {
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