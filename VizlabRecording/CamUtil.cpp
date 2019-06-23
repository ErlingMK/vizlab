#include "pch.h"
#include "CamUtil.h"


CamUtil::CamUtil()
{
}


CamUtil::~CamUtil()
{
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

CameraList CamUtil::retrieveAllCameras(const InterfaceList p_interface_list)
{
	CameraList available_cameras;
	for (auto i = 0; i < p_interface_list.GetSize(); ++i)
	{
		CameraList temp_cam_list = p_interface_list.GetByIndex(i)->GetCameras();
		if (temp_cam_list.GetSize() != 0) available_cameras.Append(temp_cam_list);
	}
	return available_cameras;
}

void CamUtil::QueryInterface(InterfacePtr pInterface, int i)
{
	try
	{
		INodeMap& nodeMap = pInterface->GetTLNodeMap();
		CStringPtr pInterfaceName = nodeMap.GetNode("InterfaceDisplayName");

		if (IsAvailable(pInterfaceName) || IsReadable(pInterfaceName))
		{
			cout << i << ". " << "Interface Name: " << pInterfaceName->GetValue() << endl;
		}
		else
		{
			cout << "Displayname can't be read." << endl;
		}
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Can't query interface: " << e.what() << endl << endl;
	}
}

void CamUtil::writeDeviceInfo(INodeMap & node_map)
{
	std::fstream main_catalogue_file;
	main_catalogue_file.open("device_info.txt", std::ios::out | std::ios::app);

	cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;
	try
	{
		FeatureList_t features;
		CCategoryPtr category = node_map.GetNode("DeviceInformation");
		if (IsAvailable(category) && IsReadable(category))
		{
			category->GetFeatures(features);

			FeatureList_t::const_iterator it;
			for (it = features.begin(); it != features.end(); ++it)
			{
				if (main_catalogue_file.is_open())
				{
					CNodePtr pfeatureNode = *it;
					main_catalogue_file << pfeatureNode->GetName() << " : ";
					CValuePtr pValue = (CValuePtr)pfeatureNode;
					pValue->ToString();
					main_catalogue_file << (IsReadable(pValue) ? pValue->ToString() : (gcstring)"Node not readable");
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
	catch (Spinnaker::Exception &e)
	{
		main_catalogue_file << "Error: " << e.what() << endl;
		main_catalogue_file.close();
		return;
	}
	main_catalogue_file.close();
}

std::string CamUtil::CurrentDateTime()
{
	time_t rawtime = time(NULL);
	struct tm tstruct;
	localtime_s(&tstruct, &rawtime);
	char buf[80];
	strftime(buf, sizeof(buf), "%d_%m_%y_%H_%M_%S", &tstruct);

	return buf;
}
