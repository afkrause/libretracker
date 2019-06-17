#include "eyetracking_speller.h"

#include "deps/s/sdl_opencv.h"



// helper function. when pressing a button in the fltk gui, 
// calling this function grabs the focus of the specified opencv window
void grab_focus(char* wname)
{
	cv::destroyWindow(wname); cv::namedWindow(wname);
}



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
		auto v = s.col(i) - W_calib * pe.col(i);
		err += v.norm();
	}
	cout << "mapping error: " << err << endl;
}


static void mouse_callback(int event, int x, int y, int, void* user_data)
{
	using namespace cv;

	Eyetracking_speller* ptr = static_cast<Eyetracking_speller*>(user_data);

	/*
	bool eye_button_up = false, eye_button_down = false;

	if (event == EVENT_LBUTTONUP) { eye_button_up = true; eye_button_down = false; }
	if (event == EVENT_LBUTTONDOWN) { eye_button_down = true; eye_button_up = false; }
	*/

	//ptr->set_mouse(x, y, eye_button_up, eye_button_down);
	ptr->set_mouse(x, y, false, false);
	
	if (event == EVENT_LBUTTONUP) { cout << "mauspos: " << x << "\t" << y << endl; }
}



void Eyetracking_speller::setup(enum_simd_variant simd_width)
{
	
	Pupil_tracking::setup(simd_width);

	using namespace cv;
	using namespace EL;

	calibration_targets = calibration_points = MatrixXd::Zero(2, 4); // atm: 4 point calibration 
	validation_points = MatrixXd::Zero(2, 5);
	W_calib = MatrixXd::Zero(2, 4); // assuming 4 polynomial features

	// TODO: load previous calibration matrix from file

	speller.setup();
	ar_canvas.setup();

	// setup gradient based pupil capture
	opt = load_parameters(SETTINGS_LPW);
	timm.set_options(opt);

	// prepare gui for gradient based pupil capture
	params = set_params(opt);
	//setup_gui();

	// GUI
	//sg = Simple_gui(min(Fl::w() - 200, 1420), 180, 400, 600);
	sg = Simple_gui(20, 60, 400, 600);

	sg.add_separator_box("1. adjust canvas size and AR-marker tracking:");
	sg.add_slider("canvas width", gui_param_w, 640, 5000, 10);
	sg.add_slider("canvas height", gui_param_h, 480, 3000, 10);
	sg.add_slider("AR marker size", gui_param_marker_size, 40, 400, 10, "Width and height of the AR marker in pixel.");
	sg.add_slider("detection size", ar_canvas.min_marker_size, 0.005, 0.1, 0.001, "minimum detection size of a marker (in percent of total image area)");

	sg.add_separator_box("2. adjust cameras and pupil tracking:");
	sg.add_button("swap cameras", [&]() { auto tmp = scene_camera; scene_camera = eye_camera; eye_camera = tmp; }, 3, 0);
	sg.add_button("eye-cam", [&]() { eye_cam_controls.setup(eye_camera, 20, 20, 400, 400, "Eye-Camera Controls"); }, 3, 1);
	sg.add_button("scene-cam", [&]() { scene_cam_controls.setup(scene_camera, 20, 20, 400, 400, "Scene-Camera Controls"); }, 3, 2);
	sg.add_button("adjust pupil tracking", [&]() { setup_gui(); sg.show(); }, 1, 0);

	/*
	// TODO TODO TODO !!
	sg.add_separator_box("camera calibration (not implemented yet):");
	sg.add_button("scene camera", [&]() { state = STATE_CALIBRATION_SCENE_CAM; }, 2, 0);
	sg.add_button("eye camera", [&]() { state = STATE_CALIBRATION_EYE_CAM; }, 3, 1);
	sg.add_button("save", [&]() {}, 3, 2);
	*/
	
	sg.add_separator_box("3. eyetracker calibration & validation:");
	sg.add_button("calibrate", [&]() { grab_focus("screen"); calibration_counter = 0; state = STATE_CALIBRATION; }, 4, 0);
	//sg.add_button("todo (9pt)", [&]() { calibration_counter = 0; state = STATE_CALIBRATION; }, 3, 1);
	sg.add_button("visualize", [&]() { calibration_counter = 0; state = STATE_CALIBRATION_VISUALIZE; }, 4, 1);
	sg.add_button("validate", [&]() { grab_focus("screen");  validation_counter = 0; state = STATE_VALIDATION;  }, 4, 2, "check the calibration by testing additional points.");
	sg.add_button("fix offset", [&]() { offset = offset_validation; }, 4, 3, "remove a potential systematic offset found after validation.");

	sg.add_separator_box("jitter filter (double exponential filter)");
	sg.add_slider("smoothing", filter_smoothing, 0, 1, 0.01);
	sg.add_slider("predictive", filter_predictive, 0, 1, 0.001);

	sg.add_separator_box("4. run modules:");
	sg.add_button("run speller", [&]() { grab_focus("screen"); state = STATE_RUNNING; }, 1, 0);
	sg.add_button("record and stream to client", [&]() { run_ssvep(); }, 1, 0);
	sg.add_button("quit", [&]() { sg.hide(); Fl::check(); is_running = false; }, 1, 0);


	sg.finish();
	sg.show();

	//#ifndef HAVE_OPENGL
	//if (flags & CV_WINDOW_OPENGL) CV_ERROR(CV_OpenGlNotSupported, "Library was built without OpenGL support");
	//#else
	//setOpenGlDrawCallback("windowName", glCallback);

	namedWindow("eye_cam");
	namedWindow("screen");// , WINDOW_OPENGL | WINDOW_AUTOSIZE);

	// place the main window to the right side of the options gui
	moveWindow("screen", 450, 60);
	resizeWindow("screen", w, h);
	
	moveWindow("eye_cam", 20, 700);

	// uncomment to simulate gaze using mouse 
	cv::setMouseCallback("screen", mouse_callback, this);

}

void Eyetracking_speller::draw_validation()
{
	using namespace cv;
	auto sc = Point2f(0.5f*img_screen.cols, 0.5f*img_screen.rows); // screen center
	int mb = ar_canvas.marker_size + ar_canvas.marker_border;

	array<Point2f, 5> validation_targets = { Point2f(sc.x  , mb), Point2f(w - mb, sc.y), Point2f(sc.x, h - mb), Point2f(mb, sc.y), sc };

	Point2f p1, p2; // arrow start- and end-point
	if (validation_counter < 5)
	{
		if (ar_canvas.valid())
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
		offset_validation = Point2f(0.0f, 0.0f);

		for (size_t i = 0; i < validation_targets.size(); i++)
		{
			p1 = validation_targets[i];
			p2 = Point2f(validation_points(0, i), validation_points(1, i)) - offset;
			circle(img_screen, p1, 20, Scalar(0, 240, 255), FILLED);
			circle(img_screen, p2, 5, Scalar(255, 200, 0), FILLED);
			line(img_screen, p1, p2, Scalar(255, 200, 0));
			error += norm(p2 - p1);
			offset_validation += p2 - p1;
		}
		offset_validation /= float(validation_targets.size());
		error /= float(validation_targets.size());
		auto str_offset = "(" + to_string(offset_validation.x) + ", " + to_string(offset_validation.y) + ")";

		putText(img_screen, "mean validation error:" + to_string(error), Point2i(mb + 20, sc.y - 20), FONT_HERSHEY_SIMPLEX, 2, Scalar(100, 255, 200), 4);
		putText(img_screen, "mean offset (x,y)    :" + str_offset      , Point2i(mb + 20, sc.y + 20), FONT_HERSHEY_SIMPLEX, 2, Scalar(100, 255, 200), 4);

		ar_canvas.draw(img_screen, 0, 0, w, h);
		// draw gaze point after coordinate transformation		
		circle(img_screen, p_projected, 8, Scalar(255, 0, 255), 4);

	}

	imshow("screen", img_screen);
}

void Eyetracking_speller::draw_calibration()
{
	using namespace cv;

	auto screen_center = Point2f(0.5f*img_screen.cols, 0.5f*img_screen.rows);

	int mb = ar_canvas.marker_size + ar_canvas.marker_border;

	const int s = 4; // spacing of the arrow tip from the marker edge
	array<Point2f, 4> look_pos = { Point2f(mb + s, mb + s),  Point2f(w - mb - s, mb + s), Point2f(w - mb - s, h - mb - s), Point2f(mb + s, h - mb - s) };
	
	ar_canvas.draw(img_screen, 0, 0, w, h);

	if (ar_canvas.valid())
	{
		tracking_lost_counter--;
		if (tracking_lost_counter < 0) { tracking_lost_counter = 0; } // avoid underflow
	}
	else
	{
		tracking_lost_counter++;
		if (tracking_lost_counter > 20) { tracking_lost_counter = 20; }// avoid overflow
	}


	if (tracking_lost_counter < 10)
	{
		if (calibration_counter < 4)
		{
			// point to the edge where the user has to look at
			auto p2 = look_pos[calibration_counter];
			auto p1 = p2 - 0.25 * (p2 - screen_center);
			arrowedLine(img_screen, p1, p2, Scalar(200, 100, 255), 2);
			putText(img_screen, "Please look exactly at the corner of the Marker !", Point2i(mb, screen_center.y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);
			putText(img_screen, "Press space to confirm.", Point2i(mb, screen_center.y + 40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);
		}
		else
		{
			putText(img_screen, "Calibration finished !", Point2i(mb, screen_center.y), FONT_HERSHEY_SIMPLEX, 2, Scalar(100, 255, 200), 4);

			// draw gaze point after coordinate transformation		
			circle(img_screen, p_projected, 8, Scalar(255, 0, 255), 4);
		}
		float scaling = float(mb) / float(frame_scene_cam.rows);
		draw_scene_cam_to_screen(scaling, -1, img_screen.rows - mb);
	}
	else
	{
		putText(img_screen, "Please point your head to the center of the screen,", Point2i(mb, mb+40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
		putText(img_screen, "such that all markers are visible to the scene camera.", Point2i(mb, mb+40+40), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 2);
		draw_scene_cam_to_screen(1.0f, -1, -1);
	}

	imshow("screen", img_screen);
}


void Eyetracking_speller::draw_scene_cam_to_screen(float scaling, int x, int y)
{
	if (scaling == 1.0f)
	{
		frame_scene_cam_scaled = frame_scene_cam;
	}
	else
	{
		cv::resize(frame_scene_cam, frame_scene_cam_scaled, cv::Size(), scaling, scaling);
	}

	if (img_screen.rows > frame_scene_cam_scaled.rows && img_screen.cols > frame_scene_cam_scaled.cols)
	{
		if (x == -1) { x = round(0.5f * (img_screen.cols - frame_scene_cam_scaled.cols)); }
		if (y == -1) { y = round(0.5f * (img_screen.rows - frame_scene_cam_scaled.rows)); }
		frame_scene_cam_scaled.copyTo(img_screen(cv::Rect(x, y, frame_scene_cam_scaled.cols, frame_scene_cam_scaled.rows)));
	}
}


void Eyetracking_speller::draw_instructions()
{
	using namespace cv;
	int mb = ar_canvas.marker_size + ar_canvas.marker_border;
	// auto screen_center = Point2f(0.5f*img_screen.cols, 0.5f*img_screen.rows);

	ar_canvas.draw(img_screen, 0, 0, w, h);

	int y = mb + 25;
	auto print_txt = [&](const char * t)
	{
		putText(img_screen, t, Point2i(mb, y), FONT_HERSHEY_DUPLEX, 1, Scalar(255, 170, 0), 2);
		y += 35;
	};

	print_txt("1. Adjust the size of the window such that the scene camera");
	print_txt("   can see all AR markers at your current distance.");
	print_txt("2. Calibrate the eyetracker, then validate (optional).");
	print_txt("3. Press the run button to start the speller.");

	draw_scene_cam_to_screen(1.0);

	imshow("screen", img_screen);

}

void Eyetracking_speller::draw_speller(bool ssvep)
{
	using namespace cv;
	int mb = ar_canvas.marker_size + ar_canvas.marker_border;

	ar_canvas.draw(img_screen, 0, 0, w, h);
	// ********************************************************
	// draw keyboard and handle events
	if (ssvep)
	{
		speller.draw_keyboard_ssvep(img_screen, 0, mb, w, h - 2 * mb, mb, p_projected.x, p_projected.y, eye_button_up);
	}
	else
	{
		speller.draw_keyboard(img_screen, 0, mb, w, h - 2 * mb, mb, p_projected.x, p_projected.y, eye_button_up);
	}

	// draw gaze point after coordinate transformation		
	circle(img_screen, p_projected, 8, Scalar(255, 0, 255), 4);

	// draw gaze point after calibration
	circle(frame_scene_cam, p_calibrated, 4, Scalar(255, 0, 255), 2);

	// imshow("scene_cam", frame_scene_cam_copy);
	float scaling = float(mb) / float(frame_scene_cam.rows);
	draw_scene_cam_to_screen(scaling, -1, img_screen.rows - mb);
}

void Eyetracking_speller::draw()
{
	using namespace cv;
	// ********************************************************
	// draw markers on screen // and visualize gaze point after coordinate transformation
	img_screen_background.copyTo(img_screen);

	// visualize marker detection 
	ar_canvas.draw_detected_markers(frame_scene_cam);

	switch (state)
	{
	case STATE_INSTRUCTIONS: draw_instructions();  break;
	case STATE_CALIBRATION: draw_calibration(); break;
	case STATE_CALIBRATION_VISUALIZE: draw_calibration_vis(); break;
	case STATE_VALIDATION:  draw_validation(); break;
	case STATE_RUNNING:		draw_speller(); break;
		break;

	default: break;

	}

	imshow("screen", img_screen);

}


// update function for all steps from camera setup, calibration, validation and single threaded speller
// multithreaded eyetracking / speller use a different function
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

	ar_canvas.marker_size = round(gui_param_marker_size);
	ar_canvas.marker_border = round(25.0f * ar_canvas.marker_size / 100.0f);


	//timer.tick();

	eye_camera->read(frame_eye_cam);
	scene_camera->read(frame_scene_cam);

		
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
	ar_canvas.update(frame_scene_cam);
	if (ar_canvas.valid())
	{
		p_projected = ar_canvas.transform(p_calibrated);
	}

	// uncomment this to simulate gaze using the computer mouse
	// p_projected = Point2f(mx, my);

	// jitter filter
	p_projected.x = gaze_filter_x(p_projected.x);
	p_projected.y = gaze_filter_y(p_projected.y);


	// ********************************************************
	// event propagation and keyboard handling 
	key_pressed = cv::waitKey(1);

	if (ar_canvas.valid())
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

			if (STATE_RUNNING == state)
			{
				// use space as acknowledgement for a selected letter
				eye_button_up = true;
			}
		}
	}

	sg.update();
	eye_cam_controls.update();
}



void Eyetracking_speller::run(enum_simd_variant simd_width, int eye_cam_id, int scene_cam_id)
{
	cv::setUseOptimized(true);
	eye_camera = select_camera("select eye camera number (0..n):", eye_cam_id);
	scene_camera = select_camera("select scene camera number (0..n):", scene_cam_id);


	cout << "\nTo improve calibration results, the autofocus of both eye- and scene camera will be disabled.\n";
	cout << "\n The autofocus can be turned on/off via the camera menu.";
	//cout << "disable autofocus of both cameras (y/n):"; char c; cin >> c;
	//if (c == 'y')
	{
		eye_camera->set(cv::CAP_PROP_AUTOFOCUS, 0);
		scene_camera->set(cv::CAP_PROP_AUTOFOCUS, 0);
		cout << "\nautofocus disabled.\n";
	}

	setup(simd_width);

	// main loop
	while (is_running)
	{
		// for gui stuff
		opt = set_options(params);
		timm.set_options(opt);

		update();
		draw();

		if (27 == key_pressed) // VK_ESCAPE
		{
			break;
		}
	}
}


#include <lsl_cpp.h>
#include "lt_lsl_protocol.h"
#include <limits>

// multithreaded capture and rendering to ensure flicker stimuli are presented with the monitor refresh rate
// separate blocking function with a while loop
void Eyetracking_speller::run_ssvep()
{
	using namespace cv;
	using namespace lsl;
	using namespace chrono;
	
	cv::destroyAllWindows();

	//hide gui to avoid multithreading problems (especially with changing camera properties )
	sg.hide();
	bool run = true;
	Simple_gui sg_local(50,50,150,100, "Record and Stream");
	sg_local.add_button("stop streaming", [&](){ run = false;  });
	//sg_local.add_button("quit program", [&]() { exit(EXIT_SUCCESS);} );
	sg_local.finish();
	
	// Sdl_opencv sdl;

	
	//////////////////////////////
	// launch the capture threads
	thread_eyecam.setup(eye_camera, "eyecam");
	thread_scenecam.setup(scene_camera, "scncam");

	cout << "waiting for the first video frames to arrive..";
	// wait for frames to arrive
	while (!(thread_eyecam.new_frame && thread_scenecam.new_frame)) { cout << "."; cv::waitKey(1); this_thread::sleep_for(150ms); }
	cout << "\nthe first frame of both the eye- and scenecam has arrived.\n";
	//////////////////////////////



	vector<double> eye_data(LT_N_EYE_DATA);
	vector<double> marker_data(1+4*2);

	// labstreaming layer 
	// todo: add correct sampling rate
	cout << "creating labstreaming layer outlet for simulated EEG data..\n";
	stream_outlet lsl_out_eye(stream_info("LT_EYE", "LT_EYE", LT_N_EYE_DATA, 30, cf_double64));
	stream_outlet lsl_out_marker(stream_info("LT_MARKER", "LT_MARKER", 1 + 4 * 2, 30, cf_double64));


	auto time_start = chrono::high_resolution_clock::now();
	
	Timer timer0(500, "\nframe :"); // man duration of individual frames. for 60 Hz monitor refresh rate, it should be close to 16.66 ms
	Timer timer1(500, "\nupdate:");
	Timer timer2(500, "\nrender:");

	while (run)
	{
		timer0.tick();
		timer1.tick();

		// process events
		//if (sdl.waitKey().sym == SDLK_ESCAPE) { break; }

		// ********************************************************
		// copy camera data from the capture threads
		if (thread_scenecam.new_frame)
		{
			thread_scenecam.get_frame(frame_scene_cam);
			thread_scenecam.new_frame = false;

			ar_canvas.update(frame_scene_cam);

			// send marker data (helpful in the client for visualizing marker positions
			marker_data[0] = duration_cast<duration<double>>(high_resolution_clock::now() - time_start).count();
			for (int i = 0; i < ar_canvas.image_plane.size(); i++)
			{
				marker_data[1 + 2 * i + 0] = double(ar_canvas.image_plane[i].x) / frame_scene_cam.cols;
				marker_data[1 + 2 * i + 1] = double(ar_canvas.image_plane[i].y) / frame_scene_cam.rows;
			}
			lsl_out_marker.push_sample(marker_data);
		}

		// TODO: if the eye cam has a higher FPS than the render thread, the pupil center calculation should be in a separate thread !
		if (thread_eyecam.new_frame)
		{
			thread_eyecam.get_frame(frame_eye_cam);
			thread_eyecam.new_frame = false;

			// ********************************************************
			// get pupil position
			cv::cvtColor(frame_eye_cam, frame_eye_gray, cv::COLOR_BGR2GRAY);
			if (opt.blur > 0) { GaussianBlur(frame_eye_gray, frame_eye_gray, cv::Size(opt.blur, opt.blur), 0); }
			std::tie(pupil_pos, pupil_pos_coarse) = timm.pupil_center(frame_eye_gray);

			// ********************************************************
			// map pupil position to scene camera position using calibrated 2d to 2d mapping
			p_calibrated = mapping_2d_to_2d(Point2f(pupil_pos.x, pupil_pos.y));

			//std::nan
			p_projected = Point2f(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
			if (ar_canvas.valid())
			{
				//p_projected = ar_canvas.transform(p_calibrated);
				p_projected = ar_canvas.transform(p_calibrated, ar_canvas.screen_plane_external);
				cout << "\n" << p_projected;
			}

			eye_data[LT_TIMESTAMP] = duration_cast<duration<double>>(high_resolution_clock::now() - time_start).count();
			eye_data[LT_PUPIL_X] = double(pupil_pos.x) / frame_eye_cam.cols;
			eye_data[LT_PUPIL_Y] = double(pupil_pos.y) / frame_eye_cam.rows;
			eye_data[LT_GAZE_X] = double(p_calibrated.x) / frame_scene_cam.cols;
			eye_data[LT_GAZE_Y] = double(p_calibrated.y) / frame_scene_cam.rows;
			eye_data[LT_SCREEN_X] = p_projected.x;
			eye_data[LT_SCREEN_Y] = p_projected.y;

			// todo fix: jitter filter is adjusted to pixel coordinates , not normalized coordinates
			// jitter filter (updated at eyecam fps)
			//p_projected.x = gaze_filter_x(p_projected.x);
			//p_projected.y = gaze_filter_y(p_projected.y);

			lsl_out_eye.push_sample(eye_data);
		}

		
		/*
		// uncomment this to simulate gaze using the computer mouse
		//p_projected = Point2f(mx, my);

		timer1.tock();

		//* // old code for vertically synchronized rendering using libSDL 
		// render part
		timer2.tick();
		img_screen_background.copyTo(img_screen);
		draw_speller(true);
		timer2.tock();

		
		
		// draw to screen (vsynced flip) 
		sdl.imshow(img_screen,100,100);
		auto dt = timer0.tock();
		if (dt > 0.025)
		{
			cout << "\nslow frame. dt = " << dt;
		}
		*/

		sg_local.update();
	}


	thread_scenecam.stop();
	thread_eyecam.stop();
	sg.show();

	// todo restore all other windows 

}