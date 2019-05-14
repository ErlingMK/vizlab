#include "pch.h"
#include "CameraConfiguration.h"


CameraConfiguration::CameraConfiguration()
{
}


CameraConfiguration::~CameraConfiguration()
{
}

void CameraConfiguration::SetTriggerMode(CameraPtr pCam, TriggerSourceEnums triggerSource, TriggerModeEnums triggerMode, TriggerSelectorEnums triggerSelector, TriggerActivationEnums triggerActivation)
{
	pCam->TriggerSource.SetValue(triggerSource);
	pCam->TriggerMode.SetValue(triggerMode);

	// A trigger signal is required for each individual image that is acquired. In Continuous mode, the trigger acquires one image.
	pCam->TriggerSelector.SetValue(triggerSelector);
	pCam->TriggerActivation.SetValue(triggerActivation);
	std::cout << "TriggerMode set to hardware!" << std::endl;
}

void CameraConfiguration::EnableImageTimestamp(CameraPtr pCam)
{
	pCam->ChunkSelector.SetValue(ChunkSelectorEnums::ChunkSelector_Timestamp);
	pCam->ChunkEnable.SetValue(true);
	pCam->ChunkModeActive.SetValue(true);
}

void CameraConfiguration::ResetTrigger(CameraList & list)
{
	std::cout << "Setting camera triggers to OFF..." << std::endl;
	for (size_t i = 0; i < list.GetSize(); ++i)
	{
		CameraPtr pCam = list.GetByIndex(i);
		pCam->Init();
		//INodeMap& nodeMap = pCam->GetNodeMap();
		pCam->TriggerMode.SetValue(Spinnaker::TriggerModeEnums::TriggerMode_Off);
		pCam->DeInit();
	}
}

void CameraConfiguration::SetBufferSize(INodeMap & sNodeMap, int numBuffers)
{
	// Retrieve Buffer Handling Mode Information
	CEnumerationPtr ptrHandlingMode = sNodeMap.GetNode("StreamBufferHandlingMode");
	if (!IsAvailable(ptrHandlingMode) || !IsWritable(ptrHandlingMode)) {
		std::cout << "Unable to set Buffer Handling mode (node retrieval). Aborting..." << std::endl << std::endl;
		return;
	}

	CEnumEntryPtr ptrHandlingModeEntry = ptrHandlingMode->GetCurrentEntry();
	if (!IsAvailable(ptrHandlingModeEntry) || !IsReadable(ptrHandlingModeEntry)) {
		std::cout << "Unable to set Buffer Handling mode (Entry retrieval). Aborting..." << std::endl << std::endl;
		return;
	}

	// Set stream buffer Count Mode to manual
	CEnumerationPtr ptrStreamBufferCountMode = sNodeMap.GetNode("StreamBufferCountMode");
	if (!IsAvailable(ptrStreamBufferCountMode) || !IsWritable(ptrStreamBufferCountMode)) {
		std::cout << "Unable to set Buffer Count Mode (node retrieval). Aborting..." << std::endl << std::endl;
		return;
	}

	CEnumEntryPtr ptrStreamBufferCountModeManual = ptrStreamBufferCountMode->GetEntryByName("Manual");
	if (!IsAvailable(ptrStreamBufferCountModeManual) || !IsReadable(ptrStreamBufferCountModeManual)) {
		std::cout << "Unable to set Buffer Count Mode entry (Entry retrieval). Aborting..." << std::endl << std::endl;
		return;
	}

	ptrStreamBufferCountMode->SetIntValue(ptrStreamBufferCountModeManual->GetValue());

	std::cout << "Stream Buffer Count Mode set to manual..." << std::endl;

	// Retrieve and modify Stream Buffer Count
	CIntegerPtr ptrBufferCount = sNodeMap.GetNode("StreamBufferCountManual");
	if (!IsAvailable(ptrBufferCount) || !IsWritable(ptrBufferCount)) {
		std::cout << "Unable to set Buffer Count (Integer node retrieval). Aborting..." << std::endl << std::endl;
		return;
	}

	// Display Buffer Info
	std::cout << std::endl << "Default Buffer Handling Mode: " << ptrHandlingModeEntry->GetDisplayName() << std::endl;
	std::cout << "Default Buffer Count: " << ptrBufferCount->GetValue() << std::endl;
	std::cout << "Maximum Buffer Count: " << ptrBufferCount->GetMax() << std::endl;

	if (numBuffers > ptrBufferCount->GetMax()) {
		ptrBufferCount->SetValue(ptrBufferCount->GetMax());
	}
	else {
		ptrBufferCount->SetValue(numBuffers);
	}
	std::cout << "Buffer count now set to: " << ptrBufferCount->GetValue() << std::endl;
}
