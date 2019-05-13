#pragma once

#include <Eigen/LU> 
#include <Eigen/SVD>
#include <Eigen/QR>

#define EYETRACKING_SPELLER_DEMO

#include "aruco_canvas.h"
#include "speller_canvas.h"
#include "pupil_tracking.h"
#include "deps/dependencies.h"

#include "deps/s/opencv_threaded_capture.h"

#include <atomic>

class Eyetracking_speller : public Pupil_tracking
{
protected:

	//const unsigned int w = 1024;
	//const unsigned int h = 768;
	unsigned int w = 1280;
	unsigned int h = 1024;
	unsigned int w_old = 0, h_old = 0;
	double gui_param_w = w, gui_param_h = h;
	double gui_param_marker_size = 100, gui_param_marker_threshold = 30;

	cv::Mat img_screen, img_screen_background;
	cv::Mat frame_scene_cam, frame_scene_cam_scaled;
	cv::Mat frame_eye_gray, frame_eye_cam;

	
	Threaded_capture thread_eyecam;
	Threaded_capture thread_scenecam;
	

	options_type opt;

	Aruco_canvas ar_canvas;
	Speller_canvas speller;

	//shared_ptr<cv::VideoCapture> eye_camera, scene_camera;
	shared_ptr<Camera> eye_camera, scene_camera;


	// "mouse" values
	int mx = 0, my = 0;
	bool eye_button_up = false;
	bool eye_button_down = false;

	int calibration_counter = 0;
	int validation_counter = 0;
	int tracking_lost_counter = 0;
	
	// the offset that was measured during validation
	cv::Point2f offset_validation{0.0f, 0.0f};
	// the offset that will be used. this gives the user the option to set offset = offset_validation or leave it as calibrated
	cv::Point2f offset{ 0.0f, 0.0f };

	//Timer timer{50};

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
	} state = STATE_INSTRUCTIONS;

	// GUI
	Simple_gui sg;
	Camera_control eye_cam_controls;
	Camera_control scene_cam_controls;

	////////////////////////////
	// for calibration
	Eigen::MatrixXd validation_points, calibration_points, calibration_targets, W_calib;

	// for jitter filter
	double filter_smoothing  = 0.25;
	double filter_predictive = 0.075;
	Filter_double_exponential<double> gaze_filter_x;
	Filter_double_exponential<double> gaze_filter_y;

	// draw a scaled copy of the scene cam image to the main screen image.
	// if x or y = -1 then the image is centered along the x or y axis
	void draw_scene_cam_to_screen(float scaling, int x = -1, int y = -1);

	// special function for hybrid eyetracking+ssvep speller
	void run_ssvep();

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

	void run(enum_simd_variant simd_width, int eye_cam_id = -1, int scenen_cam_id=-1);
	void setup(enum_simd_variant simd_width);
	void update();

	void draw_instructions();
	void draw_validation();
	void draw_calibration();
	void draw_calibration_vis()
	{
		// TODO !
	}

	void draw_speller(bool ssvep=false);
	void draw();

	void set_mouse(int x, int y, bool eye_button_up_, bool eye_button_down_) { mx = x; my = y; eye_button_up = eye_button_up_; eye_button_down = eye_button_down_; }
};
