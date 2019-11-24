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
	int marker_size_old = 120;
	
	
	// the relevant markers in this frame that have the proper ID for the canvas
	std::array< aruco::Marker, 4> markers;

	// ok == 4 if all four markers are detected.
	int n_visible_markers = 0;


	aruco::CameraParameters CamParam;
	aruco::MarkerDetector MDetector;
	std::array<cv::Mat, 4> img_markers;

	// helpers for estimating the position of temporatily invisible / untrackable markers
	/*
	class Marker_estimation_features
	{
	public:
		cv::Point2f offset; // the offset between two Markers (average over the difference between the 4 corresponding Corner Points)
		cv::Vec2f direction_vector;
		float scaling;
	};
	std::array< std::array<Marker_estimation_features, 4>, 4> marker_features;
	*/
	std::array< std::array<cv::Point2f, 4>, 4> mutual_marker_offsets;

	void calc_canvas_plane_from_markers();

	// track the canvas using all four markers, no marker prediction.
	

	// track the canvas and estimate the position of invisible / currently not trackable markers using a simple prediction:
	// calculate the average over all marker offsets ( offsets = vectors from the non-trackable marker to the othe, still trackable markers)
	void predict_markers_using_mutual_offsets();

	// use the marker edges that point to the neighbouring markers and calculate the scaling that is required to let this edge vector point to the neighbouring marker
	// this is better than the simple offset predictor, because it is mostly invariant to head rotations and distance changes,  
	// but is prone to jitter if markers are too small. hence, here, larger markers should be used.
	std::array< std::array<cv::Point2f, 4>, 4> edge_vec;
	std::array< std::array<float, 4>, 4> edge_scale;
	void predict_markers_using_edge_vectors();

public:

	enum enum_prediction_method
	{
		NO_MARKER_PREDICTION,
		MARKER_PREDICTION_MUTUAL_OFFSETS,
		MARKER_PREDICTION_EDGE_VECTORS
	} prediction_method = MARKER_PREDICTION_EDGE_VECTORS;


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

	int marker_size = 150;

	bool valid() { return n_visible_markers == 4; }


	void setup(bool use_enclosed_markers = false);

	// void set_detection_size(float minimum_procentual_marker_size);

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