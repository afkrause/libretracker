
#include "helpers.h"

using namespace std;

// create a string with date and time like this: 20080501123104  ( 01.05. 2008, 12:31.04 uhr)
std::string date_time_str()
{
	time_t t = time(nullptr);
	auto mytime = localtime(&t);

	int year = mytime->tm_year - 100 + 2000;	// liefert mytime->tm_year ist z[2]. 103 - das steht f�r 2003  
	int month = mytime->tm_mon + 1;			// monate werden anscheinend von 0-11 gez�hlt
	int day = mytime->tm_mday;
	int hour = mytime->tm_hour;
	int minute = mytime->tm_min;
	int second = mytime->tm_sec;

	return
		std::to_string(year) +
		add_leading_zeros<2>(std::to_string(month)) +
		add_leading_zeros<2>(std::to_string(day)) +
		add_leading_zeros<2>(std::to_string(hour)) +
		add_leading_zeros<2>(std::to_string(minute)) +
		add_leading_zeros<2>(std::to_string(second));
}



std::tuple<float, int, int> draw_preview(cv::Mat& img_preview, cv::Mat& img_target, float scaling, int x, int y)
{
	if(img_preview.empty()){ return make_tuple(scaling, x, y); }
	
	cv::Mat img_preview_scaled;

	if (scaling < 0.0f)
	{
		// fit img_preview into img_target while keeping the aspect ration of img_preview

		// first, scale the image up/down to the target size based on both preview-and target width
		scaling = float(img_target.cols) / float(img_preview.cols);

		// now, check if the height of the scaled image is fitting into the target. if not, scale according to the height ratio. 		
		if (scaling * img_preview.rows > img_target.rows)
		{
			scaling = float(img_target.rows) / float(img_preview.rows);
		}
	}

	if (scaling == 1.0f)
	{
		img_preview_scaled = img_preview;
	}
	else
	{
		cv::resize(img_preview, img_preview_scaled, cv::Size(), scaling, scaling);
	}

	if (img_target.rows >= img_preview_scaled.rows && img_target.cols >= img_preview_scaled.cols)
	{
		if (x == -1) { x = round(0.5f * (img_target.cols - img_preview_scaled.cols)); }
		if (y == -1) { y = round(0.5f * (img_target.rows - img_preview_scaled.rows)); }
		img_preview_scaled.copyTo(img_target(cv::Rect(x, y, img_preview_scaled.cols, img_preview_scaled.rows)));
	}

	return make_tuple(scaling, x, y);
}



#ifdef _WIN32
#include "deps/DeviceEnumerator.h" 
#endif

shared_ptr<Camera> select_camera(string message, int id, cv::VideoCaptureAPIs backend)
{
	// first try the provided id
	shared_ptr<Camera> capture = nullptr;
	if (id != -1)
	{
		capture = make_shared<Camera>(id, backend);
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


string to_fourcc_string(int code)
{
	char fourCC[5];
	for (int i = 0; i < 4; i++)
	{
		fourCC[3 - i] = code >> (i * 8);
	}
	fourCC[4] = '\0';
	return string(fourCC);
}