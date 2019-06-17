#pragma once

#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>


#include "deps/dependencies.h"

#include "deps/aruco/aruco.h"
#include "deps/aruco/cvdrawingutils.h"


// implements a drawing canvas / flat screen that is tracked using aruco markers
class Aruco_canvas
{
protected:
	std::array<cv::Mat, 4> img_markers_orig;
	int marker_size_old = 100;
	float min_marker_size_old = 0.0f;
	std::vector< aruco::Marker > markers;

	// ok == 4 if all four markers are detected.
	int n_visible_markers = 0;


	aruco::CameraParameters CamParam;
	aruco::MarkerDetector MDetector;
	std::array<cv::Mat, 4> img_markers;

public:
	// active area in screen coordinates
	typedef std::array<cv::Point2f, 4> plane_type;


	// store the Marker corners relevant for defining the screen plane
	plane_type screen_plane;

	// stores the points of the detected rectangle / image plane
	plane_type image_plane;

	// use this plane definition with corners (0,0) - (1,1) 
	// if an external application renders the AR Markers or of you use 
	// "physical" / printed markers
	const plane_type screen_plane_external = 
	{
		cv::Point2f(0,0),
		cv::Point2f(1,0),
		cv::Point2f(1,1),
		cv::Point2f(0,1)
	};


	int marker_size = 100;
	int marker_border = 25;
	double min_marker_size = 0.02f; // In order to be general and to adapt to any image size, the minimum marker size is expressed as a normalized value(0, 1) indicating the minimum area that a marker must occupy in the image to consider it valid.


	bool valid() { return n_visible_markers == 4; }


	void setup();


	void draw(cv::Mat& img, const int x, const int y, const int w, const int h);

	void draw_detected_markers(cv::Mat& img);

	void update(cv::Mat& img_cam);

	// assumes all markers are visible, hence p1..p4 are well defined
	// check with valid() before calling transform
	cv::Point2f transform(cv::Point2f gaze_point)
	{
		return transform(gaze_point, screen_plane);
	}

	// assumes all markers are visible, hence p1..p4 are well defined
	// check with valid() before calling transform
	cv::Point2f transform(cv::Point2f gaze_point, const plane_type& target_plane)
	{
		using namespace cv;
		using namespace std;
		// try to calc screen coordinates from gaze point
		auto H = calc_perspective_matrix(image_plane, target_plane);
		return perspective_transform(H, gaze_point);
	}


	protected:

		// https://www.researchgate.net/profile/Christopher_R_Wren/publication/215439543_Perspective_Transform_Estimation/links/56df558708ae9b93f79a948e.pdf
		// see PerspectiveTransformEstimation.pdf
		// array of points on:
		// ip = Image plane
		// wp = world plane
		Eigen::Matrix<float, 3, 3> calc_perspective_matrix(const std::array<cv::Point2f, 4> & ip, const std::array<cv::Point2f, 4> & wp);



		inline cv::Point2f perspective_transform(Eigen::Matrix<float, 3, 3> H, const cv::Point2f& p)
		{
			using namespace cv;
			using namespace Eigen;

			Vector3f v(p.x, p.y, 1);

			Vector3f C(H(2, 0), H(2, 1), 1);
			H(2, 0) = 0;
			H(2, 1) = 0;
			H(2, 2) = 1;
			auto t = (H * v) / C.dot(v);
			return Point2f(t[0], t[1]);
		}

};