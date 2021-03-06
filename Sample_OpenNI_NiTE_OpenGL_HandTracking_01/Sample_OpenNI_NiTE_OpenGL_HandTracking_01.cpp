// OpenNI2Ex01.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <OpenNI.h>
// GLUT headers
#include <gl/glut.h>
// NiTE2 headers
#include <NiTE.h>

using namespace openni;

nite::HandTracker hTracker;
int window_w = 640;
int window_h = 480;
OniRGB888Pixel* gl_texture;
int handDepth = 0;
const int RANGE = 50;
int numberOfHands = 0;

char ReadLastCharOfLine()
{
	int newChar = 0;
	int lastChar;
	fflush(stdout);
	do
	{
		lastChar = newChar;
		newChar = getchar();
	} while ((newChar != '\n')
		&& (newChar != EOF));
	return (char)lastChar;
}
bool HandleStatus(nite::Status status)
{
	if (status == nite::STATUS_OK)
		return true;
	printf("ERROR: #%d, %s", status,
		openni::OpenNI::getExtendedError());
	ReadLastCharOfLine();
	return false;
}

void gl_IdleCallback()
{
	glutPostRedisplay();
}

void gl_KeyboardCallback(unsigned char key, int x, int y)
{
	if (key == 27) // ESC Key
	{
		hTracker.destroy();
		nite::NiTE::shutdown();
		exit(0);
	}
}
void _start_handTracker(nite::HandTrackerFrameRef handsFrame) {
	const nite::Array<nite::GestureData>& gestures = handsFrame.getGestures();
	for (int i = 0; i < gestures.getSize(); ++i) {
		if (gestures[i].isComplete()) {
			nite::HandId handId;
			hTracker.startHandTracking(gestures[i].getCurrentPosition(), &handId);
		}
	}
}
void _setup_opengl_viewpoint() {
	// Clear the OpenGL buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, window_w, window_h, 0, -1.0, 1.0);
}
void _init_texture_map() {
	glTexParameteri(GL_TEXTURE_2D, 0x8191, GL_TRUE); // 0x8191 = GL_GENERATE_MIPMAP
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		window_w, window_h, 0, GL_RGB,
		GL_UNSIGNED_BYTE, gl_texture);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0.0f, (float)window_h, 0.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f((float)window_w, (float)window_h, 0.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f((float)window_w, 0.0f, 0.0f);
	glEnd();
}
void _draw_handsPoint(nite::HandTrackerFrameRef handsFrame, VideoFrameRef depthFrame, nite::Status& status, double resizeFactor, unsigned int texture_x, unsigned int texture_y) {
	glBegin(GL_POINTS);
	glColor3f(1, 0, 0);
	const nite::Array<nite::HandData>& hands = handsFrame.getHands();
	for (int i = 0; i < hands.getSize(); ++i) {
		nite::HandData hand = hands[i];
		if (hand.isNew())
			numberOfHands++;
		if (hand.isLost())
			numberOfHands--;

		if (hands[i].isTracking()) {
			
			printf("Hand #%d (%.2f, %.2f, %.2f)\n",
				hands[i].getId(),
				hands[i].getPosition().x,
				hands[i].getPosition().y,
				hands[i].getPosition().z);
			float posX, posY;
			status = hTracker.convertHandCoordinatesToDepth(
				hands[i].getPosition().x,
				hands[i].getPosition().y,
				hands[i].getPosition().z,
				&posX, &posY);
			DepthPixel* depthPixel = (DepthPixel*)
				(
					(char*)depthFrame.getData() +
					((int) posY * depthFrame.getStrideInBytes())
				) + (int)(posX);
			int depthValue = *depthPixel;
			handDepth = depthValue;
			
			printf("Hand #%d (%.2f, %.2f, %.2f) (%.2f, %.2f) - %d\n",
				hands[i].getId(),
				hands[i].getPosition().x,
				hands[i].getPosition().y,
				hands[i].getPosition().z,
				posX, posY,
				depthValue);
			if (HandleStatus(status)) {
				glVertex2f((posX * resizeFactor) + texture_x, (posY * resizeFactor) + texture_y);
			}
		}
	}
	glEnd();
	glColor3f(1.f, 1.f, 1.f);
}

int* _cal_depth_histogram(VideoFrameRef depthFrame, int* numberOfPoints) {
	// TODO : implement calculate depth histogram (return array)
	int depthHistogram[65536];
	memset(depthHistogram, 0, sizeof(depthHistogram));
	for (int y = 0; y < depthFrame.getHeight(); ++y)
	{
		DepthPixel* depthCell = (DepthPixel*)
			(
			(char*)depthFrame.getData() +
				(y * depthFrame.getStrideInBytes())
				);
		for (int x = 0; x < depthFrame.getWidth(); ++x, ++depthCell)
		{
			if (*depthCell != 0)
			{
				depthHistogram[*depthCell]++;
				(*numberOfPoints)++;
			}
		}
	}
	return depthHistogram;
}
double _cal_resize_factor(VideoFrameRef depthFrame) {
	double resizeFactor = min((window_w / (double)depthFrame.getWidth()),
		(window_h / (double)depthFrame.getHeight()));
	return resizeFactor;
}
unsigned int _cal_padding(int actual, int expect, double resizeFactor) {
	unsigned int padding = (unsigned int)(expect - (resizeFactor * actual)) / 2;
	return padding;
}

void _assign_texturePixel(OniRGB888Pixel* texturePixel, VideoFrameRef depthFrame, int y, int x, int* depthHistogram, double resizeFactor, int numberOfPoints, int numberOfHandPoints) {

	DepthPixel* depthPixel = (DepthPixel*)
		(
			(char*)depthFrame.getData() +
			(
				((int)(y / resizeFactor) *
				depthFrame.getStrideInBytes())
			)
		) + (int)(x / resizeFactor);
	if (*depthPixel != 0)
	{

		float depthValue = ((float)depthHistogram[*depthPixel] / numberOfPoints) * 255;
		float depthHandValue = ((float)depthHistogram[*depthPixel] / numberOfHandPoints) * 255;
		if (handDepth > 0 && numberOfHands > 0) {


			if (handDepth-RANGE <= *depthPixel && *depthPixel <= handDepth+RANGE) {
				texturePixel->b = 255 - depthHandValue;
				texturePixel->g = 255 - depthHandValue;
				texturePixel->r = 255 - depthHandValue;
			}
			else {
				texturePixel->b = 255 - depthValue;
				texturePixel->g = 0;
				texturePixel->r = 0;
			}
		}
		else {
			texturePixel->b = 255 - depthValue;
			texturePixel->g = 255 - depthValue;
			texturePixel->r = 255 - depthValue;
		}
	}
	else
	{
		texturePixel->b = 0;
		texturePixel->g = 0;
		texturePixel->r = 255;
	}
}

void gl_DisplayCallback()
{
	if (hTracker.isValid())
	{
		nite::Status status = nite::STATUS_OK;
		nite::HandTrackerFrameRef handsFrame;
		status = hTracker.readFrame(&handsFrame);

		if (status == nite::STATUS_OK && handsFrame.isValid())
		{
			_start_handTracker(handsFrame);

			_setup_opengl_viewpoint();

			// UPDATING TEXTURE (DEPTH 1MM TO RGB888)
			VideoFrameRef depthFrame = handsFrame.getDepthFrame();
			
			// calculate Depth Histogram
			int numberOfPoints = 0;
			int numberOfHandPoints = 0;

			int depthHistogram[65536];
			memset(depthHistogram, 0, sizeof(depthHistogram));
			for (int y = 0; y < depthFrame.getHeight(); ++y)
			{
				DepthPixel* depthCell = (DepthPixel*)
					(
						(char*)depthFrame.getData() +
						(y * depthFrame.getStrideInBytes())
					);
				for (int x = 0; x < depthFrame.getWidth(); ++x, ++depthCell)
				{
					if (*depthCell != 0)
					{
						depthHistogram[*depthCell]++;
						numberOfPoints++;

						if (handDepth > 0 && numberOfHands > 0) {
							if (handDepth - RANGE <= *depthCell && *depthCell <= handDepth + RANGE)
								numberOfHandPoints++;
						}
					}
				}
			}
			for (int nIndex = 1; nIndex < sizeof(depthHistogram) / sizeof(int); nIndex++)
			{
				depthHistogram[nIndex] += depthHistogram[nIndex - 1];
			}


			double resizeFactor = _cal_resize_factor(depthFrame);
			unsigned int texture_x = _cal_padding(depthFrame.getWidth(), window_w, resizeFactor);
			unsigned int texture_y = _cal_padding(depthFrame.getHeight(), window_h, resizeFactor);
			
			for (unsigned int y = 0; y < (window_h - 2 * texture_y); y++) {
				OniRGB888Pixel* texturePixel = gl_texture + ((y + texture_y) * window_w) + texture_x;
				for (unsigned int x = 0; x < (window_w - 2 * texture_x); x++, texturePixel++) {
					_assign_texturePixel(texturePixel, depthFrame, y, x, depthHistogram, resizeFactor, numberOfPoints, numberOfHandPoints);
				}
			}

			_init_texture_map();
			_draw_handsPoint(handsFrame, depthFrame, status, resizeFactor, texture_x, texture_y);
			glutSwapBuffers();
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	nite::Status status = nite::STATUS_OK;

	printf("Initializing NiTE ...\r\n");
	status = nite::NiTE::initialize();
	if (!HandleStatus(status))
		return 1;

	printf("Creating a hand tracker object ...\r\n");
	status = hTracker.create();
	if (!HandleStatus(status))
		return 1;
	printf("Done.\r\n");

	printf("Searching for wave gesture ...\r\n");
	status = hTracker.startGestureDetection(nite::GESTURE_HAND_RAISE);
	if (!HandleStatus(status)) 
		return 1;

	printf("Initializing OpenGL ...\r\n");
	gl_texture = (OniRGB888Pixel*)malloc(window_w * window_h * sizeof(OniRGB888Pixel));
	
	glutInit(&argc, (char**)argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(window_w, window_h);
	glutCreateWindow("OpenGL | OpenNI 2.x CookBook Sample");
	glPointSize(10.0);
	
	glutKeyboardFunc(gl_KeyboardCallback);
	glutDisplayFunc(gl_DisplayCallback);
	glutIdleFunc(gl_IdleCallback);
	
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	
	printf("Starting OpenGL rendering process ...\r\n");
	glutMainLoop();
	return 0;
}