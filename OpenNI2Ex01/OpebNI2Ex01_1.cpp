// OpenNI2Ex01.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <OpenNI.h>

using namespace openni;

char ReadLastCharOfLine()
{
	int newChar = 0;
	int lastChar;
	fflush(stdout);
	do
	{
		lastChar = newChar;
		newChar = getchar();
	} while ((newChar != '\n' && (newChar != EOF)));

	return (char)lastChar;
}

bool HandleStatus(Status status)
{
	if (status == STATUS_OK) {
		return true;
	}
	printf("ERROR: #%d, %s", status, OpenNI::getExtendedError());
	ReadLastCharOfLine();
	return false;
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("OpenNI Version is %d.%d.%d.%d",
		OpenNI::getVersion().major,
		OpenNI::getVersion().minor,
		OpenNI::getVersion().maintenance,
		OpenNI::getVersion().build);
	printf("Scanning machine for devices and loading "
		"modules/divers ...\r\n");
	Status status = STATUS_OK;
	status = OpenNI::initialize();
	if (!HandleStatus(status))
		return 1;

	printf("Completed.\r\n");

	// show list of all device
	openni::Array<openni::DeviceInfo> listOfDevices;
	openni::OpenNI::enumerateDevices(&listOfDevices);
	int numberOfDevices = listOfDevices.getSize();

	if (numberOfDevices > 0) {
		printf("%d Device(s) are available to use.\r\n\r\n",
			numberOfDevices);
		for (int i = 0; i < numberOfDevices; i++)
		{
			openni::DeviceInfo device = listOfDevices[i];
			printf("%d. %s->%s (VID: %d | PID: %d) is connected "
				" at %s\r\n",
				i,
				device.getVendor(),
				device.getName(),
				device.getUsbVendorId(),
				device.getUsbProductId(),
				device.getUri());
		}
	}
	else {
		printf("No device connected to this machine.");
	}

	printf("Press ENTER to continue.\r\n");
	ReadLastCharOfLine();

	// open device
	printf("Opening any device ...\r\n");
	Device device;
	status = device.open(ANY_DEVICE);
	if (!HandleStatus(status)) return 1;
	printf("%s Opened, Completed.\r\n",
		device.getDeviceInfo().getName());
	printf("Press ENTER to continue.\r\n");
	ReadLastCharOfLine();
	printf("Checking if depth stream is supported ...\r\n");
	if (!device.hasSensor(SENSOR_DEPTH))
	{
		printf("Depth stream not supported by this device. "
			"Press ENTER to exit.\r\n");
		ReadLastCharOfLine();
		return 1;
	}
	printf("Asking device to create a depth stream ...\r\n");
	VideoStream sensor;
	status = sensor.create(device, SENSOR_DEPTH);
	if (!HandleStatus(status)) return 1;
	printf("Completed.\r\n");
	printf("Changing sensor video mode to 640x480@30fps.\r\n");
	VideoMode depthVM;
	depthVM.setFps(30);

	depthVM.setResolution(640, 480);
	depthVM.setPixelFormat(PIXEL_FORMAT_DEPTH_1_MM);
	status = sensor.setVideoMode(depthVM);
	if (!HandleStatus(status)) return 1;
	printf("Completed.\r\n");
	printf("Asking sensor to start receiving data ...\r\n");
	status = sensor.start();
	if (!HandleStatus(status)) return 1;
	printf("Completed.\r\n");

	printf("Press ENTER to exit.\r\n");
	ReadLastCharOfLine();
	sensor.destroy();
	device.close();
	OpenNI::shutdown();
	return 0;
}


