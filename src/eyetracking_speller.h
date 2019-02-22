#pragma once

#include <Eigen/LU> 
#include <Eigen/SVD>
#include <Eigen/QR>

#define EYETRACKING_SPELLER_DEMO

#include "aruco_canvas.h"
#include "speller_canvas.h"
#include "pupil_tracking.h"
#include "deps/dependencies.h"

//#include "../../webcam_to_screen_mapping_perspective_corrected/aruco_canvas.h"
//#include "../../webcam_to_screen_mapping_perspective_corrected/speller_canvas.h"

class Eyetracking_speller : public Pupil_tracking
{
protected:

	//const unsigned int w = 1024;
	//const unsigned int h = 768;
	unsigned int w = 1280;
	unsigned int h = 1024;
	unsigned int w_old = 0, h_old = 0;
	double gui_param_w = w, gui_param_h = h;

	cv::Mat img_screen_background;
	cv::Mat img_screen, frame_scene_cam, frame_eye_cam, frame_eye_gray;

	options_type opt;

	Aruco_canvas ar_canvas;
	Speller_canvas speller;

	shared_ptr<cv::VideoCapture> eye_camera, scene_camera;

	// "mouse" values
	int mx = 0, my = 0;
	bool eye_button_up = false;
	bool eye_button_down = false;

	int calibration_counter = 0;
	int validation_counter = 0;
	Timer timer{50};

	int key_pressed = -1;

	cv::Point pupil_pos, pupil_pos_coarse;

	cv::Point2f p_calibrated; // gaze point in scene cam coordinates (after calibration)
	cv::Point2f p_projected; // calibrated gaze point after inverse projection from 
	

	enum enum_state
	{
		STATE_INSTRUCTIONS,
		STATE_CALIBRATION_SCENE_CAM,
		STATE_CALIBRATION_EYE_CAM,
		STATE_CALIBRATION,
		STATE_CALIBRATION_VISUALIZE,
		STATE_VALIDATION,
		STATE_RUNNING
	} state;

	// GUI
	#ifdef USE_FLTK_GUI
	Simple_gui sg;
	#endif

	////////////////////////////
	// for calibration
	Eigen::MatrixXd validation_points, calibration_points, calibration_targets, W_calib;

	// for jitter filter
	double filter_smoothing  = 0.25;
	double filter_predictive = 0.075;
	Filter_double_exponential<double> gaze_filter_x;
	Filter_double_exponential<double> gaze_filter_y;

public:
	
	// best suited for 4-point calibration
	auto polynomial_features(double x, double y) { Eigen::Matrix<double, 4, 1> v; v << 1.0f, x, y, x*y; return v; };

	// best suited for 3-point calibration
	// auto polynomial_features(float x, float y) { Eigen::Matrix<double, 3, 1> v; v << 1.0f, x, y; return v; };

	void calibrate();

	cv::Point2f mapping_2d_to_2d(cv::Point2f p)
	{
		Eigen::Vector2d tmp = W_calib * polynomial_features(p.x, p.y);
		return cv::Point2f(tmp(0), tmp(1));
	}

public:	

	~Eyetracking_speller()
	{
		cv::destroyAllWindows();
	}

	void setup();

	void draw_validation();

	void draw_calibration();

	void draw_calibration_vis()
	{
	}


	void draw_running();

	void draw();

	void update();

	void run();

};
