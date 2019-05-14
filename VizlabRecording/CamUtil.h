#pragma once
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <thread>
#include <fstream>
#include <iostream>
#include <sstream> 
#include <vector>
#include <memory>
#include <chrono>
#include <sys/stat.h>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

class CamUtil
{
public:
	CamUtil();
	~CamUtil();

	static CameraList RetrieveAllCameras(InterfaceList pInterfaceList);
	static void QueryInterface(InterfacePtr pInterface, int i);
	static void WriteDeviceInfo(Spinnaker::GenApi::INodeMap & nodeMap);
	static std::string CurrentDateTime();
};

