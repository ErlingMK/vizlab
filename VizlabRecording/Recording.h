#pragma once
#include <thread>
#include <filesystem>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream> 
#include <vector>
#include <memory>
#include <chrono>
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include "RecordingParameters.h"

using namespace Spinnaker;
using namespace GenApi;
using namespace GenICam;
using namespace std;

class Recording
{
public:
	Recording(InterfaceList interface_list, RecordingParameters recording_parameters);

	void prepareCameras();
	void startRecording();

private:
	void saveImages(vector<shared_ptr<vector<ImagePtr>>>& image_vectors) const;
	void acquireImages(CameraPtr p_cam, int num_of_images, shared_ptr<vector<ImagePtr>> images) const;
	void queryInterface(const InterfacePtr& p_interface, int i) const;
	void retrieveAllCameras(const InterfaceList& p_interface_list);
	void writeDeviceInfoToFile(INodeMap& node_map) const;
	static std::string currentDateTime();

	InterfaceList m_interface_list_;
	CameraList cameras_;
	RecordingParameters recording_parameters_;

};

