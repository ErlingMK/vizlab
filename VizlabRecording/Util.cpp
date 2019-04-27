#include "pch.h"
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <thread>
#include <iostream>
#include <sstream> 
#include <vector>
#include <memory>
#include <chrono>
#include <sys/stat.h>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

#pragma warning(disable : 4996)

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

static std::string CurrentDateTime()
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%d_%m_%y_%H_%M_%S", &tstruct);

	return buf;
}

static CameraList RetrieveAllCameras(InterfaceList pInterfaceList)
{
	CameraList availableCameras;
	for (int i = 0; i < pInterfaceList.GetSize(); ++i)
	{
		CameraList tempCamList = pInterfaceList.GetByIndex(i)->GetCameras();
		if (tempCamList.GetSize() != 0) availableCameras.Append(tempCamList);
	}
	return availableCameras;
}

static void QueryInterface(InterfacePtr pInterface, int i)
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

static void PrintDeviceInfo(Spinnaker::GenApi::INodeMap & nodeMap)
{
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
				CNodePtr pfeatureNode = *it;
				cout << pfeatureNode->GetName() << " : ";
				CValuePtr pValue = (CValuePtr)pfeatureNode;
				pValue->ToString();
				cout << (IsReadable(pValue) ? pValue->ToString() : (gcstring)"Node not readable");
				cout << endl;
			}
		}
		else
		{
			cout << "Device control information not available." << endl;
		}
	}
	catch (Spinnaker::Exception &e)
	{
		cout << "Error: " << e.what() << endl;
		return;
	}
}
