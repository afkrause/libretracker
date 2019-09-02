#pragma once
#include <atomic>

#include <Eigen/LU> 
#include <Eigen/SVD>
#include <Eigen/QR>

#include "aruco_canvas.h"
#include "speller_canvas.h"
#include "pupil_tracking.h"
#include "deps/dependencies.h"
#include "deps/s/opencv_threaded_capture.h"
#include "calibration.h"



class Eyetracking_speller : public Pupil_tracking
{
protected:

	// these canvas window sizes should work out-of-the-box for windows 10 and a full-HD display
	unsigned int w = 1280;
	unsigned int h = 1000;
	unsigned int w_old = 0, h_old = 0;
	double gui_param_w = w, gui_param_h = h;
	double gui_param_marker_size = 150;

	cv::Mat img_screen, img_screen_background;
	cv::Mat frame_scene_cam, frame_scene_cam_scaled;
	

	
	Threaded_capture thread_eyecam;
	Threaded_capture thread_scenecam;
	

	//options_type opt;

	
	Speller_canvas speller;

	//shared_ptr<cv::VideoCapture> eye_camera, scene_camera;
	std::shared_ptr<Camera> scene_camera;



	// "mouse" values
	int mx = 0, my = 0;
	bool eye_button_up = false;
	bool eye_button_down = false;

	int calibration_counter = 0;
	int validation_counter = 0;
	int tracking_lost_counter = 150;
	

	//Timer timer{50};

	int key_pressed = -1;

	cv::Point2f pupil_pos;// , pupil_pos_coarse;
	cv::Point2f p_calibrated; // gaze point in scene cam coordinates (after calibration)
	cv::Point2f p_projected; // calibrated gaze point after inverse projection from 
	

	enum enum_state
	{
		STATE_INSTRUCTIONS,
		STATE_CALIBRATION_SCENE_CAM,
		STATE_CALIBRATION_EYE_CAM,
		STATE_CALIBRATION,
		STATE_RUNNING
	} state = STATE_INSTRUCTIONS;

	// GUI
	Simple_gui sg;
	Camera_control eye_cam_controls;
	Camera_control scene_cam_controls;

	////////////////////////////
	// for calibration
	Calibration calibration;

	// for jitter filter
	double filter_smoothing  = 0.25;
	double filter_predictive = 0.075;
	Filter_double_exponential<double> gaze_filter_x;
	Filter_double_exponential<double> gaze_filter_y;



	// special function for hybrid eyetracking+ssvep speller
	void run_ssvep();

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
	
	void draw_calibration_prep();
	void draw_calibration_vis()
	{
		// TODO !
	}


	void draw_speller(bool ssvep=false);
	void draw();

	void set_mouse(int x, int y, bool eye_button_up_, bool eye_button_down_) { mx = x; my = y; eye_button_up = eye_button_up_; eye_button_down = eye_button_down_; }
};
