#pragma once

// here you can choose to include the version of timm.h with opencl support
#include "timm_opencl.h"
//#include "timm.h"

#include "helpers.h"

#include <opencv2/highgui/highgui.hpp>

class Timm_two_stage
{
private:

	cv::Mat frame_gray_windowed;
	cv::Mat frame_color;

public:
	int simd_width = USE_VEC256;
	struct options
	{
		using timm_options = typename Timm::options;
		int blur = 0;
		int window_width = 150;
		timm_options stage1; // coarse pupil center estimation stage
		timm_options stage2; // fine, windowed pupil center estimation stage
	} opt;

	void setup(enum_simd_variant simd_width)
	{
		stage1.setup(simd_width);
		stage2.setup(simd_width);

		#ifdef __TIMM_OPENCL__
		// only try to compile the opencl kernel if we actually use OpenCL.
		if (simd_width == USE_OPENCL)
		{
			gradient_kernel.setup();
		}
		#endif
	}

	#ifdef __TIMM_OPENCL__
	Opencl_kernel gradient_kernel;

	Timm_opencl stage1;
	Timm_opencl stage2;
	#else
	Timm stage1;
	Timm stage2;
	#endif

	Timm_two_stage() 
	#ifdef __TIMM_OPENCL__
	: stage1(gradient_kernel), stage2(gradient_kernel)
	#endif
	{
		stage1.debug_window_name = "Stage 1 (coarse)";
		stage2.debug_window_name = "Stage 2 (fine)";
		
	}

	void set_options(options o)
	{
		opt = o;
		stage1.opt = opt.stage1;
		stage2.opt = opt.stage2;

		// if the window width is smaller than the down scaling width, make the down_scaling_width equal to the window width to save processing time
		stage2.opt.down_scaling_width = min(stage2.opt.down_scaling_width, opt.window_width);
	}

	void set_debug_toggles(array<bool, 4> debug_toggles)
	{
		stage1.debug_toggles = debug_toggles;
		stage2.debug_toggles = debug_toggles;
	}

	array<float, 4> get_timings() { return array<float, 4>{stage1.measure_timings[0], stage1.measure_timings[1], stage2.measure_timings[0], stage2.measure_timings[1]}; }

	// two stages: coarse estimation and local refinement of pupil center
	tuple<cv::Point, cv::Point> pupil_center(cv::Mat& frame_gray)
	{
		//-- Find Eye Centers
		cv::Point pupil_pos_coarse = stage1.pupil_center(frame_gray);
		
		auto rect = fit_rectangle(frame_gray, pupil_pos_coarse, opt.window_width);
		frame_gray_windowed = frame_gray(rect);
		cv::Point pupil_pos = stage2.pupil_center(frame_gray_windowed);
		
		pupil_pos.x += rect.x;
		pupil_pos.y += rect.y;
		return tie(pupil_pos, pupil_pos_coarse);
	}



private:

	///////// visualisation stuff ///////////

	template<class T> void draw_cross(cv::Mat& img, T p, int w, cv::Scalar col = cv::Scalar(255, 255, 255))
	{
		float w2 = 0.5*w;
		cv::line(img, cv::Point(p.x - w2, p.y), cv::Point(p.x + w2, p.y), col);
		cv::line(img, cv::Point(p.x, p.y - w2), cv::Point(p.x, p.y + w2), col);
	}


	// fit a rectangle with center c and half-width w into a given image
	cv::Rect fit_rectangle(cv::Mat frame, cv::Point2f c, int w)
	{
		int w2 = w; // half width of windows
		w2 = clip<int>(w2, 0, 0.5*frame.cols);
		w2 = clip<int>(w2, 0, 0.5*frame.rows);

		int x = c.x; x = clip<float>(x, w2, frame.cols - w2);
		int y = c.y; y = clip<float>(y, w2, frame.rows - w2);
		return cv::Rect(x - w2, y - w2, 2 * w2, 2 * w2);
	}


public:
	void visualize_frame(cv::Mat frame, cv::Point2f pupil_pos, cv::Point2f pupil_pos_coarse, const cv::Point2f* ground_truth_pos = nullptr)
	{
		cv::cvtColor(frame, frame_color, cv::COLOR_GRAY2BGR);
		auto rect = fit_rectangle(frame_color, pupil_pos_coarse, opt.window_width);

		// draw local processing rectangle
		cv::rectangle(frame_color, rect, cv::Scalar(0, 0, 155));
		
		// draw eye center coarse
		//circle(frame_color, pupil_pos_coarse, 2, cv::Scalar(255, 0, 0), 1);
		
		// draw eye center fine
		circle(frame_color, pupil_pos, 5, cv::Scalar(0, 255, 0), 2);

		//if given a ground truth pos, draw this too
		if (ground_truth_pos)
		{
			draw_cross(frame_color, *ground_truth_pos, 7, cv::Scalar(255, 0, 255));
		}

		cv::imshow("Timm's algorithm: two-stage gradient based dark pupil tracking", frame_color);
	}


};
