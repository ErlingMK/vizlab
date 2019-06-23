#pragma once
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;

class RecordingParameters
{
public:
	RecordingParameters();
	RecordingParameters(const bool continuous_writing_to_disc, AcquisitionModeEnums acquisition_mode_enum, bool enable_hardware_trigger,
	                    bool write_device_info_to_file, int number_of_images_per_camera, PixelFormatEnums pixel_format_enum, ColorProcessingAlgorithm color_processing_algorithm, ImageFileFormat image_file_format);

	AcquisitionModeEnums acquisition_mode_enum;
	bool enable_hardware_trigger;
	bool write_device_info_to_file;
	int number_of_images_per_camera;
	bool continuous_writing_to_disc;
	PixelFormatEnums pixel_format_enum;
	ColorProcessingAlgorithm color_processing_algorithm;
	ImageFileFormat image_file_format;

private:

};

