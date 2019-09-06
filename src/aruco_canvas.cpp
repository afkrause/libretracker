#include "aruco_canvas.h"

#include <limits>

void Aruco_canvas::setup(bool use_enclosed_markers)
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
	array<string, 4> marker_file_names;
	if (use_enclosed_markers)
	{
		marker_file_names = array<string, 4>{ "aruco_00100_e.png", "aruco_00301_e.png", "aruco_00450_e.png", "aruco_00700_e.png" };
	}
	else
	{
		//marker_file_names = array<string, 4>{ "aruco_00100.png", "aruco_00301.png", "aruco_00450.png", "aruco_00700.png" };

		// according to the aruco main developer, ARUCO_MIP_36h12 is the preferred dictionary. 
		// https://stackoverflow.com/questions/50076117/what-are-the-advantages-disadvantages-between-the-different-predefined-aruco-d
		marker_file_names = array<string, 4>{ "aruco_mip_36h12_00002.png", "aruco_mip_36h12_00004.png", "aruco_mip_36h12_00006.png", "aruco_mip_36h12_00008.png" };
	}

	for (size_t i = 0; i < marker_file_names.size(); i++)
	{
		img_markers_orig[i] = imread("assets/" + marker_file_names[i]);
		resize(img_markers_orig[i], img_markers[i], Size(marker_size, marker_size));
	}

	for (auto& p : image_plane) { p = Point2f(NAN, NAN); }

	// set_detection_size(0.008f);
	using namespace aruco;
	auto p = MarkerDetector::Params();
	
	//p.setDetectionMode(DM_NORMAL, 0.005f);// min_marker_size); // NORMAL mode is slow and it does not work with enclosed markers (bug?)
	//p.setDetectionMode(DM_VIDEO_FAST, 0); // this mode sometimes seems to be a bit unreliable. but might better for raspberry pi (higher speed)
	p.setDetectionMode(DM_FAST, 0.0f); // best option currently
	p.setCornerRefinementMethod(CORNER_SUBPIX);
	if (use_enclosed_markers) { p.detectEnclosedMarkers(true); } // with enclosed markers, corners jitter less, BUT markers need to be much larger to get detected..
	MDetector.setParameters(p);
	MDetector.setDictionary("ARUCO_MIP_36h12"); // according to the aruco main developer, ARUCO_MIP_36h12 is the preferred dictionary. 

}


void Aruco_canvas::draw(cv::Mat& img, const int x, const int y, const int w, const int h)
{
	using namespace cv;

	const unsigned int b = 0; // marker border to leave between screen border and marker
	const unsigned int s = marker_size;
	img_markers[0].copyTo(img(Rect(b, b, s, s)));
	img_markers[1].copyTo(img(Rect(w - s - b, b, s, s)));
	img_markers[2].copyTo(img(Rect(w - s - b, h - s - b, s, s)));
	img_markers[3].copyTo(img(Rect(b, h - s - b, s, s)));


	// draw the active area
	const int n_subdiv = 10;
	auto mb = marker_size ;
	float area_w = w - 2 * mb;
	float area_h = h - 2 * mb;

	// update the screen plane
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

void Aruco_canvas::draw_detected_markers(cv::Mat& img)
{
	using namespace cv;

	// for each marker, draw info and its boundaries in the image
	for (unsigned int i = 0; i < markers.size(); i++)
	{
		if (markers[i].size() == 4)
		{
			//cout << Markers[i] << endl;
			markers[i].draw(img, Scalar(0, 0, 255), 2);
		}
	}

	// draw yellow frame if all 4 markers are visible
	if (n_visible_markers == 4)
	{
		auto& p = image_plane;
		auto col = Scalar(0, 255, 255);
		cv::line(img, p[0], p[1], col);
		cv::line(img, p[1], p[2], col);
		cv::line(img, p[2], p[3], col);
		cv::line(img, p[3], p[0], col);
	}

}

/*
void Aruco_canvas::set_detection_size(float minimum_procentual_marker_size)
{
	using namespace aruco;
	auto p = MarkerDetector::Params();
	p.setDetectionMode(DM_VIDEO_FAST, minimum_procentual_marker_size); // this mode sometimes seems to be a bit unreliable. but might better for raspberry pi (higher speed)
	//p.setDetectionMode(DM_FAST, min_marker_size);
	MDetector.setParameters(p);
}
*/

void Aruco_canvas::update(cv::Mat& img_cam)
{
	using namespace cv;
	using namespace aruco;
	using namespace std;

	// if the marker size externally changed, resize accordingly
	if (marker_size != marker_size_old)
	{
		for (size_t i = 0; i < img_markers_orig.size(); i++)
		{
			resize(img_markers_orig[i], img_markers[i], Size(marker_size, marker_size));
		}
		marker_size_old = marker_size;
	}

	/*
	if (min_marker_size != min_marker_size_old)
	{
		min_marker_size_old = min_marker_size;
	}
	*/

	// detect aruco markers
	// Ok, let's detect
	markers = MDetector.detect(img_cam, CamParam, -1);



	// check if all markers are visible 
	n_visible_markers = 0;
	for (auto& p : image_plane) { p = Point2f(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()); }
	for (auto& m : markers)
	{
		if (m.isValid() && m.size() == 4)
		{
			/*
			if (m.id == 100) { image_plane[0] = m[2]; n_visible_markers++; }
			if (m.id == 301) { image_plane[1] = m[3]; n_visible_markers++; }
			if (m.id == 450) { image_plane[2] = m[0]; n_visible_markers++; }
			if (m.id == 700) { image_plane[3] = m[1]; n_visible_markers++; }
			//*/
			//*
			if (m.id == 2) { image_plane[0] = m[2]; n_visible_markers++; }
			if (m.id == 4) { image_plane[1] = m[3]; n_visible_markers++; }
			if (m.id == 6) { image_plane[2] = m[0]; n_visible_markers++; }
			if (m.id == 8) { image_plane[3] = m[1]; n_visible_markers++; }
			//*/
		}
	}
}


Eigen::Matrix<float, 3, 3> Aruco_canvas::calc_perspective_matrix(const std::array<cv::Point2f, 4> & ip, const std::array<cv::Point2f, 4> & wp)
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


// OLD CODE not required anymore, but maybe useful later

/*

// length of the vector
inline float norm(cv::Point2f p)
{
	return sqrt(p.dot(p));
}


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

*/

