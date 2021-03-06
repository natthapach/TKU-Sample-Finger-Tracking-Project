// FingerTracking01.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Application.h"
#include "RGBActivator.h"
#include "DepthActivator.h"

using namespace std;

RGBActivator rgbActivator;
DepthActivator depthActivator;
std::shared_ptr<Activator> depthActivatorPtr(new DepthActivator());
std::shared_ptr<Activator> rgbActivatorPtr(new RGBActivator());
Application mainApplication(depthActivatorPtr);

int keyboardCallback(int key) {
	if (key == 27)
		return 1;
	if (key == 'c' || key == 'C') {
		std::printf("key C\n");
		mainApplication.setMainFrameActivator(rgbActivatorPtr);
	}
	else if (key == 'd' || key == 'D') {
		cout << "key D\n";
		cout << depthActivator.getName() << endl;
		mainApplication.setMainFrameActivator(depthActivatorPtr);
	}
		
	return 0;
}

int main()
{
	mainApplication.registerActivator(depthActivatorPtr);
	mainApplication.registerActivator(rgbActivatorPtr);
	mainApplication.setOnKeyboardCallback(keyboardCallback);

	mainApplication.onInitial("Finger Track");
	mainApplication.start();
    return 0;
}

