#include "pch.h"
#include "Recording.h"
#include "CameraConfiguration.h"

Recording::Recording(const InterfaceList interface_list, const RecordingParameters recording_parameters)
{
	m_interface_list_ = interface_list;
	recording_parameters_ = recording_parameters;
	retrieveAllCameras(m_interface_list_);
}

void Recording::prepareCameras()
{
	CameraConfiguration::resetTrigger(cameras_);

	for(auto i = 0; i < cameras_.GetSize(); ++i)
	{
		auto p_cam = cameras_.GetByIndex(i);
		try
		{
			auto& tl_device_node_map = p_cam->GetTLDeviceNodeMap();
			auto& tl_stream_node_map = p_cam->GetTLStreamNodeMap();

			p_cam->Init();
			CameraConfiguration::setBufferSize(tl_stream_node_map, recording_parameters_.number_of_images_per_camera);

			if(recording_parameters_.write_device_info_to_file) writeDeviceInfoToFile(tl_device_node_map);

			p_cam->AcquisitionMode.SetValue(recording_parameters_.acquisition_mode_enum);
			std::cout << "Set to: " << recording_parameters_.acquisition_mode_enum << std::endl;

			if(recording_parameters_.enable_hardware_trigger)
			{
				CameraConfiguration::setTriggerMode(p_cam, TriggerSource_Line0, TriggerMode_On, TriggerSelector_FrameStart, TriggerActivation_RisingEdge);
				std::cout << std::endl << "Hardware trigger enabled!" << std::endl;
				// CameraConfiguration::enableImageTimestamp(p_cam);
			}
			p_cam->DeInit();
		}
		catch (Exception& e)
		{
			std::cout << "Error: " << e.what() << std::endl;
		}
	}
}


void Recording::acquireImages(CameraPtr p_cam, const int num_of_images, shared_ptr<vector<ImagePtr>> images) const
{
	try
	{
		p_cam->Init();
		p_cam->BeginAcquisition();
		if (recording_parameters_.enable_hardware_trigger )std::cout << endl << "Camera: " << p_cam->DeviceSerialNumber() << ' ' << "Waiting for trigger..." << endl;
		/*std::cout << "Thread goes to sleep." << std::endl;
		std::this_thread::sleep_for(10s);
		std::cout << "Thread wakes up." << std::endl;
*/
		for (size_t image_count = 0; image_count < num_of_images; ++image_count)
		{
			try
			{
				auto p_result_image = p_cam->GetNextImage();

				//TimePoint<chrono::nanoseconds> now = chrono::high_resolution_clock::now();
				//timePoints->push_back(now);

				if (p_result_image->IsIncomplete())
				{
					std::cout << "Image incomplete with image status " << p_result_image->GetImageStatus() << "..." << endl << endl;
				}
				else
				{
					images->emplace_back(ImagePtr(p_result_image));
					// std::cout << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() << endl;
					cout << "Camera " << p_cam->DeviceSerialNumber() << " grabbed image " << image_count << ", width = " << p_result_image->GetWidth() << ", height = " << p_result_image->GetHeight() << endl;
					//p_result_image->Release();
				}
			}
			catch (Exception& e)
			{
				std::cout << "Stopped!" << e.what() << endl << endl;
			}
		}
	}
	catch (Exception& e)
	{
		std::cout << "Can't start acquisition: " << e.GetFullErrorMessage() << endl << endl;
	}
}

void Recording::startRecording()
{
	// One shared pointer for each camera points to a vector of images.
	vector<thread> threads;
	vector<shared_ptr<vector<ImagePtr>>> ptr_vector;

	//vector<shared_ptr<vector<TimePoint<chrono::nanoseconds>>>> timePoints;

	for (size_t i = 0; i < cameras_.GetSize(); ++i)
	{
		auto p_cam = cameras_.GetByIndex(i);

		ptr_vector.emplace_back(make_shared<vector<ImagePtr>>());
		thread t(&Recording::acquireImages, this, p_cam, recording_parameters_.number_of_images_per_camera, ptr_vector[i]);
		threads.push_back(move(t));

		//timePoints.emplace_back(make_shared<vector<TimePoint<chrono::nanoseconds>>>());
	}

	for (auto& t : threads)
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
	if(answer == 'y')
	{
		saveImages(ptr_vector);
	}

	for (size_t i = 0; i < cameras_.GetSize(); ++i)
	{
		auto p_cam = cameras_.GetByIndex(i);
		p_cam->EndAcquisition();
		p_cam->DeInit();
	}
	CameraConfiguration::resetTrigger(cameras_);

	cameras_.Clear();
}

void Recording::saveImages(vector<shared_ptr<vector<ImagePtr>>>& image_vectors) const
{
	auto camera = 0;
	vector<thread> threads;

	auto recording_dir = std::filesystem::path("D:/VizlabRecordings/" + currentDateTime());
	filesystem::create_directories(recording_dir);
	recording_dir += "/";

	//CreateTxtFile(currentDateTime, camList);

	// Loops through cameras
	for (const auto& v : image_vectors)
	{
		auto p_cam = cameras_.GetByIndex(camera);
		auto params = recording_parameters_;

		thread t([v ,&recording_dir, p_cam, &params]
		{
			auto camera_dir = std::filesystem::path(recording_dir);
			camera_dir += static_cast<std::string>(p_cam->DeviceSerialNumber());
			create_directory(camera_dir);

			auto counter = -1;
			for (const auto& image : *v)
			{
				++counter;
				if (!(image->IsIncomplete()))
				{
					try
					{
						ImagePtr converted_image = image->Convert(params.pixel_format_enum, params.color_processing_algorithm);

						ostringstream filename;
						filename << camera_dir.string();
						filename << "/" << counter;
						converted_image->Save(filename.str().c_str(), params.image_file_format);
						std::cout << "Image saved at " << filename.str() << endl;
					}
					catch (Exception& e)
					{
						std::cout << endl << e.GetFullErrorMessage() << endl;
					}
				}
			}
		});
		threads.push_back(move(t));
		camera++;
	}
	for (auto& t : threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}
}

void Recording::retrieveAllCameras(const InterfaceList& p_interface_list)
{
	for (auto i = 0; i < p_interface_list.GetSize(); ++i)
	{
		CameraList temp_cam_list = p_interface_list.GetByIndex(i)->GetCameras();
		if (temp_cam_list.GetSize() != 0) cameras_.Append(temp_cam_list);
	}
	if (cameras_.GetSize() == 0) std::cout << "\tNo devices detected." << endl;
	else std::cout << std::endl << "Numbers of cameras detected: " << cameras_.GetSize() << endl;
}

void Recording::queryInterface(const InterfacePtr& p_interface, const int i) const
{
	try
	{
		auto& node_map = p_interface->GetTLNodeMap();
		CStringPtr p_interface_name = node_map.GetNode("InterfaceDisplayName");

		if (IsAvailable(p_interface_name) || IsReadable(p_interface_name))
		{
			cout << i << ". " << "Interface Name: " << p_interface_name->GetValue() << endl;
		}
		else
		{
			cout << "Display-name can't be read." << endl;
		}
	}
	catch (Exception& e)
	{
		cout << "Can't query interface: " << e.what() << endl << endl;
	}
}

void Recording::writeDeviceInfoToFile(INodeMap& node_map) const
{
	fstream main_catalogue_file;
	main_catalogue_file.open("device_info.txt", std::ios::out | std::ios::app);

	cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;
	try
	{
		FeatureList_t features;
		const CCategoryPtr category = node_map.GetNode("DeviceInformation");
		if (IsAvailable(category) && IsReadable(category))
		{
			category->GetFeatures(features);

			for (FeatureList_t::const_iterator it = features.begin(); it != features.end(); ++it)
			{
				if (main_catalogue_file.is_open())
				{
					CNodePtr pfeature_node = *it;
					main_catalogue_file << pfeature_node->GetName() << " : ";
					auto p_value = static_cast<CValuePtr>(pfeature_node);
					main_catalogue_file << (IsReadable(p_value) ? p_value->ToString() : static_cast<gcstring>("Node not readable"));
					main_catalogue_file << endl;
				}
			}
			main_catalogue_file << '\n';
		}
		else
		{
			main_catalogue_file << "Device control information not available." << endl;
		}
	}
	catch (Exception& e)
	{
		main_catalogue_file << "Error: " << e.what() << endl;
		main_catalogue_file.close();
		return;
	}
	main_catalogue_file.close();
}

std::string Recording::currentDateTime()
{
	auto rawtime = time(nullptr);
	struct tm tstruct{};
	localtime_s(&tstruct, &rawtime);
	char buf[80];
	strftime(buf, sizeof(buf), "%d_%m_%y_%H_%M_%S", &tstruct);

	return buf;
}

//bool check_permissions() {
//	FILE *tempFile = fopen("test.txt", "w+");
//	if (tempFile == NULL)
//	{
//		cout << "Failed to create file in current folder.  Please check permissions." << endl;
//		cout << "Press Enter to exit..." << endl;
//		getchar();
//		return false;
//	}
//	fclose(tempFile);
//	remove("test.txt");
//	return true;
//}
