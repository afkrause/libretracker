#pragma once

#include "deps/dependencies.h"
#ifdef _WIN32
#include "deps/DeviceEnumerator.h"
#endif

#include "deps/timms_algorithm/src/timm_two_stage.h"
#include "deps/s/cv_camera_control.h"
#include <atomic>


enum enum_pupil_tracking_variant
{
	PUPIL_TRACKING_TIMM,
	PUPIL_TRACKING_PURE,
	PUPIL_TRACKING_PUREST
};

/*
class Pupil
{
public:
	cv::Point2f center{ nan(), nan() };
	cv::RotatedRect ellipse{ cv::Point2f{nan(), nan()}, cv::Size2f{nan(), nan()}, nan() };
};
*/

// todo - use class Pupil 
class Pupil_tracker_base
{
public:
	virtual void setup(enum_simd_variant simd_width) = 0;
	virtual void update(cv::Mat& eye_cam_frame) = 0;	
	virtual cv::Point2f pupil_center() = 0;
	
	virtual void draw(cv::Mat& img) { }
	virtual void show_gui() {}
};


#include "deps/tuebingen_pure/PuRe.h"
#include "deps/tuebingen_pure/pupil-tracking/PuReST.h"
class Pupil_tracker_pure : public Pupil_tracker_base
{
protected: 
	PuRe pupil_detector;
	Pupil pupil;

public:
	cv::Mat frame_gray;

public:
	virtual void setup(enum_simd_variant simd_width)
	{

	}

	virtual void update(cv::Mat& eye_cam_frame)
	{
		cv::cvtColor(eye_cam_frame, frame_gray, cv::COLOR_BGR2GRAY);
		pupil = pupil_detector.run(frame_gray);
	}

	virtual void draw(cv::Mat& img)
	{
		cv::circle(img, pupil.center, 4, cv::Scalar(255, 0, 255), 2);

		if (pupil.size.width > 0 && pupil.size.height > 0)
		{
			cv::ellipse(img, pupil, cv::Scalar(0, 255, 0), 1);
		}
	}


	virtual cv::Point2f pupil_center()
	{
		return pupil.center;
	}

};

class Pupil_tracker_purest : public Pupil_tracker_base
{
protected:
	PuRe pupil_detect;
	std::unique_ptr<PupilTrackingMethod> pupil_tracking;
	Pupil pupil;
	int frame_counter = 0;
public:
	cv::Mat frame_gray;

public:
	virtual void setup(enum_simd_variant simd_width)
	{
		pupil_tracking = std::make_unique<PuReST>();
		frame_counter = 0;
	}

	virtual void update(cv::Mat& eye_cam_frame)
	{
		// for now, roi is as large as whole eye cam frame
		// TODO: user selectabel roi
		cv::Rect roi(0, 0, eye_cam_frame.cols, eye_cam_frame.rows);
		cv::cvtColor(eye_cam_frame, frame_gray, cv::COLOR_BGR2GRAY);
		pupil_tracking->run(frame_counter, frame_gray, roi, pupil, pupil_detect);
		frame_counter++;
	}

	virtual void draw(cv::Mat& img)
	{
		cv::circle(img, pupil.center, 4, cv::Scalar(255, 0, 255), 2);
		if(pupil.size.width > 0 && pupil.size.height > 0)
		{
			cv::ellipse(img, pupil, cv::Scalar(0, 255, 0), 1);
		}
	}


	virtual cv::Point2f pupil_center()
	{
		return pupil.center;
	}

};

class Pupil_tracker_timm : public Pupil_tracker_base
{

public:
	Timm_two_stage timm; // todo.. move to protected 
//protected: 
	cv::Mat frame_gray;
	

	enum enum_parameter_settings
	{
		SETTINGS_DEFAULT,
		SETTINGS_LPW,
		SETTINGS_FUTURE1,
		SETTINGS_FUTURE2,
		SETTINGS_FUTURE3,
		SETTINGS_FROM_FILE,
	};

	using options_type = typename Timm_two_stage::options;
	using params_type = std::array<double, 11>;


	// allowed sizes for the kernels
	const std::array<float, 5> sobel_kernel_sizes{ -1, 1, 3, 5, 7 };
	const std::array<float, 16> blur_kernel_sizes{ 0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29 };

	// helper functions for the fltk gui
	params_type  set_params(options_type opt);
	options_type set_options(params_type params);


	cv::Point pupil_pos, pupil_pos_coarse;

	options_type opt;
	std::array<double, 11> params;
	std::array<bool, 4> debug_toggles{ false,false, false,false };
	std::array<bool, 4> debug_toggles_old;
	bool do_init_windows = true;

	
	double n_threads = 1;

	Simple_gui sg;
	void setup_gui();
public: 

	virtual void setup(enum_simd_variant simd_width);
	virtual void update(cv::Mat& eye_cam_frame);
	
	virtual void draw(cv::Mat& img);
	
	virtual void show_gui() { sg.show(); }

	virtual cv::Point2f pupil_center()
	{
		return pupil_pos;
	}

	// special to timms algorithm
	options_type decode_genom(Eigen::VectorXf params);
	options_type load_parameters(enum_parameter_settings s);

};

class Pupil_tracking
{
protected:
	
	std::shared_ptr<Pupil_tracker_base> pupil_tracker;

	std::atomic<bool> is_running; // must be atomic because it is later used to exit the asynchronous capture thread
	
public:

	// TODO: proper encapsulation
	std::shared_ptr<Camera> eye_camera;
	cv::Mat frame_eye_cam;
	std::vector<cv::VideoCapture> cameras; // list of active cameras. required for cv::VideoCapture:waitAny
	std::vector<int> camera_ready{ 0, 0 };


	void setup(enum_simd_variant simd_width, enum_pupil_tracking_variant pupil_tracking_variant);

	void update()
	{
		// TODO !
	}


	// capture from the usb webcam 
	//void run(enum_simd_variant simd_width, int eye_cam_id = -1, cv::VideoCaptureAPIs eye_cam_backend=cv::CAP_ANY);
	void run(enum_simd_variant simd_width, std::shared_ptr<Camera> eye_camera);
	


};

class Pupil_tracker_timm_tests : public Pupil_tracker_timm
{

protected:

public:

	std::vector<std::array<float, 4>> timings_vector;

	// fitness function
	Eigen::VectorXf eval_fitness(Eigen::VectorXf params, const std::vector<int>& idx, const int n, const std::vector<cv::Mat>& images_all, const std::vector<cv::Point2f>& ground_truth, bool visualize, const int subsampling_width = 100);

	// evaluate the best parameter set over ALL images of the EXCUSE and ELSE dataset
	// run tests on different datasets
	void run_lpw_test_all(enum_simd_variant simd_width);
	void run_swirski_test(enum_simd_variant simd_width);
	void run_excuse_test(enum_simd_variant simd_width);
	void run_differential_evolution_optim(enum_simd_variant simd_width);

};