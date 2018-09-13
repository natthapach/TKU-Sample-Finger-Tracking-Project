#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <OpenNI.h>
#include <gl/glut.h>	// GLUT headers
#include <NiTE.h>		// NiTE2 headers
#include "Callback.h"

#include "math_utils.h"

using namespace openni;

unsigned short cal_MaxDepth(VideoFrameRef newFrame) {
	unsigned short maxDepth = 0;
	if (newFrame.getSensorType() == SENSOR_DEPTH)
	{
		for (int y = 0; y < newFrame.getHeight(); ++y)
		{
			DepthPixel* depthCell = (DepthPixel*)(
				(char*)newFrame.getData() +
				(y * newFrame.getStrideInBytes())
				);
			for (int x = 0; x < newFrame.getWidth();
				++x, ++depthCell)
			{
				if (maxDepth < *depthCell) {
					maxDepth = *depthCell;
				}
			}
		}
	}
	return maxDepth;
}
double cal_resizeFactor(VideoFrameRef newFrame, int window_w, int window_h) {
	double resizeFactor = min(
		(window_w / (double)newFrame.getWidth()),
		(window_h / (double)newFrame.getHeight()));
	return resizeFactor;
}
