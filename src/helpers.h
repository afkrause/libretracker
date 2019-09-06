#pragma once


#include <string>
#include <array>

#include <opencv2/opencv.hpp>

#include "deps/s/cv_camera_control.h"


constexpr auto nan() { return std::numeric_limits<double>::quiet_NaN(); }


// given input value x, set the output value to the closest value to x found in allowed_values
template<size_t s> float to_closest(float x, const std::array<float, s>& allowed_values)
{
	float tmp = FLT_MAX;
	float y = 0;

	for (auto v : allowed_values)
	{
		if (abs(x - v) < tmp)
		{
			tmp = abs(x - v);
			y = v;
		}
	}

	return y;
}

// clip value x to range min..max
template<class T> inline T clip(T x, const T& min, const T& max)
{
	if (x < min)x = min;
	if (x > max)x = max;
	return x;
}

// produce a random number between [a,b] , a and b inclusive
inline float rnd(float a, float b) { return a + (b - a)*(rand() / float(RAND_MAX)); }


template<size_t n> std::string add_leading_zeros(std::string s)
{
	auto to_add = clip<size_t>(n - s.length(), 0, n); for (size_t a = 0; a < to_add; a++) { s = "0" + s; }; return s;
}

// toggle a boolean
inline void toggle(bool& b)
{
	if (b) { b = false; }
	else { b = true; }
}



// Finds the intersection of two lines, or returns false.
// The lines are defined by (o1, p1) and (o2, p2).
// from: https://answers.opencv.org/question/9511/how-to-find-the-intersection-point-of-two-lines/
inline bool line_intersection(cv::Point2f o1, cv::Point2f p1, cv::Point2f o2, cv::Point2f p2, cv::Point2f& r)
{
	using namespace cv;
	Point2f x = o2 - o1;
	Point2f d1 = p1 - o1;
	Point2f d2 = p2 - o2;

	float cross = d1.x * d2.y - d1.y * d2.x;
	if (abs(cross) < /*EPS*/1e-8)
		return false;

	double t1 = (x.x * d2.y - x.y * d2.x) / cross;
	r = o1 + d1 * t1;
	return true;
}

// ####################################

// draw a scaled copy of the one image (e.g. scene cam) to the a larger image (e.g. main canvas).
// if x or y = -1 then the image is centered along the x or y axis
void draw_preview(cv::Mat& img_preview, cv::Mat& img_target, float scaling = 1.0f, int x = -1, int y = -1);


// generic opencv camera selection dialog
// if provided an camera id != -1, it tries to open this id. 
// if that fails, the selection dialog is presented (win32: including a list of available cameras)
std::shared_ptr<Camera> select_camera(std::string message = "select video camera nr. (default=0):", int id = -1);


// helper function. when pressing a button in the fltk gui, 
// calling this function grabs the focus of the specified opencv window
void grab_focus(const char* wname);