#include "pch.h"
#include "RecordingParameters.h"

RecordingParameters::RecordingParameters()
{
}

RecordingParameters::RecordingParameters(const bool continuous_writing_to_disc, const AcquisitionModeEnums acquisition_mode_enum, const bool enable_hardware_trigger,
                                         const bool write_device_info_to_file, const int number_of_images_per_camera, const PixelFormatEnums pixel_format_enum, const ColorProcessingAlgorithm color_processing_algorithm, const ImageFileFormat image_file_format):
	acquisition_mode_enum(acquisition_mode_enum), enable_hardware_trigger(enable_hardware_trigger),
	write_device_info_to_file(write_device_info_to_file), number_of_images_per_camera(number_of_images_per_camera),
	continuous_writing_to_disc(continuous_writing_to_disc), pixel_format_enum(pixel_format_enum), color_processing_algorithm(color_processing_algorithm),
	image_file_format(image_file_format)
{
}
