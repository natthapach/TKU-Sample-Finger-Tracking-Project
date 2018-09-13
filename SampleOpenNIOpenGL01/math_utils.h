#pragma once
#include "stdafx.h"
#include <stdio.h>
#include <conio.h>
#include <OpenNI.h>
#include <gl/glut.h>	// GLUT headers
#include <NiTE.h>		// NiTE2 headers

using namespace openni;

unsigned short cal_MaxDepth(VideoFrameRef newFrame);
double cal_resizeFactor(VideoFrameRef newFrame, int window_w, int window_h);
