#pragma once
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <thread>
#include <iostream>
#include <sstream> 
#include <vector>
#include <memory>
#include <chrono>
#include <sys/stat.h>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class CameraConfiguration
{
public:
	CameraConfiguration();
	~CameraConfiguration();

	static void SetTriggerMode(CameraPtr pCam, TriggerSourceEnums triggerSource, TriggerModeEnums triggerMode, TriggerSelectorEnums triggerSelector, TriggerActivationEnums triggerActivation);
	static void EnableImageTimestamp(CameraPtr pCam);
	static void ResetTrigger(CameraList &list);
	static void SetBufferSize(INodeMap & sNodeMap, int numBuffers);
};

