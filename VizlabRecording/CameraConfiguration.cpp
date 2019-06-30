#include "pch.h"
#include "CameraConfiguration.h"


CameraConfiguration::CameraConfiguration()
= default;


CameraConfiguration::~CameraConfiguration()
= default;

void CameraConfiguration::setTriggerMode(const CameraPtr& p_cam, const TriggerSourceEnums trigger_source, const TriggerModeEnums trigger_mode, const TriggerSelectorEnums trigger_selector, const TriggerActivationEnums trigger_activation)
{
	p_cam->TriggerSource.SetValue(trigger_source);
	p_cam->TriggerMode.SetValue(trigger_mode);

	// A trigger signal is required for each individual image that is acquired. In Continuous mode, the trigger acquires one image.
	p_cam->TriggerSelector.SetValue(trigger_selector);
	p_cam->TriggerActivation.SetValue(trigger_activation);
	std::cout << std::endl << "TriggerMode set to hardware!" << std::endl;
}

void CameraConfiguration::enableImageTimestamp(const CameraPtr& p_cam)
{
	p_cam->ChunkSelector.SetValue(ChunkSelectorEnums::ChunkSelector_Timestamp);
	p_cam->ChunkEnable.SetValue(true);
	p_cam->ChunkModeActive.SetValue(true);
}

void CameraConfiguration::resetTrigger(CameraList & list)
{
	std::cout << std::endl << "Setting camera triggers to OFF..." << std::endl;
	for (size_t i = 0; i < list.GetSize(); ++i)
	{
		auto p_cam = list.GetByIndex(i);
		p_cam->TriggerMode.SetValue(TriggerMode_Off);
	}
}

void CameraConfiguration::setBufferSize(INodeMap & s_node_map, const int num_buffers)
{
	// Retrieve Buffer Handling Mode Information
	CEnumerationPtr ptrHandlingMode = s_node_map.GetNode("StreamBufferHandlingMode");
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
	CEnumerationPtr ptrStreamBufferCountMode = s_node_map.GetNode("StreamBufferCountMode");
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
	CIntegerPtr ptrBufferCount = s_node_map.GetNode("StreamBufferCountManual");
	if (!IsAvailable(ptrBufferCount) || !IsWritable(ptrBufferCount)) {
		std::cout << "Unable to set Buffer Count (Integer node retrieval). Aborting..." << std::endl << std::endl;
		return;
	}

	// Display Buffer Info
	std::cout << std::endl << "Default Buffer Handling Mode: " << ptrHandlingModeEntry->GetDisplayName() << std::endl;
	std::cout << "Default Buffer Count: " << ptrBufferCount->GetValue() << std::endl;
	std::cout << "Maximum Buffer Count: " << ptrBufferCount->GetMax() << std::endl;

	if (num_buffers > ptrBufferCount->GetMax()) {
		ptrBufferCount->SetValue(ptrBufferCount->GetMax());
	}
	else {
		ptrBufferCount->SetValue(num_buffers);
	}
	std::cout << "Buffer count now set to: " << ptrBufferCount->GetValue() << std::endl;
}
