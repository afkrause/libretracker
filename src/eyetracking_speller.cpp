#include "eyetracking_speller.h"


void Eyetracking_speller::calibrate()
{
	using namespace Eigen;
	using namespace EL;

	// with 
	// s = coordinates in screen space, 
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
	MatrixXd pe = MatrixXd::Zero(4, n_calib_points);

	// build a matrix containing all feature vectors
	for (int i = 0; i < n_calib_points; i++)
	{
		pe.col(i) = polynomial_features(e(0, i), e(1, i));
	}
	cout << pe << "\n\n";

	// does not work !! 
	//auto pe_inv = (pe.transpose() * pe).inverse() * pe.transpose();
	//cout << pe_inv << "\n\n";
	//W_calib = s * pe_inv;

	W_calib = linear_regression(pe, s); // precision: 2.75316e-12 // according to eigen docu also higher numerically robustness
	//W_calib = s * pinv(pe);			// precision: 2.39319e-07 on test-data
	//W_calib = s * pseudoInverse(pe);	// precision: 5.82143e-12

	cout << "Calibration Matrix:\n" << W_calib << "\n\n";

	//mapping error:
	double err = 0;
	for (int i = 0; i < s.cols(); i++)
	{
		err += (s.col(i) - W_calib * pe.col(i)).norm();
	}
	cout << "mapping error: " << err << endl;
}


void Eyetracking_speller::setup()
{
	using namespace cv;
	using namespace EL;

	calibration_targets = calibration_points = MatrixXd::Zero(2, 4); // atm: 4 point calibration 
	validation_points = MatrixXd::Zero(2, 5);
	W_calib = MatrixXd::Zero(2, 4); // assuming 4 polynomial features

	// TODO: load previous calibration matrix from file

	try
	{
		setUseOptimized(true);

		speller.setup();
		ar_canvas.setup();

		eye_camera = select_camera("select eye camera number (0..n):");
		scene_camera = select_camera("select scene camera number (0..n):");

		// setup gradient based pupil capture
		opt = load_parameters(SETTINGS_LPW);
		timm.set_options(opt);
	}
	catch (std::exception &e)
	{
		cout << "Exception :" << e.what() << endl;
	}


	// GUI

	bool timm_gui_initialized = false;
#ifdef USE_FLTK_GUI
	sg = Simple_gui(min(Fl::w() - 200, 1420), 180, 400, 600);

	sg.add_separator_box("camera calibration (not implemented yet):");
	sg.add_button("scene camera", [&]() { state = STATE_CALIBRATION_SCENE_CAM; }, 2, 0);
	sg.add_button("eye camera", [&]() { state = STATE_CALIBRATION_EYE_CAM; }, 3, 1);
	sg.add_button("save", [&]() {}, 3, 2);

	sg.add_separator_box("1. eyetracker calibration (repeat by klicking again):");
	sg.add_button("4-point", [&]() { calibration_counter = 0; state = STATE_CALIBRATION; }, 3, 0);
	sg.add_button("todo (9pt)", [&]() { calibration_counter = 0; state = STATE_CALIBRATION; }, 3, 1);
	sg.add_button("visualize", [&]() { calibration_counter = 0; state = STATE_CALIBRATION_VISUALIZE; }, 3, 2);

	sg.add_separator_box("2. validation (optional) 3. run speller:");
	sg.add_button("2. validate", [&]() { validation_counter = 0; state = STATE_VALIDATION;  }, 2, 0);
	sg.add_button("3. run speller", [&]() { state = STATE_RUNNING; }, 2, 1);

	sg.add_separator_box("jitter filter (double exponential filter)");
	sg.add_slider("smoothing", filter_smoothing, 0, 1, 0.01);
	sg.add_slider("predictive", filter_predictive, 0, 1, 0.001);

	sg.add_separator_box("settings:");
	sg.add_slider("canvas width", gui_param_w, 640, 5000, 10);
	sg.add_slider("canvas height", gui_param_h, 480, 3000, 10);
	sg.add_button("swap cameras", [&]() { auto tmp = scene_camera; scene_camera = eye_camera; eye_camera = tmp; }, 3, 0);
	sg.add_button("settings", [&]() { if (!timm_gui_initialized) { setup_gui(); timm_gui_initialized = true; }; sg.show(); }, 3, 1);
	sg.add_button("quit", [&]() { is_running = false; }, 3, 2);

	/*
	sg.add_button("quit", [&]() { is_running = false; }, 3, 2);
	*/
	sg.finish();

#endif
}

void Eyetracking_speller::draw_validation()
{
	using namespace cv;
	auto sc = Point2f(0.5f*img_screen.cols, 0.5f*img_screen.rows);
	int mb = ar_canvas.marker_size + ar_canvas.marker_border;

	array<Point2f, 5> validation_targets = { Point2f(sc.x  , mb), Point2f(w - mb, sc.y), Point2f(sc.x, h - mb), Point2f(mb, sc.y), sc };

	Point2f p1, p2; // arrow start- and end-point
	if (validation_counter < 5)
	{
		if (ar_canvas.ok == 4)
		{
			// point to the edge where the user has to look at
			p2 = validation_targets[validation_counter];
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
		putText(img_screen, "Validation finished !", Point2i(mb + 20, sc.y - 50), FONT_HERSHEY_SIMPLEX, 2, Scalar(100, 255, 200), 4);
		// todo: print validation pixel error
		float error = 0.0f;
		for (size_t i = 0; i < validation_targets.size(); i++)
		{
			p1 = validation_targets[i];
			p2 = Point2f(validation_points(0, i), validation_points(1, i));
			circle(img_screen, p1, 20, Scalar(0, 240, 255), FILLED);
			circle(img_screen, p2, 5, Scalar(255, 200, 0), FILLED);
			line(img_screen, p1, p2, Scalar(255, 200, 0));
			error += norm(p2 - p1);
		}
		error /= validation_targets.size();
		putText(img_screen, "mean validation error:" + to_string(error), Point2i(mb + 20, sc.y - 10), FONT_HERSHEY_SIMPLEX, 2, Scalar(100, 255, 200), 4);

		ar_canvas.draw(img_screen, 0, 0, w, h);
		// draw gaze point after coordinate transformation		
		circle(img_screen, p_projected, 8, Scalar(255, 0, 255), 4);

	}
}

void Eyetracking_speller::draw_calibration()
{
	using namespace cv;

	auto screen_center = Point2f(0.5f*img_screen.cols, 0.5f*img_screen.rows);

	int mb = ar_canvas.marker_size + ar_canvas.marker_border;

	const int s = 4; // spacing of the arrow tip from the marker edge
	array<Point2f, 4> look_pos = { Point2f(mb + s, mb + s),  Point2f(w - mb - s, mb + s), Point2f(w - mb - s, h - mb - s), Point2f(mb + s, h - mb - s) };

	if (calibration_counter < 4)
	{
		if (ar_canvas.ok == 4)
		{
			// point to the edge where the user has to look at
			auto p2 = look_pos[calibration_counter];
			auto p1 = p2 - 0.25 * (p2 - screen_center);
			arrowedLine(img_screen, p1, p2, Scalar(200, 100, 255), 2);
			ar_canvas.draw(img_screen, 0, 0, w, h);
			putText(img_screen, "Please look exactly at the corner of the Marker !", Point2i(mb, screen_center.y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);
			putText(img_screen, "Press space to confirm.", Point2i(mb, screen_center.y + 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);
		}
		else
		{
			ar_canvas.draw(img_screen, 0, 0, w, h);
			putText(img_screen, "Please point your head to the center of the screen,", Point2i(mb, screen_center.y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
			putText(img_screen, "such that all markers are visible to the scene camera.", Point2i(mb, screen_center.y + 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
		}
	}
	else
	{
		putText(img_screen, "Calibration finished !", Point2i(mb, screen_center.y), FONT_HERSHEY_SIMPLEX, 2, Scalar(100, 255, 200), 4);
	}
}


void Eyetracking_speller::draw_running()
{
	using namespace cv;
	int mb = ar_canvas.marker_size + ar_canvas.marker_border;

	ar_canvas.draw(img_screen, 0, 0, w, h);
	// ********************************************************
	// draw keyboard and handle events
	speller.draw_keyboard(img_screen, 0, mb, w, h - 2 * mb, mb, p_projected.x, p_projected.y, eye_button_up);

	// draw gaze point after coordinate transformation		
	circle(img_screen, p_projected, 8, Scalar(255, 0, 255), 4);

}

void Eyetracking_speller::draw()
{
	using namespace cv;
	// ********************************************************
	// draw markers on screen // and visualize gaze point after coordinate transformation
	img_screen_background.copyTo(img_screen);


	switch (state)
	{
	case STATE_INSTRUCTIONS: break;
	case STATE_CALIBRATION: draw_calibration(); break;
	case STATE_CALIBRATION_VISUALIZE: draw_calibration_vis(); break;
	case STATE_VALIDATION:  draw_validation(); break;
	case STATE_RUNNING:		draw_running(); break;
		break;

	default: break;

	}


	// ********************************************************
	// draw gaze point after calibration
	circle(frame_scene_cam, p_calibrated, 4, Scalar(255, 0, 255), 2);

	// ********************************************************
	// display images
	imshow("scene_cam", frame_scene_cam);
	imshow("screen", img_screen);

}

void Eyetracking_speller::update()
{
	using namespace cv;

	// update values that might have been changed via GUI sliders
	gaze_filter_x.set_params(1.0 - filter_smoothing, filter_predictive);
	gaze_filter_y.set_params(1.0 - filter_smoothing, filter_predictive);

	w = gui_param_w;
	h = gui_param_h;
	if (w_old != w || h_old != h)
	{
		img_screen_background = Mat(h, w, CV_8UC3, Scalar(255, 255, 255)); // white background
		w_old = w; h_old = h;
	}

	//namedWindow("eye_cam", WINDOW_AUTOSIZE);
	//namedWindow("screen", WINDOW_AUTOSIZE);

	/*
	// simulate gaze point using mouse
	auto cb_mouse = [&](int event, int x, int y, int flags, void* userdata)
	{
		mx = x;
		my = y;
		if (event == CV_EVENT_LBUTTONUP) { eye_button_up = true; eye_button_down = false; }
		if (event == CV_EVENT_LBUTTONDOWN) { eye_button_down = true; eye_button_up = false; }
		// cout << "mauspos: " << x << "\t" << y << endl; };
	};
	setMouseCallback("eye_cam", cb_mouse, nullptr);
	*/

	timer.tick();

	scene_camera->read(frame_scene_cam);
	eye_camera->read(frame_eye_cam);


	// ********************************************************
	// get pupil position

	cv::cvtColor(frame_eye_cam, frame_eye_gray, cv::COLOR_BGR2GRAY);
	if (opt.blur > 0) { GaussianBlur(frame_eye_gray, frame_eye_gray, cv::Size(opt.blur, opt.blur), 0); }

	std::tie(pupil_pos, pupil_pos_coarse) = timm.pupil_center(frame_eye_gray);
	timm.visualize_frame(frame_eye_gray, pupil_pos, pupil_pos_coarse);

	// ********************************************************
	// map pupil position to scene camera position using calibrated 2d to 2d mapping
	p_calibrated = mapping_2d_to_2d(Point2f(pupil_pos.x, pupil_pos.y));

	// ********************************************************
	ar_canvas.update(frame_scene_cam, p_calibrated);
	p_projected = ar_canvas.p_projected;

	// jitter filter
	p_projected.x = gaze_filter_x(p_projected.x);
	p_projected.y = gaze_filter_y(p_projected.y);

	mx = p_projected.x;
	my = p_projected.y;


	// ********************************************************
	// event propagation and keyboard handling 
	key_pressed = cv::waitKey(1);

	if (ar_canvas.ok == 4)
	{
		//if (VK_SPACE == key_pressed)
		if( int(' ') == key_pressed )
		{
			if (STATE_CALIBRATION == state)
			{
				if (calibration_counter < 4)
				{
					cout << "space pressed. submitting calibration point." << endl;
					auto p = ar_canvas.image_plane;
					calibration_targets(0, calibration_counter) = p[calibration_counter].x;
					calibration_targets(1, calibration_counter) = p[calibration_counter].y;

					calibration_points(0, calibration_counter) = pupil_pos.x;
					calibration_points(1, calibration_counter) = pupil_pos.y;

					if (calibration_counter == 3) { calibrate(); }
					/*
					for SMI
					if (calibration_counter == 0) { etg.calibrate(p[0].x, p[0].y, frame_number); }
					if (calibration_counter == 1) { etg.calibrate(p[1].x, p[1].y, frame_number); }
					if (calibration_counter == 2) { etg.calibrate(p[2].x, p[2].y, frame_number); }
					*/
					calibration_counter++;
				}
				else
				{
					cout << "calibrated point = " << p_calibrated.x << "\t" << p_calibrated.y << endl;
					eye_button_up = true;
				}
			}

			if (STATE_VALIDATION == state)
			{
				validation_points(0, validation_counter) = p_projected.x;
				validation_points(1, validation_counter) = p_projected.y;
				validation_counter++;
			}
		}
	}

#ifdef USE_FLTK_GUI
	sg.update();
#endif
}



void Eyetracking_speller::run()
{
	setup();
	// main loop
	while (is_running)
	{
		update();
		draw();

		if (27 == key_pressed) // VK_ESCAPE
		{
			break;
		}
	}
}