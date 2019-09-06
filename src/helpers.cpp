
#include "helpers.h"

using namespace std;


void draw_preview(cv::Mat& img_preview, cv::Mat& img_target, float scaling, int x, int y)
{
	if(img_preview.empty()){ return; }
	
	cv::Mat img_preview_scaled;
	if (scaling == 1.0f)
	{
		img_preview_scaled = img_preview;
	}
	else
	{
		cv::resize(img_preview, img_preview_scaled, cv::Size(), scaling, scaling);
	}

	if (img_target.rows > img_preview_scaled.rows && img_target.cols > img_preview_scaled.cols)
	{
		if (x == -1) { x = round(0.5f * (img_target.cols - img_preview_scaled.cols)); }
		if (y == -1) { y = round(0.5f * (img_target.rows - img_preview_scaled.rows)); }
		img_preview_scaled.copyTo(img_target(cv::Rect(x, y, img_preview_scaled.cols, img_preview_scaled.rows)));
	}
}



#ifdef _WIN32
#include "deps/DeviceEnumerator.h" 
#endif

shared_ptr<Camera> select_camera(string message, int id)
{
	// first try the provided id
	shared_ptr<Camera> capture = nullptr;
	if (id != -1)
	{
		capture = make_shared<Camera>(id);
		if (capture->isOpened())
		{
			return capture;
		}
	}

	// if that fails, start interactive selection dialog
	cout << "\n=== Menu Camera Selection ===\n";
	#ifdef _WIN32
	{
		DeviceEnumerator de;
		// Video Devices
		auto devices = de.getVideoDevicesMap();
		if (devices.size() == 0) { std::cout << "no video devices detected.\n"; }

		// Print information about the devices
		for (auto const& device : devices)
		{
			std::cout << "id:" << device.first << " Name: " << device.second.deviceName << std::endl;
		}
	}
	#endif
	while (true)
	{
		cout << message;
		int cam_nr = 0;
		cin >> cam_nr;
		capture = make_shared<Camera>(cam_nr);
		if (capture->isOpened()) { break; }
		cerr << "\ncould not open and initialize camera nr. " << cam_nr << ". please try again!\n";
	}

	// TODO: dialog for webcam resolution entry
	//capture->set(cv::CAP_PROP_FRAME_WIDTH, width);
	//capture->set(cv::CAP_PROP_FRAME_HEIGHT, height);

	return capture;
}


void grab_focus(const char* wname)
{
	cv::destroyWindow(wname);
	cv::namedWindow(wname);
}