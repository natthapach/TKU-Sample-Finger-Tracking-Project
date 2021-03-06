// SampleOpenNIOpenGL01.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <OpenNI.h>
#include <gl/glut.h>	// GLUT headers
#include <NiTE.h>		// NiTE2 headers
#include "Callback.h"
#include "math_utils.h"

#include <iostream>
//#include <opencv2/opencv.hpp>

using namespace openni;
//using namespace cv;

int window_w = 640;
int window_h = 480;
OniRGB888Pixel* gl_texture;
VideoStream selectedSensor;
Device device;

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
bool HandleStatus(Status status)
{
	if (status == STATUS_OK)
		return true;
	printf("ERROR: #%d, %s", status,
		OpenNI::getExtendedError());
	ReadLastCharOfLine();
	return false;
}
void SetActiveSensor(SensorType sensorType, Device* device)
{
	Status status = STATUS_OK;
	printf("Checking if stream is supported ...\r\n");
	if (!device->hasSensor(sensorType))
	{
		printf("Stream not supported by this device.\r\n");
		return;
	}
	if (selectedSensor.isValid())
	{
		printf("Stop and destroy old stream.\r\n");
		selectedSensor.stop();
		selectedSensor.destroy();
	}
	printf("Asking device to create a stream ...\r\n");
	status = selectedSensor.create(*device, sensorType);
	if (!HandleStatus(status)) return;
	printf("Setting video mode to 640x480x30 RGB24 ...\r\n");
	VideoMode vmod;
	vmod.setFps(30);
	vmod.setPixelFormat(PIXEL_FORMAT_RGB888);
	vmod.setResolution(640, 480);
	status = selectedSensor.setVideoMode(vmod);
	if (!HandleStatus(status)) return;
	printf("Done.\r\n");
	printf("Starting stream ...\r\n");
	status = selectedSensor.start();
	if (!HandleStatus(status)) return;
	printf("Done.\r\n");
}
void gl_KeyboardCallback(unsigned char key, int x, int y)
{
	if (key == 27) // ESC Key
	{
		selectedSensor.destroy();
		OpenNI::shutdown();
		exit(0);
	}
	else if (key == 'C' || key == 'c')
	{
		if (device.isValid())
		{
			printf("\r\n-->Setting active sensor to COLOR\r\n");
			SetActiveSensor(SENSOR_COLOR, &device);
		}
	}
	else if (key == 'I' || key == 'i')
	{
		if (device.isValid())
		{
			printf("\r\n-->Setting active sensor to IR\r\n");
			SetActiveSensor(SENSOR_IR, &device);
		}
	}
	else if (key == 'd' || key == 'D')
	{
		if (device.isValid())
		{
			printf("\r\n-->Setting active sensor to Depth\r\n");
			SetActiveSensor(SENSOR_DEPTH, &device);
		}
	}
}
void gl_IdleCallback()
{
	glutPostRedisplay();
}

int _openSensor() {
	Status status = STATUS_OK;
	VideoStream* streamPointer = &selectedSensor;
	int streamReadyIndex;
	status = OpenNI::waitForAnyStream(&streamPointer, 1, &streamReadyIndex, 500);
	if (status != STATUS_OK)
		throw status;
	return streamReadyIndex;
}
void _setUpOpenGLViewPoint() {
	// Clear the OpenGL buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, window_w, window_h, 0, -1.0, 1.0);
}
void _assign_texturePixel_Depth(OniRGB888Pixel* texturePixel, VideoFrameRef newFrame, int y, int x, unsigned short maxDepth, double resizeFactor) {
	DepthPixel* streamPixel = (DepthPixel*)
		(
		(char*)newFrame.getData() +
			((int)(y / resizeFactor) * newFrame.getStrideInBytes())
			) +
			(int)(x / resizeFactor);
	if (*streamPixel != 0) {
		char depthValue = ((float)*streamPixel / maxDepth) * 255;
		texturePixel->b = 255 - depthValue;
		texturePixel->r = 255 - depthValue;
		texturePixel->g = 255 - depthValue;
	}
	else {
		texturePixel->b = 0;
		texturePixel->r = 0;
		texturePixel->g = 0;
		texturePixel->g = 0;
	}
}
void _assign_texturePixel_Camera_IR(OniRGB888Pixel* texturePixel, VideoFrameRef newFrame, int y, int x, double resizeFactor) {
	OniRGB888Pixel* streamPixel = (OniRGB888Pixel*)
		(
			(char*)newFrame.getData() +
			((int)(y / resizeFactor) * newFrame.getStrideInBytes())
		) + (int)(x / resizeFactor);
	memcpy(texturePixel, streamPixel, sizeof(OniRGB888Pixel));
}
void _create_OpenGL_TextureMap() {
	glTexParameteri(GL_TEXTURE_2D, 0x8191, GL_TRUE);
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
	glutSwapBuffers();
}

void gl_DisplayCallback()
{
	try {
		if (selectedSensor.isValid())
		{
			Status status = STATUS_OK;
			int streamreadyindex = _openSensor();
			if (streamreadyindex == 0)
			{

				VideoFrameRef newFrame;
				status = selectedSensor.readFrame(&newFrame);

				if (status == STATUS_OK && newFrame.isValid())
				{
					unsigned short maxDepth = 0;	// for depth sensor

					_setUpOpenGLViewPoint();
					
					// UPDATING & RESIZING TEXTURE (RGB888 TO RGB888)
					if (newFrame.getSensorType() == SENSOR_DEPTH)
					{
						maxDepth = cal_MaxDepth(newFrame);
					}

					double resizeFactor = cal_resizeFactor(newFrame, window_w, window_h);

					unsigned int texture_x = (unsigned int)(window_w - (resizeFactor * newFrame.getWidth())) / 2;
					unsigned int texture_y = (unsigned int)(window_h - (resizeFactor * newFrame.getHeight())) / 2;
					for (unsigned int y = 0; y < (window_h - 2 * texture_y); ++y)
					{
						OniRGB888Pixel* texturePixel = gl_texture + ((y + texture_y) * window_w) + texture_x;
						for (unsigned int x = 0; x < (window_w - 2 * texture_x); x++)
						{
							if (newFrame.getSensorType() == SENSOR_DEPTH) {
								_assign_texturePixel_Depth(texturePixel, newFrame, y, x, maxDepth, resizeFactor);
							}
							else {
								_assign_texturePixel_Camera_IR(texturePixel, newFrame, y, x, resizeFactor);
							}

							texturePixel += 1; // Moves variable by 3 bytes
						}
					}
					// Create the OpenGL texture map
					_create_OpenGL_TextureMap();
				}
			}
		}
	}
	catch (Status e) {
		
	}
	
}

//int main(int argc, char const *argv[]) {
//	/* 畫布 */
//	Mat img(270, 720, CV_8UC3, Scalar(56, 50, 38));
//	/* 直線 */
//	line(img, Point(20, 40), Point(120, 140), Scalar(255, 0, 0), 3);
//	/* 實心方塊 */
//	rectangle(img, Point(150, 40), Point(250, 140), Scalar(0, 0, 255), -1);
//	/* 實心圓 */
//	circle(img, Point(330, 90), 50, Scalar(0, 255, 0), -1);
//	/* 空心橢圓 */
//	ellipse(img, Point(460, 90), Size(60, 40), 45, 0, 360, Scalar(255, 255, 0), 2);
//	/* 不規則圖形 */
//	Point points[1][5];
//	int x = 40, y = 540;
//	points[0][0] = Point(0 + y, 50 + x);
//	points[0][1] = Point(40 + y, 0 + x);
//	points[0][2] = Point(110 + y, 35 + x);
//	points[0][3] = Point(74 + y, 76 + x);
//	points[0][4] = Point(28 + y, 96 + x);
//	const Point* ppt[1] = { points[0] };
//	int npt[] = { 5 };
//	polylines(img, ppt, npt, 1, 1, Scalar(0, 255, 255), 3);
//	/* 繪出文字 */
//	putText(img, "Test Passed !!", Point(10, 230), 0, 3, Scalar(255, 170, 130), 3);
//	/* 開啟畫布 */
//	namedWindow("OpenCV Test By:Charlotte.HonG", WINDOW_AUTOSIZE);
//	imshow("OpenCV Test By:Charlotte.HonG", img);
//	waitKey(0);
//	return 0;
//}

int _tmain(int argc, _TCHAR* argv[])
{
	Status status = STATUS_OK;
	printf("Scanning machine for devices and loading "
		"modules/drivers ...\r\n");
	status = OpenNI::initialize();
	if (!HandleStatus(status)) return 1;
	printf("Completed.\r\n");
	printf("Opening first device ...\r\n");
	status = device.open(ANY_DEVICE);
	if (!HandleStatus(status)) return 1;
	printf("%s Opened, Completed.\r\n",
		device.getDeviceInfo().getName());
	printf("Initializing OpenGL ...\r\n");
	gl_texture = (OniRGB888Pixel*)malloc(
		window_w * window_h * sizeof(OniRGB888Pixel));
	glutInit(&argc, (char**)argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(window_w, window_h);
	glutCreateWindow("OpenGL | OpenNI 2.x CookBook Sample");
	glutKeyboardFunc(gl_KeyboardCallback);
	glutDisplayFunc(gl_DisplayCallback);
	glutIdleFunc(gl_IdleCallback);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	printf("Starting OpenGL rendering process ...\r\n");
	SetActiveSensor(SENSOR_COLOR, &device);
	printf("Press C for color and I for IR.\r\n");
	glutMainLoop();
	return 0;
}

