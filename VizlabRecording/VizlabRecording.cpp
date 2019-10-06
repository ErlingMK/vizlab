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
#include "Recording.h"


using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

template<class Duration>
using TimePoint = chrono::time_point<chrono::high_resolution_clock, Duration>;

const int MAX_IMAGES_PER_CAMERA = 20000;


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

std::string currentDateTime()
{
	auto rawtime = time(nullptr);
	struct tm tstruct {};
	localtime_s(&tstruct, &rawtime);
	char buf[80];
	strftime(buf, sizeof(buf), "%d_%m_%y_%H_%M_%S", &tstruct);

	return buf;
}

void copyTxtFile(const string& new_rec_directory_name, const string& serial_numbers_file_path, const string& main_recording_directory)
{
	try
	{
		std::filesystem::copy_file(serial_numbers_file_path, main_recording_directory + "/"+ new_rec_directory_name + ".txt");
	} 
	catch (std::filesystem::filesystem_error& e)
	{
		std::cout << e.what() << '\n';
	}
}

void runMultipleCameras()
{
	const string main_recordings_directory = "C:/Recordings";

	const auto new_rec_directory_name = currentDateTime();

	// You should make a file and write the serial numbers in the order you want the images to appear in the combined image. This file will be copied and put in each new recording directory and used by the ImageCombiner. 
	const string serial_numbers_file_path = main_recordings_directory + "/" + "serialnumbers.txt";

	const auto new_rec_directory = main_recordings_directory + "/" + new_rec_directory_name;

	SystemPtr system = System::GetInstance();
	InterfaceList interface_list = system->GetInterfaces();
	
	int num_of_images;
	std::cout << endl << "Enter number of images to capture:" << endl;
	std::cin >> num_of_images;
	while (num_of_images <= 0 || num_of_images > MAX_IMAGES_PER_CAMERA)
	{
		std::cout << "Invalid number of images..." << endl;
		std::cout << endl << "Enter number of images to capture: " << endl;
		std::cin >> num_of_images;
	}
	
	const RecordingParameters recording_parameters{ true, AcquisitionMode_Continuous, false, true, num_of_images, PixelFormat_Mono8, HQ_LINEAR, PGM, new_rec_directory};

	Recording recording{ interface_list, recording_parameters };

	recording.startRecording();

	interface_list.Clear();
	system->ReleaseInstance();

	copyTxtFile(new_rec_directory_name, serial_numbers_file_path, main_recordings_directory);
}

int main(int argc, char const *argv[])
{
	runMultipleCameras();
}
