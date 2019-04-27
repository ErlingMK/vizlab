#include "pch.h"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <filesystem>
#include <thread>
#include <iostream>
#include <sstream> 
#include <vector>
#include <memory>
#include <chrono>
#include <sys/stat.h>
#include "Util.cpp"
#include "CameraConfig.cpp"
//#include <pthread.h>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

// TODO: Finn måte å hente mengde RAM og regn utifra det.
const int MAX_IMAGES_PER_CAMERA = 1000;

void AcquireImages(CameraPtr pCam, const unsigned int num_of_images, shared_ptr<vector<ImagePtr>> images)
{
	try
	{
		pCam->Init();
		pCam->BeginAcquisition();
		//ImagePtr mImage;

		std::cout << endl << "Camera: " << pCam->DeviceSerialNumber() << ' ' << "Waiting for trigger..." << endl;
		for (size_t image_count = 0; image_count < num_of_images; ++image_count)
		{
			try
			{
				ImagePtr pResultImage = pCam->GetNextImage();
				if (pResultImage->IsIncomplete())
				{
					std::cout << "Image incomplete with image status " << pResultImage->GetImageStatus() << "..." << endl << endl;
				}
				else
				{
					/*mImage = Image::Create();
					mImage->DeepCopy(pResultImage);
					*/
					images->emplace_back(ImagePtr(pResultImage));
					/*
					const ChunkData &data = pResultImage->GetChunkData();
					cout << data.GetTimestamp() << endl;
					*/
					std::cout << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() << endl;
					// cout << "Camera " << pCam->DeviceSerialNumber() << " grabbed image " << image_count << ", width = " << pResultImage->GetWidth() << ", height = " << pResultImage->GetHeight() << endl;
					pResultImage->Release();
				}
			}
			catch (Spinnaker::Exception &e)
			{
				std::cout << "Stopped!" << e.what() << endl << endl;
			}
		}
	}
	catch (Spinnaker::Exception &e)
	{
		std::cout << "Can't start acquistion: " << e.GetFullErrorMessage() << endl << endl;
	}
}

//Sets acquisition mode to continuous, changes number of buffers used, enables trigger mode and prints camera info.
void PrepareCamera(CameraPtr pCam, int numBuffers)
{
	try
	{
		INodeMap& tlDeviceNodeMap = pCam->GetTLDeviceNodeMap();
		INodeMap& tlStreamNodeMap = pCam->GetTLStreamNodeMap();
		SetBufferSize(tlStreamNodeMap, numBuffers);
		PrintDeviceInfo(tlDeviceNodeMap);
		
		pCam->Init();
		pCam->AcquisitionMode.SetValue(Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous);
		std::cout << "Set to continuous!" << endl;
		//SetTriggerMode(pCam, TriggerSourceEnums::TriggerSource_Line0, TriggerModeEnums::TriggerMode_On, TriggerSelectorEnums::TriggerSelector_FrameStart, TriggerActivationEnums::TriggerActivation_RisingEdge);
		//EnableImageTimestamp(pCam);
		pCam->DeInit();
	}
	catch (Spinnaker::Exception &e)
	{
		std::cout << "Error: " << e.what() << endl;
		return;
	}
}

void SaveImages(vector<shared_ptr<vector<ImagePtr>>> &image_vectors, CameraList& camList) 
{	
	int camera = 0;
	int counter = 0;
	auto recordingsDir = std::filesystem::path("Recordings\\" + CurrentDateTime());
	std::filesystem::create_directories(recordingsDir);
	recordingsDir += "\\";

	// Loops through cameras
	for (shared_ptr<vector<ImagePtr>> v : image_vectors)
	{
		CameraPtr pCam = camList.GetByIndex(camera);

		auto cameraDir = std::filesystem::path(recordingsDir);
		cameraDir += pCam->DeviceSerialNumber().c_str();
		std::filesystem::create_directory(cameraDir);
		//mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		// Loops through each image in one camera
		for (ImagePtr image : *v)
		{
			if (!(image->IsIncomplete()))
			{
				try
				{
					ImagePtr converted_image = image->Convert(Spinnaker::PixelFormat_BayerRG8, Spinnaker::ColorProcessingAlgorithm::HQ_LINEAR);

					ostringstream filename;
					filename << cameraDir.string();
					filename << "\\image" << counter << ".png";
					
					converted_image->Save(filename.str().c_str());
					std::cout << "Image saved at " << filename.str() << endl;

					counter++;
				}
				catch (Spinnaker::Exception& e)
				{
					std::cout << endl << e.GetFullErrorMessage() << endl;
				}
			}
		}
		counter = 0;
		camera++;
	}
}

void RunMultipleCameras(InterfaceList pInterfaceList)
{
	CameraList camList = RetrieveAllCameras(pInterfaceList);
	
	int i = pInterfaceList.GetSize();
	vector<thread> threads;

	unsigned int numCameras = camList.GetSize();
	if (numCameras == 0) {
		std::cout << "\tNo devices detected." << endl;
		return;
	}

	ResetTrigger(camList);

	std::cout << "Numbers of cameras detected: " << numCameras << endl;
	//std::cin.ignore();
	//if (std::cin.get() != '\n') {
	//	camList.Clear();
	//	return;
	//}

	int num_of_images;
	std::cout << endl << "Enter number of images to capture:" << endl;
	std::cin >> num_of_images;

	while (num_of_images <= 0 || num_of_images > MAX_IMAGES_PER_CAMERA)
	{
		std::cout << "Invalid number of images..." << endl;
		std::cout << endl << "Enter number of images to capture: " << endl;
		std::cin >> num_of_images;
	}

	for (size_t i = 0; i < numCameras; ++i)
	{
		CameraPtr pCam = camList.GetByIndex(i);
		PrepareCamera(pCam, num_of_images);
	}

	// Vector of shared pointers to vector of images. One per camera.
	vector<shared_ptr<vector<ImagePtr>>> ptr_vector;

	for (size_t i = 0; i < numCameras; ++i)
	{
		CameraPtr pCam = camList.GetByIndex(i);
		ptr_vector.emplace_back(make_shared<vector<ImagePtr>>());  
		thread t(AcquireImages, pCam, num_of_images, ptr_vector[i]);
		threads.emplace_back(move(t));
	}

	for (thread &t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	char answer;
	std::cout << endl << "Recording complete. Save images?" << endl;
	std::cout << "y/n?" << endl;
	std::cin >> answer;

	if (answer == 'y')
	{
		SaveImages(ptr_vector, camList);
	}
	else
	{
		std::cout << "Recording not saved." << endl;
	}

	for (size_t i = 0; i < numCameras; ++i) 
	{
		CameraPtr pCam = camList.GetByIndex(i);
		// EndAcquisition deletes all buffers and therefore deletes all images in memory. 
		pCam->EndAcquisition();
		pCam->DeInit();
	}
	ResetTrigger(camList);
	camList.Clear();
}

int main(int argc, char const *argv[]) {
	int result = 0;

	/*if (!check_permissions()) {
		exit(-1);
	}
	*/

	SystemPtr system = System::GetInstance();
	InterfaceList interfaceList = system->GetInterfaces();

	int numInterfaces = interfaceList.GetSize();
	std::cout << "Number of interfaces detected: " << numInterfaces << endl << endl;

	RunMultipleCameras(interfaceList);

	interfaceList.Clear();
	system->ReleaseInstance();
}
