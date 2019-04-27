#include "pch.h"
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
using namespace std;

static void SetTriggerMode(CameraPtr pCam, TriggerSourceEnums triggerSource, TriggerModeEnums triggerMode, TriggerSelectorEnums triggerSelector, TriggerActivationEnums triggerActivation)
{
	pCam->TriggerSource.SetValue(triggerSource);
	pCam->TriggerMode.SetValue(triggerMode);

	// A trigger signal is required for each individual image that is acquired. In Continuous mode, the trigger acquires one image.
	pCam->TriggerSelector.SetValue(triggerSelector);
	pCam->TriggerActivation.SetValue(triggerActivation);
	cout << "TriggerMode set to hardware!" << endl;
}

static void EnableImageTimestamp(CameraPtr pCam)
{    
	pCam->ChunkSelector.SetValue(ChunkSelectorEnums::ChunkSelector_Timestamp);
	pCam->ChunkEnable.SetValue(true);
	pCam->ChunkModeActive.SetValue(true);
}

static void ResetTrigger(CameraList &list)
{
	cout << "Setting camera triggers to OFF..." << endl;
	for (size_t i = 0; i < list.GetSize(); ++i)
	{
		CameraPtr pCam = list.GetByIndex(i);
		pCam->Init();
		//INodeMap& nodeMap = pCam->GetNodeMap();
		pCam->TriggerMode.SetValue(Spinnaker::TriggerModeEnums::TriggerMode_Off);
		pCam->DeInit();
	}
}

static void SetBufferSize(INodeMap & sNodeMap, int numBuffers)
{
	// Retrieve Buffer Handling Mode Information
	CEnumerationPtr ptrHandlingMode = sNodeMap.GetNode("StreamBufferHandlingMode");
	if (!IsAvailable(ptrHandlingMode) || !IsWritable(ptrHandlingMode)) {
		cout << "Unable to set Buffer Handling mode (node retrieval). Aborting..." << endl << endl;
		return;
	}

	CEnumEntryPtr ptrHandlingModeEntry = ptrHandlingMode->GetCurrentEntry();
	if (!IsAvailable(ptrHandlingModeEntry) || !IsReadable(ptrHandlingModeEntry)) {
		cout << "Unable to set Buffer Handling mode (Entry retrieval). Aborting..." << endl << endl;
		return;
	}

	// Set stream buffer Count Mode to manual
	CEnumerationPtr ptrStreamBufferCountMode = sNodeMap.GetNode("StreamBufferCountMode");
	if (!IsAvailable(ptrStreamBufferCountMode) || !IsWritable(ptrStreamBufferCountMode)) {
		cout << "Unable to set Buffer Count Mode (node retrieval). Aborting..." << endl << endl;
		return;
	}

	CEnumEntryPtr ptrStreamBufferCountModeManual = ptrStreamBufferCountMode->GetEntryByName("Manual");
	if (!IsAvailable(ptrStreamBufferCountModeManual) || !IsReadable(ptrStreamBufferCountModeManual)) {
		cout << "Unable to set Buffer Count Mode entry (Entry retrieval). Aborting..." << endl << endl;
		return;
	}

	ptrStreamBufferCountMode->SetIntValue(ptrStreamBufferCountModeManual->GetValue());

	cout << "Stream Buffer Count Mode set to manual..." << endl;

	// Retrieve and modify Stream Buffer Count
	CIntegerPtr ptrBufferCount = sNodeMap.GetNode("StreamBufferCountManual");
	if (!IsAvailable(ptrBufferCount) || !IsWritable(ptrBufferCount)) {
		cout << "Unable to set Buffer Count (Integer node retrieval). Aborting..." << endl << endl;
		return;
	}

	// Display Buffer Info
	cout << endl << "Default Buffer Handling Mode: " << ptrHandlingModeEntry->GetDisplayName() << endl;
	cout << "Default Buffer Count: " << ptrBufferCount->GetValue() << endl;
	cout << "Maximum Buffer Count: " << ptrBufferCount->GetMax() << endl;

	if (numBuffers > ptrBufferCount->GetMax()) {
		ptrBufferCount->SetValue(ptrBufferCount->GetMax());
	}
	else {
		ptrBufferCount->SetValue(numBuffers);
	}
	cout << "Buffer count now set to: " << ptrBufferCount->GetValue() << endl;
}