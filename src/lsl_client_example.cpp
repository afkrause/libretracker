#pragma comment(lib, "opencv41/build/x64/vc15/lib/opencv_world411.lib")
#pragma comment(lib, "labstreaminglayer/build/install/LSL/lib/liblsl64.lib")

#include <iostream>
#include<array>

#include <opencv2/opencv.hpp>

#include <lsl_cpp.h>
#include "lt_lsl_protocol.h"


// optional but helpful for centering the headset such that all markers are visible by the scene camera
int draw_markers(cv::Mat& img, int x, int y, int w, int h, const std::vector<double>& marker_data, const std::vector<std::vector<double>>& eye_data);

int main()
{
	using namespace lsl;
	using namespace std;
	using namespace cv;

	//////////// init labstreaming layer and stream ////////////////////////
	cout << "trying to resolve libretracker stream(s)..\n";

	vector<stream_info> results = resolve_stream("type", "LT_EYE");
	if (results.size() == 0) { throw runtime_error("Libretracker not running / not streaming data."); }
	cout << "found LT_EYE stream!\n";

	// assume that only one eyetracker is connected, hence simply take the first stream
	stream_inlet inlet_eye(results[0]);
	cout << "opened a eye data stream. Stream Information:\n\n " << inlet_eye.info().as_xml();

	// optional marker stream
	results = resolve_stream("type", "LT_MARKER");
	stream_inlet inlet_marker(results[0]);
	cout << "marker data stream opened.\n";
	cout << "\n\n";

	const int screen_w = 1280;
	const int screen_h = 900;


	//////////////// load the AR markers ////////////////
	const int marker_size = 150;
	std::array<cv::Mat, 4> img_markers;
	// array<string, 4> marker_file_names{ "marker_1.jpg", "marker_5.jpg", "marker_10.jpg","marker_25.jpg" };

		// according to the aruco main developer, ARUCO_MIP_36h12 is the preferred dictionary. 
		// https://stackoverflow.com/questions/50076117/what-are-the-advantages-disadvantages-between-the-different-predefined-aruco-d
	array<string, 4> marker_file_names { "aruco_mip_36h12_00002.png", "aruco_mip_36h12_00004.png", "aruco_mip_36h12_00006.png", "aruco_mip_36h12_00008.png" };


	for (size_t i = 0; i < marker_file_names.size(); i++)
	{
		auto tmp = imread("assets/" + marker_file_names[i]);
		resize( tmp, img_markers[i], Size(marker_size, marker_size));
	}

	Mat img; // render everything to this image
	Mat img_screen_background = Mat(screen_h, screen_w, CV_8UC3, Scalar(255, 255, 255)); // white background

	cout << "entering main loop. continously reading data from stream inlet.\n";
	vector< vector<double> > data;
	vector<double> marker_data;

	float x=0, y=0;
	while (true)
	{
		// this grabs all data from all channels that have accumulated up to this time point
		inlet_eye.pull_chunk(data);

		
		// render the markers
		img_screen_background.copyTo(img);
		const int w = screen_w;
		const int h = screen_h;
		const int b = 0;
		const int s = marker_size;
		img_markers[0].copyTo(img(Rect(b, b, s, s)));
		img_markers[1].copyTo(img(Rect(w - s - b, b, s, s)));
		img_markers[2].copyTo(img(Rect(w - s - b, h - s - b, s, s)));
		img_markers[3].copyTo(img(Rect(b, h - s - b, s, s)));

		// all streamed  gaze coordinates are normalized to the range [0..1]
		// * this has the advantage that the tracker doesnt need to know the distance between the markers in the client
		// * the client doesnt need to know the image resolution of the webcams

		// calc offset and scaling 
		int dw = w - 2 * s - b;
		int dh = h - 2 * s - b;
		int offset = s + b;

		// there *might* be a higher update rate for the gaze data than the screen refresh data. 
		// draw a circle for each gaze point that was received until now 
		//(currently: only 30 hz from eye cam, so there should be only one sample in data or data should be empty)
		for (auto& v : data)
		{
			double x_01 = v[LT_SCREEN_X_FILTERED];
			double y_01 = v[LT_SCREEN_Y_FILTERED];

			if (!std::isnan(x_01) && !std::isnan(y_01))
			{
				x = float(offset + dw * x_01); // todo: proper down-conversion of double to float
				y = float(offset + dh * y_01);
				circle(img, Point2f(x, y), 8, Scalar(255, 0, 255), 4);
				cout << "\n(x,y)=" << x << " " << y;
			}
		}
		// also draw the last gaze point if no new data arrived
		circle(img, Point2f(x, y), 8, Scalar(255, 0, 255), 4);

		/////// optional: visualize tracked marker /////////////
		inlet_marker.pull_sample(marker_data);
		int n_visible = draw_markers(img, 0.5 * w - 0.5 * 640, h - 480, 640, 480, marker_data, data);

		// process events
		imshow("screen", img);
		cv::waitKey(1000 / 60); // render with approx 60 fps 
	}

	return EXIT_SUCCESS;
}


// optional code
int draw_markers(cv::Mat& img, int x, int y, int w, int h, const std::vector<double>& marker_data, const std::vector<std::vector<double>>& eye_data)
{
	using namespace std;
	using namespace cv;
	if (marker_data.size() != 1 + 2 * 4) { return 0; } // cerr << "\nwrong marker data."; 

	array<Point2f, 4> markers;
	int n_visible_markers = 0;
	for (int i = 0; i < 4; i++)
	{
		double mx = x + w * marker_data[1 + 2 * i + 0];
		double my = y + h * marker_data[1 + 2 * i + 1];
		markers[i] = Point2f(mx, my);
		//if (!isnan(mx) && !isnan(my))
		if(!std::isnan(mx) && !std::isnan(my))
		{
			rectangle(img, Rect(mx - 5, my - 5, 10, 10), Scalar(255, 255, 0));
			n_visible_markers++;
		}
	}

	string n_visible_str = to_string(n_visible_markers);
	cv::putText(img, "visible markers = " + n_visible_str, Point2i(x + 0.5 * w - 150, y + h - 240), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);


	// image space rectangle
	if (n_visible_markers == 4)
	{
		for (int i = 0; i < 4; i++)
		{
			line(img, markers[i], markers[(i + 1) % 4], Scalar(0, 155, 255));
		}
	}

	// draw gaze point in scene cam coordinates
	if (eye_data.size() > 0)
	{
		auto& v = eye_data.back();
		circle(img, Point2f(x + w * v[LT_GAZE_X], y + h * v[LT_GAZE_Y]), 5, Scalar(100, 100, 100), 1);
	}


	// border
	rectangle(img, Rect(x, y, w, h), Scalar(125, 125, 125));

	return n_visible_markers;
}

// Sdl_opencv sdl; // vsynced alternative to opencv imshow

/* // code for vertically synchronized rendering using libSDL
// render part
img_screen_background.copyTo(img_screen);
// draw to screen (vsynced flip)
sdl.imshow(img_screen,100,100);
if (sdl.waitKey().sym == SDLK_ESCAPE) { break; }
*/
