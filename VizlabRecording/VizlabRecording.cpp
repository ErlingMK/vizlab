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
#include "CameraConfiguration.h"
#include "Recording.h"
//#include "CamUtil.h"
//#include <pthread.h>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

template<class Duration>
using TimePoint = chrono::time_point<chrono::high_resolution_clock, Duration>;

// TODO: Finn måte å hente mengde RAM og regn utifra det.
const int MAX_IMAGES_PER_CAMERA = 1000;
const string RecordingsDirectory = "D:/VizlabRecordings/";
const string SerialNumbersFile= "D:/VizlabRecordings/serialnumbers.txt";
string currentDateTime;


//void CreateTimeDiffFile(shared_ptr<vector<TimePoint<chrono::nanoseconds>>> timePoints, std::string serialNumber)
//{
//	auto timeDiffFile = std::filesystem::path(RecordingsDirectory + currentDateTime + "/" + serialNumber + ".txt");
//	std::fstream txtFile(timeDiffFile, std::ios::out | std::ios::app);
//
//	if (txtFile.is_open())
//	{
//		for (size_t i = 0; i < timePoints->size(); i++)
//		{
//			if (i != 0)
//			{
//				auto first = chrono::duration_cast<chrono::microseconds>(timePoints->at(i-1).time_since_epoch());
//				auto second = chrono::duration_cast<chrono::microseconds>(timePoints->at(i).time_since_epoch());
//
//				auto diff = second - first;
//				txtFile << diff.count() << endl;
//			}
//			else
//			{
//				txtFile << 0 << endl;
//			}
//		}
//	}
//	txtFile.close();
//}

void CreateTxtFile(string currentDateTime, CameraList& camList)
{
	std::fstream mainCatalogueFile;
	mainCatalogueFile.open("Recordings/" + currentDateTime + "/" + currentDateTime + ".txt");
	try
	{
		std::filesystem::copy_file(SerialNumbersFile, RecordingsDirectory + currentDateTime + "/" + currentDateTime + ".txt");
	} 
	catch (std::filesystem::filesystem_error& e)
	{
		std::cout << e.what() << '\n';
	}
	mainCatalogueFile.close();
}

void runMultipleCameras(const InterfaceList& p_interface_list)
{
	int num_of_images;
	std::cout << endl << "Enter number of images to capture:" << endl;
	std::cin >> num_of_images;
	while (num_of_images <= 0 || num_of_images > MAX_IMAGES_PER_CAMERA)
	{
		std::cout << "Invalid number of images..." << endl;
		std::cout << endl << "Enter number of images to capture: " << endl;
		std::cin >> num_of_images;
	}
	const RecordingParameters recording_parameters{ true, AcquisitionMode_Continuous, false, true, num_of_images, PixelFormat_Mono8, HQ_LINEAR, PGM};

	Recording recording{ p_interface_list, recording_parameters };
	recording.prepareCameras();
	recording.startRecording();
}

int main(int argc, char const *argv[])
{
	SystemPtr system = System::GetInstance();
	InterfaceList interface_list = system->GetInterfaces();

	const auto num_interfaces = interface_list.GetSize();
	std::cout << "Number of interfaces detected: " << num_interfaces << endl << endl;

	runMultipleCameras(interface_list);

	interface_list.Clear();
	system->ReleaseInstance();
}
