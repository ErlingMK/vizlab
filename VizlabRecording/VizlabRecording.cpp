#include "pch.h"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <filesystem>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream> 
#include <vector>
#include <memory>
#include <chrono>
#include <sys/stat.h>
#include "CamUtil.h"
#include "CameraConfiguration.h"
//#include <pthread.h>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

template<class Duration>
using TimePoint = chrono::time_point<chrono::high_resolution_clock, Duration>;

// TODO: Finn måte å hente mengde RAM og regn utifra det.
const int MAX_IMAGES_PER_CAMERA = 1000;
const string RecordingsDirectory = "D:/VizlabRecordings/";
const string SerialNumbersFile= "D:/VizlabRecordings/serialnumbers.txt";
string currentDateTime;

void CreateTimeDiffFile(shared_ptr<vector<TimePoint<chrono::nanoseconds>>> timePoints, std::string serialNumber)
{
	auto timeDiffFile = std::filesystem::path(RecordingsDirectory + currentDateTime + "/" + serialNumber + ".txt");
	std::fstream txtFile(timeDiffFile, std::ios::out | std::ios::app);

	if (txtFile.is_open())
	{
		for (size_t i = 0; i < timePoints->size(); i++)
		{
			if (i != 0)
			{
				auto first = chrono::duration_cast<chrono::microseconds>(timePoints->at(i-1).time_since_epoch());
				auto second = chrono::duration_cast<chrono::microseconds>(timePoints->at(i).time_since_epoch());

				auto diff = second - first;
				txtFile << diff.count() << endl;
			}
			else
			{
				txtFile << 0 << endl;
			}
		}
	}
	txtFile.close();
}

void AcquireImages(CameraPtr pCam, const unsigned int num_of_images, shared_ptr<vector<ImagePtr>> images,    shared_ptr<vector<TimePoint<chrono::nanoseconds>>> timePoints)
{
	try
	{
		pCam->Init();
		pCam->BeginAcquisition();
		std::cout << endl << "Camera: " << pCam->DeviceSerialNumber() << ' ' << "Waiting for trigger..." << endl;
		for (size_t image_count = 0; image_count < num_of_images; ++image_count)
		{
			try
			{
				// Holds until trigger signal
				ImagePtr pResultImage = pCam->GetNextImage();
				//TimePoint<chrono::nanoseconds> now = chrono::high_resolution_clock::now();
				//timePoints->push_back(now);

				if (pResultImage->IsIncomplete())
				{
					std::cout << "Image incomplete with image status " << pResultImage->GetImageStatus() << "..." << endl << endl;
				}
				else
				{
					images->push_back(ImagePtr(pResultImage));
					// std::cout << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() << endl;
					//cout << "Camera " << pCam->DeviceSerialNumber() << " grabbed image " << image_count << ", width = " << pResultImage->GetWidth() << ", height = " << pResultImage->GetHeight() << endl;
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
		pCam->Init();

		INodeMap& tlDeviceNodeMap = pCam->GetTLDeviceNodeMap();
		INodeMap& tlStreamNodeMap = pCam->GetTLStreamNodeMap();
		
		CameraConfiguration::SetBufferSize(tlStreamNodeMap, numBuffers);
		CamUtil::WriteDeviceInfo(tlDeviceNodeMap);

		pCam->AcquisitionMode.SetValue(Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous);
		std::cout << "Set to continuous!" << endl;
		
		//CameraConfiguration::SetTriggerMode(pCam, TriggerSourceEnums::TriggerSource_Line0, TriggerModeEnums::TriggerMode_On, TriggerSelectorEnums::TriggerSelector_FrameStart, TriggerActivationEnums::TriggerActivation_RisingEdge);
		//CameraConfiguration::EnableImageTimestamp(pCam);
		pCam->DeInit();
	}
	catch (Spinnaker::Exception &e)
	{
		std::cout << "Error: " << e.what() << endl;
		return;
	}
}

void CreateTxtFile(string currentDateTime, CameraList& camList)
{
	std::fstream mainCatalogueFile;
	//mainCatalogueFile.open("Recordings/" + currentDateTime + "/" + currentDateTime + ".txt");
	try
	{
		std::filesystem::copy_file(SerialNumbersFile, RecordingsDirectory + currentDateTime + "/" + currentDateTime + ".txt");
	} 
	catch (std::filesystem::filesystem_error& e)
	{
		std::cout << e.what() << '\n';
	}
	/*
	for (size_t i = 0; i < camList.GetSize(); i++)
	{
		mainCatalogueFile << camList.GetByIndex(i)->DeviceSerialNumber() << "\n";
	}
	*/
	mainCatalogueFile.close();
}

void SaveImages(vector<shared_ptr<vector<ImagePtr>>> &image_vectors, CameraList& camList) 
{	
	int camera = 0;
	int counter = 0;
	
	auto recordingDir = std::filesystem::path(RecordingsDirectory + currentDateTime);
	std::filesystem::create_directories(recordingDir);
	recordingDir += "/";
	
	CreateTxtFile(currentDateTime, camList);

	// Loops through cameras
	for (shared_ptr<vector<ImagePtr>> v : image_vectors)
	{
		CameraPtr pCam = camList.GetByIndex(camera);

		auto cameraDir = std::filesystem::path(recordingDir);
		cameraDir += (std::string)pCam->DeviceSerialNumber();
		std::filesystem::create_directory(cameraDir);
		//mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		// Loops through each image in one camera
		for (ImagePtr image : *v)
		{
			if (!(image->IsIncomplete()))
			{
				try
				{
					ImagePtr converted_image = image->Convert(Spinnaker::PixelFormatEnums::PixelFormat_RGB8, Spinnaker::ColorProcessingAlgorithm::HQ_LINEAR);

					ostringstream filename;
					filename << cameraDir.string();
					filename << "/" << counter;
					
					converted_image->Save(filename.str().c_str(), Spinnaker::ImageFileFormat::PPM);
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
	CameraList camList = CamUtil::RetrieveAllCameras(pInterfaceList);
	
	int i = pInterfaceList.GetSize();
	vector<thread> threads;

	unsigned int numCameras = camList.GetSize();
	if (numCameras == 0) {
		std::cout << "\tNo devices detected." << endl;
		return;
	}

	CameraConfiguration::ResetTrigger(camList);

	std::cout << "Numbers of cameras detected: " << numCameras << endl;

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
	vector<shared_ptr<vector<TimePoint<chrono::nanoseconds>>>> timePoints;

	currentDateTime = CamUtil::CurrentDateTime();

	for (size_t i = 0; i < numCameras; ++i)
	{
		CameraPtr pCam = camList.GetByIndex(i);
		ptr_vector.emplace_back(make_shared<vector<ImagePtr>>());
		timePoints.emplace_back(make_shared<vector<TimePoint<chrono::nanoseconds>>>());
		thread t(AcquireImages, pCam, num_of_images, ptr_vector[i], timePoints[i]);
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
		/*for (int i = 0; i < timePoints.size(); ++i)
		{
			CameraPtr pCam = camList.GetByIndex(i);
			CreateTimeDiffFile(timePoints[i], (string)pCam->DeviceSerialNumber());
		}*/
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
	CameraConfiguration::ResetTrigger(camList);
	camList.Clear();
}

int main(int argc, char const *argv[])
{
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
