#pragma once

#include <Eigen/LU> 
#include <Eigen/SVD>
#include <Eigen/QR>

#include "aruco_canvas.h"
#include "speller_canvas.h"
//#include "deps/dependencies.h"


class Calibration_base
{
protected:

	// best suited for 3-point calibration
	// auto polynomial_features(float x, float y) { Eigen::Matrix<double, 3, 1> v; v << 1.0f, x, y; return v; };

	// best suited for 4-point calibration
	auto polynomial_features(double x, double y) { Eigen::Matrix<double, 10, 1> v; v << 1.0f, x, y, x* y, x* x, y* y, x* x* x, y* y* y, x* y* y, x* x* y; return v; };
	double mapping_error = 0;
	int n_polynomial_features = 4;
public:
	void calibrate(int n_polynomial_features=4);
	Eigen::MatrixXd validation_points, calibration_points, calibration_targets, W_calib;

	cv::Point2f mapping_2d_to_2d(cv::Point2f p)
	{
		Eigen::Vector2d tmp = W_calib * polynomial_features(p.x, p.y).block(0, 0, n_polynomial_features, 1);
		return cv::Point2f(tmp(0), tmp(1));
	}

	void setup(int n_calibration_points);

};

class Calibration : public Calibration_base
{
protected:

	const int marker_size = 120;

	void update_calibration(cv::Mat& frame_scene_cam, cv::Point2f pupil_pos, int key_pressed);

	void draw_prep(cv::Mat& frame_scene_cam, cv::Mat& img_screen);
	void draw_calibration(cv::Mat& frame_scene_cam, cv::Mat& img_screen);
	void draw_validation(cv::Mat& frame_scene_cam, cv::Mat& img_screen);
	void draw_visualization(cv::Mat& frame_scene_cam, cv::Mat& img_screen);
	void draw_observe(cv::Mat& frame_scene_cam, cv::Mat& img_screen);
	int n_calib_points = 4;
	cv::Point2f p_calibrated, p_projected;

	std::vector<cv::Point2f> targets;

	// special aruco dictionary and marker for calibration purposes, only
	aruco::MarkerDetector marker_detector_calib;
	std::vector<aruco::Marker>  markers_calib;
	cv::Point2f marker_calib_center;
	cv::Mat img_marker_calib, img_marker_calib_scaled;
	float marker_calib_anim_fx = 0; // just a helper variable for a neat marker animation effect

	int calibration_counter = 0;
	int validation_counter = 0;
	int tracking_lost_counter = 0;

	// the offset that was measured during validation
	cv::Point2f offset_validation{ 0.0f, 0.0f };

	int n_polynomial_features = 4;
public:
	Calibration();

	Aruco_canvas ar_canvas;

	enum enum_calib_state
	{
		STATE_PREPARE,
		STATE_CALIBRATION,
		STATE_VISUALIZE_CALIBRATION,
		STATE_VISUALIZE_VALIDATION,
		STATE_OBSERVE,
		STATE_VALIDATION
	} state = STATE_PREPARE;


	// TODO
	// the offset that will be used. this gives the user the option to set offset = offset_validation or leave it as calibrated
	cv::Point2f offset{ 0.0f, 0.0f };
	
	void fix_offset()
	{
		offset = offset_validation;
	}


	void draw(cv::Mat& frame_scene_cam, cv::Mat& img_screen);

	void setup(int n_calibration_points=5);
	void setup_validation(int n_validation_points=5);

	void update(cv::Mat& frame_scene_cam, cv::Point2f pupil_pos, int key_pressed);

	void set_number_of_polynomial_features(int n) { n_polynomial_features = n; }
};