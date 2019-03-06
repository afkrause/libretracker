#pragma once

#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>


#include "deps/dependencies.h"

#include "deps/aruco/aruco.h"
#include "deps/aruco/cvdrawingutils.h"


// distance of a point to a line in 2D
// http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
inline float dist_point_line(cv::Point2f p, cv::Point2f pl1, cv::Point2f pl2)
{
	using namespace cv;
	// vector that is perpendicular onto the line
	auto v = Point2f(pl2.y - pl1.y, -(pl2.x - pl1.x));
	auto r = pl1 - p;

	const float eps = 0.000001f;

	// if ray to short, stop here.
	float len = sqrt(v.dot(v));
	if (len > eps)
	{
		// normalize ray length
		v = v / len;
	}

	// distance is the dot product between the normalized vector perpendicular to the line and the ray between the point and the line start

	return v.dot(r);
}

// length of the vector 
inline float norm(cv::Point2f p)
{
	return sqrt(p.dot(p));
}

// https://www.researchgate.net/profile/Christopher_R_Wren/publication/215439543_Perspective_Transform_Estimation/links/56df558708ae9b93f79a948e.pdf
// see PerspectiveTransformEstimation.pdf
// array of points on:
// ip = Image plane
// wp = world plane
inline Eigen::Matrix<float, 3, 3> calc_perspective_matrix(const std::array<cv::Point2f, 4>& ip, const std::array<cv::Point2f, 4>& wp)
{
	using namespace Eigen;

	assert(ip.size() == wp.size());

	Matrix<float, 8, 8> M;
	Matrix<float, 2, 8> m;
	Matrix<float, 8, 1> v;

	for (size_t i = 0; i < ip.size(); i++)
	{
		float x = ip[i].x, y = ip[i].y,
			  X = wp[i].x, Y = wp[i].y;

		m << x, y, 1, 0, 0, 0, -X * x, -X * y,
			 0, 0, 0, x, y, 1, -Y * x, -Y * y;

		v[2 * i] = X;
		v[2 * i + 1] = Y;

		M.block(2 * i, 0, 2, 8) = m;
	}
	//cout << "v=\n" << v << endl;
	//cout << "M=\n" << M << endl;

	// using matrix inverse
	Matrix<float, 8, 1> v2 = (M.transpose() * M).inverse() * M.transpose() * v;

	//cout << "v2 =\n" << v2 << endl;

	// TODO: fix pinv
	//MatrixXf M_inv;
	//pinv<float>(M , M_inv);
	//VectorXf v2 = M_inv * v;

	// reshape v2 into camera matrix H
	Matrix<float, 3, 3> H = Map<MatrixXf>(v2.data(), 3, 3);
	H(2, 2) = 1.0f;
	H.transposeInPlace();
	//cout << "H =\n" << H << endl;
	return H;
}

inline cv::Point2f perspective_transform(Eigen::Matrix<float, 3, 3> H, const cv::Point2f& p)
{
	using namespace cv;
	using namespace Eigen;

	Vector3f v(p.x, p.y, 1);

	Vector3f C(H(2, 0), H(2, 1), 1);
	H(2, 0) = 0;
	H(2, 1) = 0;
	H(2, 2) = 1;
	auto t = (H*v) / C.dot(v);
	return Point2f(t[0], t[1]);
}

// implements a drawing canvas / flat screen that is tracked using aruco markers
class Aruco_canvas
{
protected:
	std::array<cv::Mat, 4> img_markers_orig;
	int marker_size_old = 100;
	float min_marker_size_old = 0.0f;
public:
	// active area in screen coordinates
	std::array<cv::Point2f, 4> screen_plane;

	// stores the points of the detected rectangle / image plane
	std::array<cv::Point2f, 4> image_plane;

	// stores the projected point given gaze / mouse coordinates
	cv::Point2f p_projected;

	// ok == 4 if all four markers are detected.
	int ok = 0; 

	aruco::CameraParameters CamParam;
	aruco::MarkerDetector MDetector;
	std::array<cv::Mat, 4> img_markers;

	int marker_size = 100;
	int marker_border = 25;
	double min_marker_size = 0.02f; // In order to be general and to adapt to any image size, the minimum marker size is expressed as a normalized value(0, 1) indicating the minimum area that a marker must occupy in the image to consider it valid.

	// read marker size if specified (default value -1)
	float MarkerSize = -1; // std::stof(cml("-s", "-1"));



	void setup()
	{

		using namespace cv;
		using namespace aruco;
		using namespace std;

		// ********************************************************
		// set up aruco marker detection lib

		// read camera parameters if specifed
		//if (cml["-c"])  CamParam.readFromXMLFile(cml("-c"));


		//Set the dictionary you want to work with, if you included option -d in command line
		//see dictionary.h for all types
		//MDetector.setDictionary(cml("-d"), 0.f);
		

		// load 4 markers
		array<string, 4> marker_file_names{ "marker_1.jpg", "marker_5.jpg", "marker_10.jpg","marker_25.jpg" };

		for (size_t i = 0; i < marker_file_names.size(); i++)
		{
			img_markers_orig[i] = imread("assets/" + marker_file_names[i]);
			resize(img_markers_orig[i], img_markers[i], Size(marker_size, marker_size));
		}
	}


	void draw(cv::Mat& img, const int x, const int y, const int w, const int h)
	{
		using namespace cv;

		const unsigned int b = marker_border; // marker border to leave between screen border and marker
		const unsigned int s = marker_size;
		img_markers[0].copyTo(img(Rect(b, b, s, s)));
		img_markers[1].copyTo(img(Rect(w - s - b, b, s, s)));
		img_markers[2].copyTo(img(Rect(w - s - b, h - s - b, s, s)));
		img_markers[3].copyTo(img(Rect(b, h - s - b, s, s)));


		// draw the active area
		const int n_subdiv = 10;
		auto mb = marker_size + marker_border;
		float area_w = w - 2 * mb;
		float area_h = h - 2 * mb;

		screen_plane[0] = Point2f(mb, mb);
		screen_plane[1] = Point2f(mb + area_w, mb);
		screen_plane[2] = Point2f(mb + area_w, mb + area_h);
		screen_plane[3] = Point2f(mb, mb + area_h);

		// draw grid
		for (int i = 0; i <= n_subdiv; i++)
		{
			cv::line(img, Point2f(mb + area_w * i / float(n_subdiv), mb), Point2f(mb + area_w * i / float(n_subdiv), mb + area_h), Scalar(255, 225, 225));
			cv::line(img, Point2f(mb, mb + area_h * i / float(n_subdiv)), Point2f(mb + area_w, mb + area_h * i / float(n_subdiv)), Scalar(255, 225, 225));
		}
		// cv::rectangle(img_screen, Rect(mb, mb, area_w, area_h), Scalar(155,155,155));



	}

	void update(cv::Mat& img_cam, cv::Point2f gaze_point)
	{
		using namespace cv;
		using namespace aruco;
		using namespace std;

		Point2f p1, p2, p3, p4;
		ok = 0;

		// if the marker size externally changed, resize accordingly
		if (marker_size != marker_size_old)
		{
			for (size_t i = 0; i < img_markers_orig.size(); i++)
			{
				resize(img_markers_orig[i], img_markers[i], Size(marker_size, marker_size));
			}
			marker_size_old = marker_size;
		}

		if (min_marker_size != min_marker_size_old)
		{
			auto p = aruco::MarkerDetector::Params();
			p.setDetectionMode(DM_VIDEO_FAST, min_marker_size);
			MDetector.setParameters(p);
			min_marker_size_old = min_marker_size;
		}


		// detect aruco markers
		// Ok, let's detect
		vector< Marker >  markers = MDetector.detect(img_cam, CamParam, MarkerSize);

		// for each marker, draw info and its boundaries in the image
		for (unsigned int i = 0; i < markers.size(); i++)
		{
			if (markers[i].size() == 4)
			{
				//cout << Markers[i] << endl;
				markers[i].draw(img_cam, Scalar(0, 0, 255), 2);
			}
		}

		// try to calc screen coordinates from gaze point			

		for (auto m : markers)
		{
			if (m.isValid() && m.size() == 4)
			{
				if (m.id == 1) { p1 = m[2]; ok++; }
				if (m.id == 5) { p2 = m[3]; ok++; }
				if (m.id == 10) { p3 = m[0]; ok++; }
				if (m.id == 25) { p4 = m[1]; ok++; }
			}
		}

		if (ok == 4)
		{
			image_plane = array<Point2f, 4>{ p1, p2, p3, p4 };
			auto H = calc_perspective_matrix(image_plane, screen_plane);
			p_projected = perspective_transform(H, gaze_point);

			// yellow frame
			cv::line(img_cam, p1, p2, Scalar(0, 255, 255));
			cv::line(img_cam, p2, p3, Scalar(0, 255, 255));
			cv::line(img_cam, p3, p4, Scalar(0, 255, 255));
			cv::line(img_cam, p4, p1, Scalar(0, 255, 255));

		}

	}

};



/////////////////////////////////////////////////

#ifdef __TEST_THIS_MODULE__

void test_calc_perspective_matrix()
{
	std::array<Point2f, 4> ip;
	std::array<Point2f, 4> wp;

	// a perspective distorted rectangle in image space (getting smaller on the right side)
	ip[0] = Point2f(100, 100);
	ip[1] = Point2f(100, 400);
	ip[2] = Point2f(500, 300);
	ip[3] = Point2f(500, 100);

	// the world plane rectangle
	wp[0] = Point2f(0, 0);
	wp[1] = Point2f(0, 480);
	wp[2] = Point2f(640, 480);
	wp[3] = Point2f(640, 0);

	auto H = calc_perspective_matrix(ip, wp);

	using namespace Eigen;
	// test some points
	auto tmp1 = perspective_transform(H, Point2f(100, 100));
	cout << "should be (0,0): " << tmp1 << endl;
	auto tmp2 = perspective_transform(H, Point2f(100, 400));
	cout << "should be (0,480): " << tmp2 << endl;

	// pause
	char c; cin >> c;
}

#endif
