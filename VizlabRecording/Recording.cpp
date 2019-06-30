#include "pch.h"
#include "Recording.h"
#include "CameraConfiguration.h"




Recording::Recording(const InterfaceList interface_list, const RecordingParameters recording_parameters)
{
	m_interface_list_ = interface_list;
	recording_parameters_ = recording_parameters;
	retrieveAllCameras(m_interface_list_);
}

void Recording::startRecording()
{
	prepareCameras();

	vector<thread> threads;
	//vector<shared_ptr<vector<TimePoint<chrono::nanoseconds>>>> timePoints;

	if(!recording_parameters_.continuous_writing_to_disc)
	{
		vector<shared_ptr<vector<ImagePtr>>> ptr_vector;
		for (size_t i = 0; i < cameras_.GetSize(); ++i)
		{
			auto p_cam = cameras_.GetByIndex(i);

			ptr_vector.emplace_back(make_shared<vector<ImagePtr>>());
			thread t(&Recording::acquireImagesSaveAfterRec, this, p_cam, recording_parameters_.number_of_images_per_camera, ptr_vector[i]);
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
		if (answer == 'y')
		{
			saveImages(ptr_vector);
		}

		for (size_t i = 0; i < cameras_.GetSize(); ++i)
		{
			auto p_cam = cameras_.GetByIndex(i);
			p_cam->EndAcquisition();
		}
	}
	else
	{
		auto recording_dir = std::filesystem::path(recording_parameters_.recording_directory);
		filesystem::create_directories(recording_dir);
		recording_dir += "/";
		for(size_t i = 0; i < cameras_.GetSize(); ++i)
		{
			auto p_cam = cameras_.GetByIndex(i);
			thread t(&Recording::acquireImagesSaveDuringRec, this, p_cam, recording_parameters_.number_of_images_per_camera, recording_dir);
			threads.push_back(move(t));
		}
		for (auto& t : threads)
		{
			if (t.joinable())
			{
				t.join();
			}
		}
	}

	resetCameras();
}

void Recording::acquireImagesSaveAfterRec(CameraPtr p_cam, const int num_of_images, shared_ptr<vector<ImagePtr>> images) const
{
	try
	{
		p_cam->BeginAcquisition();
		if (recording_parameters_.enable_hardware_trigger )std::cout << endl << "Camera: " << p_cam->DeviceSerialNumber() << ' ' << "Waiting for trigger..." << endl;
	
		for (size_t image_count = 0; image_count < num_of_images; ++image_count)
		{
			try
			{
				const auto p_result_image = p_cam->GetNextImage();

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
					p_result_image->Release();
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

void Recording::acquireImagesSaveDuringRec(const CameraPtr& p_cam, const int num_of_images, std::filesystem::path recording_directory) const
{
	try
	{
		const auto cam_directory = recording_directory += static_cast<std::string>(p_cam->DeviceSerialNumber());
		create_directory(recording_directory);

		p_cam->BeginAcquisition();

		if (recording_parameters_.enable_hardware_trigger)std::cout << endl << "Camera: " << p_cam->DeviceSerialNumber() << ' ' << "Waiting for trigger..." << endl;

		for (size_t image_count = 0; image_count < num_of_images; ++image_count)
		{
			try
			{
				const auto p_result_image = p_cam->GetNextImage();

				if (p_result_image->IsIncomplete())
				{
					std::cout << "Image incomplete with image status " << p_result_image->GetImageStatus() << "..." << endl << endl;
					p_result_image->Release();
				}
				else
				{
					cout << "Camera " << p_cam->DeviceSerialNumber() << " grabbed image " << image_count << ", width = " << p_result_image->GetWidth() << ", height = " << p_result_image->GetHeight() << endl;

					auto converted_image = p_result_image->Convert(recording_parameters_.pixel_format_enum, recording_parameters_.color_processing_algorithm);

					ostringstream filename;
					filename << cam_directory.string();
					filename << "/" << image_count;

					converted_image->Save(filename.str().c_str(), recording_parameters_.image_file_format);
					std::cout << "Image saved at " << filename.str() << endl;

					p_result_image->Release();
				}
			}
			catch (Exception& e)
			{
				std::cout << "Stopped!" << e.what() << endl << endl;
			}
		}
		p_cam->EndAcquisition();
	}
	catch (Exception& e)
	{
		std::cout << "Can't start acquisition: " << e.GetFullErrorMessage() << endl << endl;
	}
}

void Recording::saveImages(vector<shared_ptr<vector<ImagePtr>>>& image_vectors) const
{
	auto camera = 0;
	vector<thread> threads;

	auto recording_dir = std::filesystem::path(recording_parameters_.recording_directory);
	filesystem::create_directories(recording_dir);
	recording_dir += "/";


	for (const auto& v : image_vectors)
	{
		auto p_cam = cameras_.GetByIndex(camera);
		auto params = recording_parameters_;

		thread t([v ,recording_dir, p_cam, params]
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

void Recording::prepareCameras()
{
	CameraConfiguration::resetTrigger(cameras_);

	for(auto i = 0; i < cameras_.GetSize(); ++i)
	{
		auto p_cam = cameras_.GetByIndex(i);
		try
		{
			p_cam->Init();
			
			auto& tl_device_node_map = p_cam->GetTLDeviceNodeMap();
			auto& tl_stream_node_map = p_cam->GetTLStreamNodeMap();

			CameraConfiguration::setBufferSize(tl_stream_node_map, recording_parameters_.number_of_images_per_camera);

			if(recording_parameters_.write_device_info_to_file) writeDeviceInfoToFile(tl_device_node_map);

			p_cam->AcquisitionMode.SetValue(recording_parameters_.acquisition_mode_enum);
			std::cout << "Set to: " << recording_parameters_.acquisition_mode_enum << std::endl;

			// CameraConfiguration::enableImageTimestamp(p_cam);

			if(recording_parameters_.enable_hardware_trigger)
			{
				CameraConfiguration::setTriggerMode(p_cam, TriggerSource_Line0, TriggerMode_On, TriggerSelector_FrameStart, TriggerActivation_RisingEdge);
				std::cout << std::endl << "Hardware trigger enabled!" << std::endl;
			}
		}
		catch (Exception& e)
		{
			std::cout << "Error: " << e.what() << std::endl;
		}
	}
}

void Recording::resetCameras()
{
	CameraConfiguration::resetTrigger(cameras_);

	for (int i = 0; i < cameras_.GetSize(); ++i)
	{
		auto p_cam = cameras_.GetByIndex(i);
		p_cam->DeInit();
	}
	cameras_.Clear();
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
	fstream catalog_file;
	catalog_file.open("device_info.txt", std::ios::out | std::ios::app);

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
				if (catalog_file.is_open())
				{
					CNodePtr pfeature_node = *it;
					catalog_file << pfeature_node->GetName() << " : ";
					auto p_value = static_cast<CValuePtr>(pfeature_node);
					catalog_file << (IsReadable(p_value) ? p_value->ToString() : static_cast<gcstring>("Node not readable"));
					catalog_file << endl;
				}
			}
			catalog_file << '\n';
		}
		else
		{
			catalog_file << "Device control information not available." << endl;
		}
	}
	catch (Exception& e)
	{
		catalog_file << "Error: " << e.what() << endl;
		catalog_file.close();
		return;
	}
	catalog_file.close();
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
