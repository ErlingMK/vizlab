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

CameraList CamUtil::RetrieveAllCameras(InterfaceList pInterfaceList)
{
	CameraList availableCameras;
	for (int i = 0; i < pInterfaceList.GetSize(); ++i)
	{
		CameraList tempCamList = pInterfaceList.GetByIndex(i)->GetCameras();
		if (tempCamList.GetSize() != 0) availableCameras.Append(tempCamList);
	}
	return availableCameras;
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

void CamUtil::WriteDeviceInfo(Spinnaker::GenApi::INodeMap & nodeMap)
{
	std::fstream mainCatalogueFile;
	mainCatalogueFile.open("device_info.txt", std::ios::out | std::ios::app);

	cout << endl << "*** DEVICE INFORMATION ***" << endl << endl;
	try
	{
		FeatureList_t features;
		CCategoryPtr category = nodeMap.GetNode("DeviceInformation");
		if (IsAvailable(category) && IsReadable(category))
		{
			category->GetFeatures(features);

			FeatureList_t::const_iterator it;
			for (it = features.begin(); it != features.end(); ++it)
			{
				if (mainCatalogueFile.is_open())
				{
					CNodePtr pfeatureNode = *it;
					mainCatalogueFile << pfeatureNode->GetName() << " : ";
					CValuePtr pValue = (CValuePtr)pfeatureNode;
					pValue->ToString();
					mainCatalogueFile << (IsReadable(pValue) ? pValue->ToString() : (gcstring)"Node not readable");
					mainCatalogueFile << endl;
				}
			}
			mainCatalogueFile << '\n';
		}
		else
		{
			mainCatalogueFile << "Device control information not available." << endl;
		}
	}
	catch (Spinnaker::Exception &e)
	{
		mainCatalogueFile << "Error: " << e.what() << endl;
		mainCatalogueFile.close();
		return;
	}
	mainCatalogueFile.close();
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
