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

	static void setTriggerMode(const CameraPtr& p_cam, TriggerSourceEnums trigger_source, TriggerModeEnums trigger_mode, TriggerSelectorEnums trigger_selector, TriggerActivationEnums trigger_activation);
	static void enableImageTimestamp(const CameraPtr& p_cam);
	static void resetTrigger(CameraList &list);
	static void setBufferSize(INodeMap & s_node_map, int num_buffers);
};

