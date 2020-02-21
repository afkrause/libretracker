#include "calibration.h"

#include "helpers.h"

#include <iostream>

using namespace cv;
using namespace std;

void Calibration_base::setup(int n_calibration_points)
{
	using namespace Eigen;
	int n = n_calibration_points;

	calibration_targets = MatrixXd::Zero(2, n);
	calibration_points = MatrixXd::Zero(2, n);
	validation_points = MatrixXd::Zero(2, 5);
	
	W_calib = MatrixXd::Zero(2, 4); // assuming 4 polynomial features

}

void Calibration_base::calibrate(int n_polynomial_features_)
{
	n_polynomial_features = n_polynomial_features_;
	if (n_polynomial_features < 3) { n_polynomial_features = 3; }
	if (n_polynomial_features > 10) { n_polynomial_features = 10; }
	using namespace Eigen;
	using namespace EL;
	using namespace std;

	// with 
	// s = calibration marker coordinates in scene-cam space, 
	// e = pupil center coordinates in eye-camera image space
	// and feature vector 
	// p(e) = [e.x, e.y, e.x*e.x, e.x*e.y, e.y*e.y]  
	// we need to calculate linear regression weights W that minimise the squared error such that
	// s = W * p(e)

	auto s = calibration_targets;
	auto e = calibration_points;


	/*
	// test data
	MatrixXd s = MatrixXd::Zero(2, 4);
	s << 257, 556, 551, 265,
		 154, 151, 345, 334;

	MatrixXd e = MatrixXd::Zero(2, 4);
	e << 545, 485, 489, 551,
		 235, 233, 263, 265;

	cout << s << "\n\n";
	cout << e << "\n\n";
	//*/

	int n_calib_points = s.cols();
	MatrixXd pe = MatrixXd::Zero(n_polynomial_features, n_calib_points);

	// build a matrix containing all feature vectors
	for (int i = 0; i < n_calib_points; i++)
	{
		pe.col(i) = polynomial_features(e(0, i), e(1, i)).block(0, 0, n_polynomial_features, 1);
	}
	cout << "\n\npolynomial features:\n" << pe;

	// does not work !! 
	//auto pe_inv = (pe.transpose() * pe).inverse() * pe.transpose();
	//cout << pe_inv << "\n\n";
	//W_calib = s * pe_inv;

	W_calib = linear_regression(pe, s); // precision: 2.75316e-12 // according to eigen docu also higher numerically robustness
	//W_calib = s * pinv(pe);			// precision: 2.39319e-07 on test-data
	//W_calib = s * pseudoInverse(pe);	// precision: 5.82143e-12

	cout << "\n\nCalibration Matrix:\n" << W_calib;

	//calc mapping error:	
	mapping_error = 0;
	for (int i = 0; i < s.cols(); i++)
	{
		auto v = s.col(i) - W_calib * pe.col(i);
		mapping_error += v.norm();
	}
	mapping_error /= double(s.cols());

	cout << "\nmapping error: " << mapping_error << endl;
}


Calibration::Calibration()
{
	setup(5);
	marker_detector_calib.setDictionary("assets/calibration_marker_5x5.dict");
	img_marker_calib = imread("assets/calibration_marker.png");
	resize(img_marker_calib, img_marker_calib_scaled, Size(marker_size, marker_size));
	
	ar_canvas.setup();
}

void Calibration::setup(int n_calibration_points)
{
	n_calib_points = n_calibration_points;
	Calibration_base::setup(n_calib_points);
	calibration_counter = 0;
	state = STATE_PREPARE;
	tracking_lost_counter = 150; // needed to prepare the user to face his head towards the screen

	targets.clear();
	
	// reset validation
	setup_validation();
}

void Calibration::setup_validation(int n_validation_points)
{
	// TODO - n_validation_points
	validation_counter = 0; 

	// reset validation offsets
	offset = offset_validation = cv::Point2f{ 0.0f, 0.0f };
}


void Calibration::draw(cv::Mat& frame_scene_cam, cv::Mat& img_screen)
{
	const int w = img_screen.cols;
	const int h = img_screen.rows;
	const int mb = ar_canvas.marker_size;

	const auto screen_center = Point2f(0.5f * w, 0.5f * h);

	switch (state)
	{
	case Calibration::STATE_PREPARE:
		draw_prep(frame_scene_cam, img_screen);
		break;
	case Calibration::STATE_CALIBRATION:
		draw_calibration(frame_scene_cam, img_screen);
		break;
	case Calibration::STATE_VISUALIZE_CALIBRATION:
		draw_visualization(frame_scene_cam, img_screen);
		// cv::putText(img_screen, "Calibration finished !", Point2i(mb, screen_center.y), FONT_HERSHEY_SIMPLEX, 2, Scalar(100, 255, 200), 4);
		// draw gaze point after coordinate transformation		
		//circle(img_screen, p_projected, 8, Scalar(255, 0, 255), 4);
		break;
	case Calibration::STATE_VALIDATION:
		draw_validation(frame_scene_cam, img_screen);
		break;
	case Calibration::STATE_VISUALIZE_VALIDATION:
		draw_validation(frame_scene_cam, img_screen);
		break;
	default:
		break;
	}
}

void Calibration::draw_calibration(cv::Mat& frame_scene_cam, cv::Mat&  img_screen)
{
	using namespace std;
	using namespace cv;


	const int w = img_screen.cols;
	const int h = img_screen.rows;

	const auto screen_center = Point2f(0.5f * w, 0.5f * h);

	const int ms = marker_size;
	const int s = 10; // spacing to canvas border
	const int mb = ms + 2 * s;
	if (targets.empty())
	{
		switch (n_calib_points)
		{
		case 5:
			targets =
			{
				Point2f(0.5 * ms + s, 0.5 * ms + s),
				Point2f(w - 0.5 * ms - s, 0.5 * ms + s),
				Point2f(w - 0.5 * ms - s, h - 0.5 * ms - s),
				Point2f(0.5 * ms + s, h - 0.5 * ms - s),
				Point2f(0.5 * w, 0.5 * h)
			};
			break;
		case 9:
			targets =
			{
				Point2f(0.5 * ms + s, 0.5 * ms + s),
				Point2f(w - 0.5 * ms - s, 0.5 * ms + s),
				Point2f(w - 0.5 * ms - s, h - 0.5 * ms - s),
				Point2f(0.5 * ms + s, h - 0.5 * ms - s),
				Point2f(0.5 * w, 0.5 * h),

				Point2f(0.5 * ms + s, 0.5 * h),
				Point2f(0.5 * w, 0.5 * ms + s),
				Point2f(w - 0.5 * ms - s, 0.5 * h),
				Point2f(0.5 * w, h - 0.5 * ms - s)
			};
			break;
		default:
			cerr << "\nwrong number of calibration points selected.";
			break;
		}
	}



	//ar_canvas.draw(img_screen, 0, 0, w, h);


	// visualize detected calibration marker(s)
	if (markers_calib.size() > 0) //for (auto& m : markers_calib)
	{
		auto& m = markers_calib[0];
		m.draw(frame_scene_cam, Scalar(0, 0, 255), 2);
		cv::circle(frame_scene_cam, marker_calib_center, 3, Scalar(0, 255, 0), 2);
	}

	// handle marker tracking
	if (markers_calib.size() > 0)
	{
		tracking_lost_counter--;
		if (tracking_lost_counter < 0) { tracking_lost_counter = 0; } // avoid underflow
	}
	else
	{
		tracking_lost_counter++;
		if (tracking_lost_counter > 20) { tracking_lost_counter = 20; }// avoid overflow
	}

	if (calibration_counter < n_calib_points)
	{

		int x = mb + 10;
		int y = screen_center.y;
		putText(img_screen, "Please look exactly into the middle of the marker!", Point2i(x, y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2); y += 40;
		putText(img_screen, "Do not move your head, only move your eyes.", Point2i(x, y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2); y += 40;

		if (markers_calib.size() > 0)
		{
			putText(img_screen, "Press space to confirm.", Point2i(x, y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);

			// draw scene cam video stream to ease checking of head orientation towards screen
			float scaling = float(mb) / float(frame_scene_cam.rows);
			draw_preview(frame_scene_cam, img_screen, scaling, -1, img_screen.rows - mb);
		}
		else
		{
			if (tracking_lost_counter > 10)
			{
				putText(img_screen, "Please point your head to the center of the screen,", Point2i(mb, mb + 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
				putText(img_screen, "such that all markers are visible to the scene camera.", Point2i(mb, mb + 40 + 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
				draw_preview(frame_scene_cam, img_screen, 0.5f);
			}
		}

		// draw calibration marker animation effect (this is just eyecandy and has little functional relevance)
		// draws a "dust trail"
		if (marker_calib_anim_fx > 0 && calibration_counter > 0)
		{
			int n = 100;
			auto dp = (targets[calibration_counter] - targets[calibration_counter - 1]);
			for (int i = round(n * (1.0f - marker_calib_anim_fx)); i <= n; i++)
			{
				float f = i / float(n);
				float c = 255 - 255 * f * marker_calib_anim_fx;
				float ms2 = (0.5f + 0.5f * f) * ms;
				auto p = targets[calibration_counter - 1] + f * dp;
				rectangle(img_screen, Rect(p.x - 0.5f * ms2, p.y - 0.5f * ms2, ms2, ms2), Scalar(c, c, c), -1);
			}
			marker_calib_anim_fx -= 0.2f;
		}

		// draw special calibration marker on top of everything else.
		auto p = targets[calibration_counter];
		if (img_marker_calib_scaled.size[0] != ms) { resize(img_marker_calib, img_marker_calib_scaled, Size(ms, ms)); }
		img_marker_calib_scaled.copyTo(img_screen(Rect(p.x - 0.5f * ms, p.y - 0.5f * ms, ms, ms)));
	}

	imshow("screen", img_screen);
}



void Calibration::update_calibration(cv::Mat& frame_scene_cam, cv::Point2f pupil_pos, int key_pressed)
{
	// track special marker
	markers_calib = marker_detector_calib.detect(frame_scene_cam);

	// calc the marker center
	// TODO: handle the (rare?) case where multiple calibration markers are visible
	if (markers_calib.size() > 0)
	{
		auto& m = markers_calib[0];
		auto o1 = m[0]; auto p1 = m[2];
		auto o2 = m[1]; auto p2 = m[3];
		cv::Point2f p;
		if (line_intersection(o1, p1, o2, p2, p))
		{
			marker_calib_center = p;
		}

		if (int(' ') == key_pressed)
		{
			if (calibration_counter < n_calib_points)
			{
				cout << "space pressed. submitting a calibration point." << endl;

				/*
				// old code using ar canvas marker edges
				auto p = ar_canvas.image_plane;
				calibration_targets(0, calibration_counter) = p[calibration_counter].x;
				calibration_targets(1, calibration_counter) = p[calibration_counter].y;
				*/

				calibration_targets(0, calibration_counter) = marker_calib_center.x;
				calibration_targets(1, calibration_counter) = marker_calib_center.y;
				calibration_points(0, calibration_counter) = pupil_pos.x;
				calibration_points(1, calibration_counter) = pupil_pos.y;

				if (calibration_counter == n_calib_points-1)
				{
					calibrate();
					state = STATE_VISUALIZE_CALIBRATION;
				}

				calibration_counter++;
				marker_calib_anim_fx = 1.0f; // just for a neat animation effect of the calibration marker
			}
		}
	}
}

void Calibration::update(cv::Mat& frame_scene_cam,  cv::Point2f pupil_pos, int key_pressed)
{
	using namespace std;

	switch (state)
	{
	case STATE_PREPARE:
		break;
	case STATE_CALIBRATION:
		update_calibration(frame_scene_cam, pupil_pos, key_pressed);
		break;

	case STATE_VISUALIZE_CALIBRATION:
	// case STATE_VISUALIZE_VALIDATION: // TODO
		if (int('+') == key_pressed) { n_polynomial_features++; calibrate(n_polynomial_features); }
		if (int('-') == key_pressed) { n_polynomial_features--; calibrate(n_polynomial_features); }
		break;

	case STATE_VALIDATION:
		if (ar_canvas.valid() && int(' ') == key_pressed)
		{
			// map pupil position to scene camera position using calibrated 2d to 2d mapping
			p_calibrated = mapping_2d_to_2d(pupil_pos);
			p_projected = ar_canvas.transform(p_calibrated);
			validation_points(0, validation_counter) = p_projected.x;
			validation_points(1, validation_counter) = p_projected.y;
			validation_counter++;
		}
		break;
	}

}


void Calibration::draw_prep(cv::Mat& frame_scene_cam, cv::Mat& img_screen)
{
	const int w = img_screen.cols;
	const int h = img_screen.rows;
	using namespace cv;
	const int mb = ar_canvas.marker_size;

	ar_canvas.draw(img_screen, 0, 0, w, h);

	if (ar_canvas.valid())
	{
		tracking_lost_counter--;
		if (tracking_lost_counter < 0) { tracking_lost_counter = 0; } // avoid underflow
	}
	else
	{
		tracking_lost_counter++;
		if (tracking_lost_counter > 150) { tracking_lost_counter = 150; }// avoid overflow
	}

	int x = mb + 10;
	int y = 60;


	if (tracking_lost_counter > 0)
	{
		putText(img_screen, "Please point your head to the center of the screen,", Point2i(x, y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
		y += 40;
		putText(img_screen, "such that all markers are visible to the scene camera.", Point2i(x, y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
		y += 40;
		rectangle(img_screen, Rect(0.5 * w - 0.5 * tracking_lost_counter, y, tracking_lost_counter, 40), Scalar(0, 255, 0), -1);

		draw_preview(frame_scene_cam, img_screen, 1.0f, -1, -1);
	}
	else
	{
		state = STATE_CALIBRATION;
	}

	imshow("screen", img_screen);
}

void Calibration::draw_visualization(cv::Mat& frame_scene_cam, cv::Mat& img_screen)
{
	using namespace cv;

	// canvas width and hight
	const int w = img_screen.cols;
	const int h = img_screen.rows;

	// scene cam width and hight
	const int sw = frame_scene_cam.cols;
	const int sh = frame_scene_cam.rows;
	const int sx = 0.5 * w - 0.5 * sw;
	const int sy = 0.5 * h - 0.5 * sh;

	Point2f offset(sx, sy);
	// draw a grid of linearly interpolated points to give an idea what the polynomial mapping does look like
	{
		/*
		// todo get eye cam frame size
		for (int x = 0; x < 640; x += 10)
		{
			for (int y = 0; y < 480; y += 10)
			{
				auto p1 = offset + mapping_2d_to_2d(Point2f(x, y));
				auto p2 = offset + mapping_2d_to_2d(Point2f(x + 10, y));
				auto p3 = offset + mapping_2d_to_2d(Point2f(x, y + 10));
				auto p4 = offset + mapping_2d_to_2d(Point2f(x + 10, y + 10));
				line(img_screen, p1, p2, Scalar(150, 150, 150), 1);
				line(img_screen, p1, p3, Scalar(150, 150, 150), 1);
				line(img_screen, p2, p4, Scalar(150, 150, 150), 1);
				line(img_screen, p3, p4, Scalar(150, 150, 150), 1);
				circle(img_screen, p1, 2, Scalar(50, 50, 50), 1);
			}
		}
		*/

		auto fn_draw_grid = [&](Point2f p1, Point2f p2, Point2f p3, Point2f p4, float n)
		{
			for (int i = 0; i <= n; i++)
			{
				auto pp1 = p1 + i * (p4 - p1) / n;
				auto pp2 = p2 + i * (p3 - p2) / n;
				for (int k = 0; k < n; k++)
				{
					auto ppp1 = offset + mapping_2d_to_2d(pp1 + k * (pp2 - pp1) / n);
					auto ppp2 = offset + mapping_2d_to_2d(pp1 + (k + 1) * (pp2 - pp1) / n);
					line(img_screen, ppp1, ppp2, Scalar(150, 150, 150), 1);
					//circle(img_screen, ppp1, 2, Scalar(50, 50, 50), 1);
				}
			}
			for (int i = 0; i <= n; i++)
			{
				auto pp1 = p1 + i * (p2 - p1) / n;
				auto pp2 = p4 + i * (p3 - p4) / n;
				for (int k = 0; k < n; k++)
				{
					auto ppp1 = offset + mapping_2d_to_2d(pp1 + k * (pp2 - pp1) / n);
					auto ppp2 = offset + mapping_2d_to_2d(pp1 + (k + 1) * (pp2 - pp1) / n);
					line(img_screen, ppp1, ppp2, Scalar(150, 150, 150), 1);
					//circle(img_screen, ppp1, 2, Scalar(50, 50, 50), 1);
				}
			}
		};

		if (n_calib_points == 5)
		{
			auto p1 = Point2f(calibration_points(0, 0), calibration_points(1, 0));
			auto p2 = Point2f(calibration_points(0, 1), calibration_points(1, 1));
			auto p3 = Point2f(calibration_points(0, 2), calibration_points(1, 2));
			auto p4 = Point2f(calibration_points(0, 3), calibration_points(1, 3));
			fn_draw_grid(p1, p2, p3, p4, 25);
		}

		if (n_calib_points == 9)
		{
			Point2f p1, p2, p3, p4;
			p1 = Point2f(calibration_points(0, 0), calibration_points(1, 0));
			p2 = Point2f(calibration_points(0, 6), calibration_points(1, 6));
			p3 = Point2f(calibration_points(0, 4), calibration_points(1, 4));
			p4 = Point2f(calibration_points(0, 5), calibration_points(1, 5));
			fn_draw_grid(p1, p2, p3, p4, 6);

			p1 = Point2f(calibration_points(0, 6), calibration_points(1, 6));
			p2 = Point2f(calibration_points(0, 1), calibration_points(1, 1));
			p3 = Point2f(calibration_points(0, 7), calibration_points(1, 7));
			p4 = Point2f(calibration_points(0, 4), calibration_points(1, 4));
			fn_draw_grid(p1, p2, p3, p4, 6);

			p1 = Point2f(calibration_points(0, 4), calibration_points(1, 4));
			p2 = Point2f(calibration_points(0, 7), calibration_points(1, 7));
			p3 = Point2f(calibration_points(0, 2), calibration_points(1, 2));
			p4 = Point2f(calibration_points(0, 8), calibration_points(1, 8));
			fn_draw_grid(p1, p2, p3, p4, 6);

			p1 = Point2f(calibration_points(0, 5), calibration_points(1, 5));
			p2 = Point2f(calibration_points(0, 4), calibration_points(1, 4));
			p3 = Point2f(calibration_points(0, 8), calibration_points(1, 8));
			p4 = Point2f(calibration_points(0, 3), calibration_points(1, 3));
			fn_draw_grid(p1, p2, p3, p4, 6);
		}
	}




	// first, draw size scene cam borders
	auto scene_cam_rect = Rect(sx, sy, sw, sh);
	rectangle(img_screen, scene_cam_rect, Scalar(0, 0, 255), 2);

	// draw legend
	const int co = 20; // cursor offset 
	Point2i cursor( sx + co, sy+co); // text cursor coordinates
	putText(img_screen, "scene cam area", cursor, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2); cursor.y += co;
	putText(img_screen, "calibration marker positions", cursor, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255,   0,   0), 2); cursor.y += co;
	putText(img_screen, "2d-2d mapped pupil positions", cursor, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(  0, 200,   0), 2); cursor.y += co;
	putText(img_screen, "n_poly_features (press +/- to change) = " + to_string(n_polynomial_features), cursor, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 2); cursor.y += co;
	putText(img_screen, "mapping error = " + to_string(int(round(mapping_error))), cursor, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0), 2); cursor.y += co;
	

	// next, draw detected calibration marker positions and corresponding detected pupil positions
	
	for (int i = 0; i < calibration_targets.cols(); i++)
	{
		auto pm = Point2f(calibration_targets(0, i), calibration_targets(1, i)); // marker positions in scene-cam coordinates
		auto pp = Point2f(calibration_points(0, i), calibration_points(1, i)); // pupil positions on eye cam coordinates
		auto ppm = mapping_2d_to_2d(pp); // 2d_to_2d mapped pupil positions in scene-cam coordinates
		pm += offset; ppm += offset;
		line(img_screen, pm, ppm, Scalar(0, 200, 0), 2);
		circle(img_screen, pm, 5, Scalar(255, 0, 0), 2);
		circle(img_screen, ppm, 3, Scalar(0, 200, 0), 2);
	}

}

void Calibration::draw_validation(cv::Mat& frame_scene_cam, cv::Mat& img_screen)
{
	const int w = img_screen.cols;
	const int h = img_screen.rows;

	using namespace cv;
	auto sc = Point2f(0.5f * img_screen.cols, 0.5f * img_screen.rows); // screen center
	int mb = ar_canvas.marker_size;

	// todo: validation points at random | different locations than calibration points
	array<Point2f,5> val_targets = 
	{
		Point2f(sc.x  , mb),
		Point2f(w - mb, sc.y),
		Point2f(sc.x, h - mb),
		Point2f(mb, sc.y),
		sc
	};


	Point2f p1, p2; // arrow start- and end-point
	if (validation_counter < val_targets.size())
	{
		if (ar_canvas.valid())
		{
			// point to the edge where the user has to look at
			p2 = val_targets[validation_counter];
			if (p2 != sc)
			{
				p1 = p2 - 0.25 * (p2 - sc); // draw an arrow pointing from center towards validation target
			}
			else
			{
				p1 = p2 + Point2f(0, 0.25 * h); // draw the arrow differently for the circle at the center 
			}
			circle(img_screen, p2, 20, Scalar(0, 240, 255), FILLED);
			arrowedLine(img_screen, p1, p2, Scalar(255, 0, 255), 6, 8, 0, 0.2);
			ar_canvas.draw(img_screen, 0, 0, w, h);
			putText(img_screen, "Please look exactly at the tip of the Arrow !", Point2i(mb + 20, mb + 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);
			putText(img_screen, "Press space to confirm.", Point2i(mb + 20, mb + 50 + 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);
		}
		else
		{
			ar_canvas.draw(img_screen, 0, 0, w, h);
			putText(img_screen, "Please point your head to the center of the screen,", Point2i(mb + 20, sc.y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
			putText(img_screen, "such that all markers are visible to the scene camera.", Point2i(mb + 20, sc.y + 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
		}
	}
	else
	{

		// draw legend
		const int co = 40; // cursor offset 
		Point2i cursor(mb + 40, mb + 40); // text cursor coordinates

		putText(img_screen, "Validation finished !", cursor, FONT_HERSHEY_SIMPLEX, 1, Scalar(100, 255, 200), 2); cursor.y += co;
		// todo: print validation pixel error
		float error = 0.0f;
		offset_validation = Point2f(0.0f, 0.0f);

		for (size_t i = 0; i < val_targets.size(); i++)
		{
			p1 = val_targets[i];
			p2 = Point2f(validation_points(0, i), validation_points(1, i)) - offset;
			circle(img_screen, p1, 20, Scalar(0, 240, 255), FILLED);
			circle(img_screen, p2, 5, Scalar(255, 200, 0), FILLED);
			line(img_screen, p1, p2, Scalar(255, 200, 0));
			error += norm(p2 - p1);
			offset_validation += p2 - p1;
		}
		offset_validation /= float(val_targets.size());
		error /= float(val_targets.size());
		auto str_offset = "(" + to_string(offset_validation.x) + ", " + to_string(offset_validation.y) + ")";

		putText(img_screen, "mean validation error:" + to_string(error), cursor, FONT_HERSHEY_SIMPLEX, 1, Scalar(100, 255, 200), 2);  cursor.y += co;
		putText(img_screen, "mean offset (x,y)    :" + str_offset, cursor, FONT_HERSHEY_SIMPLEX, 1, Scalar(100, 255, 200), 2);  cursor.y += co;

		ar_canvas.draw(img_screen, 0, 0, w, h);
		// draw gaze point after coordinate transformation		
		circle(img_screen, p_projected, 8, Scalar(255, 0, 255), 4);

	}

	imshow("screen", img_screen);
}
