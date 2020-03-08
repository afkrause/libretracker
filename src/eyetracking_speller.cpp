#include "helpers.h"
#include "eyetracking_speller.h"

using namespace std;

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
	
	Pupil_tracking::setup(simd_width, PUPIL_TRACKING_PUREST);

	using namespace cv;
	using namespace EL;


	// TODO: load previous calibration matrix from file

	speller.setup();
	calibration.setup();

	/*
	// setup gradient based pupil capture
	opt = load_parameters(SETTINGS_LPW);
	timm.set_options(opt);

	// prepare gui for gradient based pupil capture
	params = set_params(opt);
	//setup_gui();
	*/

	// GUIs
	
	sg_stream_and_record = Simple_gui(20, 60, 400, 400);
	sg_stream_and_record.add_separator_box("Jitter Filter Settings");
	sg_stream_and_record.add_slider("smoothing", filter_smoothing, 0, 1, 0.01, "adjust the amount of smoothing of the jitter filter (double exponential filter). Larger values reduce jitter, but introduce noticable lag. this lag can partially compensated increasing the predictive value.");
	sg_stream_and_record.add_slider("predictive", filter_predictive, 0, 1, 0.001, "The predictive component can partially comensate the lag introduced by smoothing. Large values can cause overshooting and damped oscillations of the filter.");
	sg_stream_and_record.add_separator_box("Recording and Streaming");
	sg_stream_and_record.add_checkbox("stream data via labstreaming layer", stream_via_LSL, 1, 0, "Stream gaze- and marker data over the local network via labstreaming layer.");
	sg_stream_and_record.add_checkbox("save gaze data", save_gaze_data, 1, 0, "Save gaze data to a text file.");
	sg_stream_and_record.add_checkbox("save scene-camera video", save_scene_cam_video, 1, 0, "Save a video of the scene camera content.");
	sg_stream_and_record.add_checkbox("save eye-camera video", save_eye_cam_video, 1, 0, "Save a video of the scene camera content.");	
	sg_stream_and_record.add_checkbox("show scene-camera during recording", show_scene_cam_during_recording, 1, 0, "Show the scene-camera with gaze cursor during recording. Might slow down recording.");
	sg_stream_and_record.add_checkbox("show eye-camera during recording", show_eye_cam_during_recording, 1, 0, "Show the eye-camera with gaze cursor during recording. Might slow down recording.");
	sg_stream_and_record.add_slider("buffer size:", video_writer_buffer_size, 0, 250, 1, "adjust the buffer size for the video writer. For fast SSD and HDD a buffer size of 25 should be sufficient. If you experience framedrops, you can increase this value, but you will need more free RAM. Ultimately, the storage medium must be fast enough to save the video at the requested frame rate, no matter what the buffer size is set to.");
	sg_stream_and_record.add_button("Start!", [&](){sg_stream_and_record.hide(); sg.hide(); run_multithreaded(); }, 1, 0, "Start recording a mjpeg video."); //hide guis to avoid multithreading problems (especially with changing camera properties )
	sg_stream_and_record.finish();
	sg_stream_and_record.hide();

	
	
	//sg = Simple_gui(min(Fl::w() - 200, 1420), 180, 400, 600);
	sg = Simple_gui(20, 60, 400, 670);
	sg.add_separator_box("1. adjust canvas size and AR-marker tracking:");
	sg.add_slider("canvas width", gui_param_w, 640, 5000, 10, "Change the width of the canvas to fit your monitor size. Make sure that all screen-tracking markers fit into the field of view of the scene camera.");
	sg.add_slider("canvas height", gui_param_h, 480, 3000, 10, "Change the height of the canvas.");
	sg.add_slider("AR marker size", gui_param_marker_size, 40, 400, 10, "Size of the AR markers in pixel. Larger values increase the robustness of marker tracking, but reduce the available screen space for e.g. the speller application.");
	// sg.add_button("use enclosed markers", [&]() {calibration.ar_canvas.setup(true); },1,0,"use enclosed markers. corners jitter less, but markers need to be larger.");
	// sg.add_slider("detection size", calibration.ar_canvas.min_marker_size, 0.005, 0.1, 0.001, "minimum detection size of a marker (in percent of total image area)");
	sg.add_radio_button("no prediction", [&]() { calibration.ar_canvas.prediction_method = Aruco_canvas::NO_MARKER_PREDICTION; }, 3, 0, "No prediction of currently non-trackable / non-visible markers.");
	sg.add_radio_button("mutual offsets", [&]() {calibration.ar_canvas.prediction_method = Aruco_canvas::MARKER_PREDICTION_MUTUAL_OFFSETS; }, 3, 1, "Marker Prediction using mutual offsets. Robust but inaccurate with head rotation and movements.");
	auto button = sg.add_radio_button("edge vectors", [&]() {calibration.ar_canvas.prediction_method = Aruco_canvas::MARKER_PREDICTION_EDGE_VECTORS; }, 3, 2, "Marker Prediction using edge-vectors of visible markers. Robust to head movements- and rotations, but prone to jitter. Using larger markers reduces jitter.");
	button->value(true);

	sg.add_separator_box("2. adjust cameras:");
	sg.add_button("swap cameras", [&]() { auto tmp = scene_camera; scene_camera = eye_camera; eye_camera = tmp; }, 3, 0, "swap eye- and scene camera.");
	sg.add_button("eye-cam", [&]() { eye_cam_controls.setup(eye_camera, 20, 20, 400, 400, "Eye-Camera Controls"); }, 3, 1, "adjust the eye-camera settings.");
	sg.add_button("scene-cam", [&]() { scene_cam_controls.setup(scene_camera, 20, 20, 400, 400, "Scene-Camera Controls"); }, 3, 2, "adjust the scene-camera settings.");

	sg.add_separator_box("3. select Pupil-Tracking algorithm:");
	sg.add_radio_button("Timm's algorithm", [&,s = simd_width]() { Pupil_tracking::setup(s, PUPIL_TRACKING_TIMM); },1,0, "Timms Algorithm is a gradient based algorithm. License: GPL3.");
	sg.add_radio_button("PuRe (for research only!)", [&, s = simd_width]() {Pupil_tracking::setup(s, PUPIL_TRACKING_PURE); }, 1, 0, "PuRe is a high accuracy- and performance algorithm from the university of t�bingen. License: research only! You are not allowed to use this algorithm and its code for commercial applications.");
	button = sg.add_radio_button("PuReST (for research only!)", [&, s = simd_width]() {Pupil_tracking::setup(s, PUPIL_TRACKING_PUREST); }, 1, 0, "PuReST is a high accuracy - and performance algorithm from the university of t�bingen.License: research only!You are not allowed to use this algorithm and its code for commercial applications.");
	button->value(true);
	sg.add_button("adjust settings", [&]() { pupil_tracker->show_gui(); }, 1, 0);


	/*
	// TODO TODO TODO !!
	sg.add_separator_box("camera calibration (not implemented yet):");
	sg.add_button("scene camera", [&]() { state = STATE_CALIBRATION_SCENE_CAM; }, 2, 0);
	sg.add_button("eye camera", [&]() { state = STATE_CALIBRATION_EYE_CAM; }, 3, 1);
	sg.add_button("save", [&]() {}, 3, 2);
	*/
	
	sg.add_separator_box("4. calibrate the eyetracker:");
	// TODO sg.add_slider("n poly features", []() {}, 4, 4, 10, "");
	sg.add_button("5 point",	[&]() { grab_focus("screen"); calibration.setup(5); state = STATE_CALIBRATION; }, 3, 0, "perform a 5-point calibration.");
	sg.add_button("9 point",	[&]() { grab_focus("screen"); calibration.setup(9); state = STATE_CALIBRATION; }, 3, 1, "perform a 9-point calibration. this takes a bit longer, but usually increases calibration accuracy.");
	sg.add_button("visualize",	[&]() { state = STATE_CALIBRATION; calibration.state = Calibration::STATE_VISUALIZE_CALIBRATION; }, 3, 2, "Visualize the calibration result. Here, you can also try to optimize the polynomial 2d-to-2d mapping by changing the number of polynomial features.");
	
	sg.add_separator_box("5. validate the calibration (optional):");
	double n_validation_points = 5;
	sg.add_slider("validation points", n_validation_points, 4, 20, 1,"Select the number of validation-points.");
	sg.add_slider("randomness [px]", n_validation_points, 0, 50, 1, "Select the random deviation of the generated validation-points from the default validation-point positions.");
	sg.add_button("validate",	[&]() { grab_focus("screen"); calibration.setup_validation(); calibration.state = Calibration::STATE_VALIDATION;  }, 3, 0, "Check the calibration by testing additional points. (optional)");
	sg.add_button("visualize",	[&]() { calibration.state = Calibration::STATE_VISUALIZE_VALIDATION;  }, 3, 1, "Visalizes the results of the validation.");
	sg.add_button("fix offset", [&]() { calibration.fix_offset();  }, 3, 2, "Remove a potential systematic offset found after validation.");

	sg.add_separator_box("6. run modules");
	sg.add_button("observe", [&]() { state = STATE_OBSERVE; }, 2, 0, "Observe gaze relative to the scene camera. You can manually check if the calculated gaze point matches a fixated real world feature.");
	sg.add_button("stream / record", [&]() { sg_stream_and_record.show(); }, 2, 1, "Stream and/or record gaze- and marker data using the labstreaming layer middleware.");
	sg.add_button("run speller", [&]() { grab_focus("screen"); state = STATE_RUN_SPELLER; }, 2, 0, "Run an eyetracking based speller demo. Press the space-bar to acknowledge a fixated letter.");
	sg.add_button("quit", [&]() { sg.hide(); Fl::check(); is_running = false; }, 2, 1);


	sg.finish();
	sg.show();

	//#ifndef HAVE_OPENGL
	//if (flags & CV_WINDOW_OPENGL) CV_ERROR(CV_OpenGlNotSupported, "Library was built without OpenGL support");
	//#else
	//setOpenGlDrawCallback("windowName", glCallback);

	namedWindow("eye_cam");
	namedWindow("screen");// , WINDOW_OPENGL | WINDOW_AUTOSIZE);

	// place the main window to the right side of the options gui
	moveWindow("screen", 450, 10);
	resizeWindow("screen", w, h);
	
	moveWindow("eye_cam", 20, 700);

	// uncomment to simulate gaze using mouse 
	cv::setMouseCallback("screen", mouse_callback, this);

	// code for special calibration marker
	calibration.setup(4);
}



void Eyetracking_speller::draw_instructions()
{
	using namespace cv;
	int mb = calibration.ar_canvas.marker_size;
	// auto screen_center = Point2f(0.5f*img_screen.cols, 0.5f*img_screen.rows);

	calibration.ar_canvas.draw(img_screen, 0, 0, w, h);

	int y = 35;
	auto print_txt = [&](const char * t)
	{
		putText(img_screen, t, Point2i(mb, y), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 170, 0), 2);
		y += 35;
	};

	print_txt("1. Adjust the window-size such that the scene camera");
	print_txt("   can see all AR markers at your current distance.");
	print_txt("2. Calibrate the eyetracker, then validate (optional).");
	print_txt("3. Start the speller or stream the eyetracking data.");

	// blur markers to avoid double detection of a marker inside the preview window
	// TODO.. just blurring doesnt help..
	// calibration.ar_canvas.blur_detected_markers(frame_scene_cam);
	// TODO quick hack: just blur the border with a border width equal to the mean width of previously seen markers

	// visualize marker detection 
	calibration.ar_canvas.draw_detected_markers(frame_scene_cam);


	draw_preview(frame_scene_cam, img_screen);

	imshow("screen", img_screen);

}

// manual validation: observe if the calculated gaze point (after calibration) matches a fixated real world feature.
void Eyetracking_speller::draw_observe()
{
	const int w = img_screen.cols;
	const int h = img_screen.rows;
	using namespace cv;

	//draw_preview(frame_scene_cam, img_screen, -1, -1, -1);
	float scaling = 0, x = 0, y = 0;
	//auto [scaling, x, y] = // requires C++17
	tie(scaling, x, y) = draw_preview(frame_scene_cam, img_screen, -1.0f, -1, -1);
	auto p = scaling * p_calibrated + Point2f(x, y);
	cv::circle(img_screen, p, 12, Scalar(255, 0, 255), 2);
}


void Eyetracking_speller::draw_speller(bool ssvep)
{
	using namespace cv;
	int mb = calibration.ar_canvas.marker_size;

	calibration.ar_canvas.draw(img_screen, 0, 0, w, h);
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
	draw_preview(frame_scene_cam, img_screen, scaling, -1, img_screen.rows - mb);
}

void Eyetracking_speller::draw()
{
	using namespace cv;

	// clear canvas
	img_screen_background.copyTo(img_screen);


	switch (state)
	{
	case STATE_INSTRUCTIONS:		draw_instructions();  break;
	case STATE_CALIBRATION:			calibration.draw(frame_scene_cam, img_screen); break;
	case STATE_RUN_SPELLER:			draw_speller(); break; 
	case STATE_OBSERVE:				draw_observe(); break;
	default: break;

	}

	pupil_tracker->draw(frame_eye_cam);
	imshow("eye_cam", frame_eye_cam);
	imshow("screen", img_screen);
}




// update function for all steps from camera setup, calibration, validation and single threaded speller
// multithreaded eyetracking / speller use a different function
void Eyetracking_speller::update()
{
	using namespace cv;
	using namespace std;
	using namespace chrono;


	// update values that might have been changed via GUI sliders
	gaze_filter_x.set_params(1.0 - filter_smoothing, filter_predictive);
	gaze_filter_y.set_params(1.0 - filter_smoothing, filter_predictive);

	// if the canvas size has changed, recreate the background image
	w = gui_param_w;
	h = gui_param_h;
	if (w_old != w || h_old != h)
	{
		img_screen_background = Mat(h, w, CV_8UC3, Scalar(255, 255, 255)); // white background
		w_old = w; h_old = h;
	}

	// update marker parameters
	calibration.ar_canvas.marker_size = round(gui_param_marker_size);


	

	// read image data from eye- and scene camera
	eye_camera->read(frame_eye_cam);
	scene_camera->read(frame_scene_cam);

		
	// ********************************************************
	// get pupil position
	/*
	cv::cvtColor(frame_eye_cam, frame_eye_gray, cv::COLOR_BGR2GRAY);
	std::tie(pupil_pos, pupil_pos_coarse) = timm.pupil_center(frame_eye_gray);
	
	timm.visualize_frame(frame_eye_gray, pupil_pos, pupil_pos_coarse);
	*/

	timer.tick();
	pupil_tracker->update(frame_eye_cam);
	pupil_pos = pupil_tracker->pupil_center();
	timer.tock();

	// ********************************************************
	// map pupil position to scene camera position using calibrated 2d to 2d mapping
	p_calibrated = calibration.mapping_2d_to_2d(Point2f(pupil_pos.x, pupil_pos.y));

	// ********************************************************
	calibration.ar_canvas.update(frame_scene_cam);
	if (calibration.ar_canvas.valid())
	{
		p_projected = calibration.ar_canvas.transform(p_calibrated);
		p_projected -= calibration.offset;
	}

	// uncomment this to simulate gaze using the computer mouse
	// p_projected = Point2f(mx, my);

	// jitter filter
	p_projected.x = gaze_filter_x(p_projected.x);
	p_projected.y = gaze_filter_y(p_projected.y);


	// ********************************************************
	// event propagation and keyboard handling 
	key_pressed = cv::waitKey(1);


	// ********************************************************
	// call update functions specific to current state / task
	switch (state)
	{
	case STATE_CALIBRATION:
		calibration.update(frame_scene_cam, pupil_pos, key_pressed);
		break;

	case STATE_OBSERVE:
		break;
	case STATE_RUN_SPELLER:
		if (calibration.ar_canvas.valid() && int(' ') == key_pressed)
		{
			// cout << "calibrated point = " << p_calibrated.x << "\t" << p_calibrated.y << endl;
			// use space as acknowledgement for a selected letter
			eye_button_up = true;
		}
		break;
	};

	sg.update();
	sg_stream_and_record.update();
	eye_cam_controls.update();
}



void Eyetracking_speller::run(enum_simd_variant simd_width,  int eye_cam_id, int scene_cam_id)
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
	Timer timer(350,"\nmain loop:");
	while (is_running)
	{
		timer.tick();
		// for gui stuff
		//opt = set_options(params);
		//timm.set_options(opt);
		Pupil_tracking::update();
		//Pupil_tracking::draw();
		update();
		draw();

		if (27 == key_pressed) // VK_ESCAPE
		{
			break;
		}
		timer.tock();
	}
}


#ifdef LSL_ENABLED

#include <lsl_cpp.h>
#include "lt_lsl_protocol.h"
#include <limits>

//#include "deps/s/sdl_opencv.h"

// multithreaded capture and rendering to ensure flicker stimuli are presented with the monitor refresh rate
// separate blocking function with a while loop
void Eyetracking_speller::run_multithreaded()
{
	using namespace cv;
	using namespace lsl;
	using namespace chrono;

	auto fps_scene_cam = scene_camera->get(cv::CAP_PROP_FPS);
	auto fps_eye_cam = eye_camera->get(cv::CAP_PROP_FPS);

	string dts = date_time_str();
	fstream fstream_gaze_data;
	if (save_gaze_data)			{ fstream_gaze_data.open(dts + "_gaze_data.txt", ios::out); }
	if (save_scene_cam_video)	{ scene_cam_video_saver.open(dts + "_scene_camera.avi", dts + "_scene_camera_timestamps.txt", fps_scene_cam, Size(img_screen.cols, img_screen.rows), int(video_writer_buffer_size)); }
	if (save_eye_cam_video)		{ eye_cam_video_saver.open(dts + "_eye_camera.avi", dts + "_eye_camera_timestamps.txt", fps_eye_cam, Size(frame_eye_cam.cols, frame_eye_cam.rows), int(video_writer_buffer_size)); }

	cv::destroyAllWindows();

	bool run = true;
	Simple_gui sg_local(50, 50, 150, 100, "Record and Stream");
	sg_local.add_button("stop recording / streaming", [&]() { run = false;  });
	//sg_local.add_button("quit program", [&]() { exit(EXIT_SUCCESS);} );
	sg_local.finish();

	// Sdl_opencv sdl;


	/*
	//////////////////////////////
	// launch the capture threads
	thread_eyecam.setup(eye_camera, "eyecam");
	thread_scenecam.setup(scene_camera, "scncam");

	cout << "waiting for the first video frames to arrive..";
	// wait for frames to arrive
	while (!(thread_eyecam.new_frame && thread_scenecam.new_frame)) { cout << "."; cv::waitKey(1); this_thread::sleep_for(150ms); }
	cout << "\nthe first frame of both the eye- and scenecam has arrived.\n";
	//////////////////////////////
	*/

	const int LT_N_MARKER_DATA = 1 + 4 * 2;
	vector<double> eye_data(LT_N_EYE_DATA);
	vector<double> marker_data(LT_N_MARKER_DATA);

	// labstreaming layer setup
	cout << "creating labstreaming layer outlet for simulated EEG data..\n";

	stream_outlet lsl_out_eye(stream_info("LT_EYE", "LT_EYE", LT_N_EYE_DATA, fps_eye_cam, cf_double64));
	stream_outlet lsl_out_marker(stream_info("LT_MARKER", "LT_MARKER", LT_N_MARKER_DATA, fps_scene_cam, cf_double64));


	
	Timer timer0(500, "\nframe :"); // mean duration of individual frames. for 60 Hz monitor refresh rate, it should be close to 16.66 ms
	Timer timer1(500, "\nupdate:");
	Timer timer2(500, "\nrender:");

	// the projected gaze point is shared between both threads - hence it needs to be atomic. 
	atomic<double> p_projected_x = 0.0;
	atomic<double> p_projected_y = 0.0;

	auto scn_cam_thread_func = [&]()
	{
		Timer timer(500, "\nscene cam :");
		auto time_start = chrono::high_resolution_clock::now();
		while (run)
		{
			timer.tick();
			scene_camera->read(frame_scene_cam);
			double timestamp = duration_cast<duration<double>>(high_resolution_clock::now() - time_start).count();
			
			//thread_scenecam.get_frame(frame_scene_cam);
			//thread_scenecam.new_frame = false;

			// for some strange reasons, the frame can still be empty.. 
			if (!frame_scene_cam.empty())
			{
				calibration.ar_canvas.update(frame_scene_cam);

				p_projected = calibration.ar_canvas.transform(p_calibrated, calibration.ar_canvas.screen_plane_external);
				p_projected -= calibration.offset;
				p_projected_x = p_projected.x;
				p_projected_y = p_projected.y;


				// send marker data (helpful in the client for visualizing marker positions
				marker_data[0] = timestamp;
				for (int i = 0; i < calibration.ar_canvas.image_plane.size(); i++)
				{
					marker_data[1 + 2 * i + 0] = double(calibration.ar_canvas.image_plane[i].x) / frame_scene_cam.cols;
					marker_data[1 + 2 * i + 1] = double(calibration.ar_canvas.image_plane[i].y) / frame_scene_cam.rows;
				}
				lsl_out_marker.push_sample(marker_data);

				// these potentially slow operations might slow down / cause framedrops a high-refresh rate eye camera streaming/recording
				if (save_scene_cam_video)
				{
					img_screen_background.copyTo(img_screen); // clear canvas
					draw_observe();
					scene_cam_video_saver.write(img_screen, timestamp);
				}

				if (show_scene_cam_during_recording)
				{
					if (!save_scene_cam_video) // still need to render if not already rendered in the save_scene_cam_video code block
					{
						img_screen_background.copyTo(img_screen); // clear canvas
						draw_observe();
					}
					imshow("screen", img_screen);
				}
			}
			cv::waitKey(1); // needed or not ?!
			timer.tock();
		}
	};
	
	auto eye_cam_thread_func = [&]()
	{
		Timer timer(500, "\neye cam :");
		auto time_start = chrono::high_resolution_clock::now();
		while (run)
		{
			timer.tick();
			//thread_eyecam.get_frame(frame_eye_cam);
			//thread_eyecam.new_frame = false;
			eye_camera->read(frame_eye_cam);
			double timestamp = duration_cast<duration<double>>(high_resolution_clock::now() - time_start).count();

			if (!frame_eye_cam.empty())
			{
				// ********************************************************
				// get pupil position
				/*
				cv::cvtColor(frame_eye_cam, frame_eye_gray, cv::COLOR_BGR2GRAY);
				std::tie(pupil_pos, pupil_pos_coarse) = timm.pupil_center(frame_eye_gray);
				*/
				pupil_tracker->update(frame_eye_cam);
				pupil_pos = pupil_tracker->pupil_center();

				// ********************************************************
				// map pupil position to scene camera position using calibrated 2d to 2d mapping
				p_calibrated = calibration.mapping_2d_to_2d(Point2f(pupil_pos.x, pupil_pos.y));

				//std::nan
				for (auto& x : eye_data) { x = nan(); }


				eye_data[LT_TIMESTAMP] = timestamp;
				eye_data[LT_PUPIL_X] = double(pupil_pos.x) / frame_eye_cam.cols;
				eye_data[LT_PUPIL_Y] = double(pupil_pos.y) / frame_eye_cam.rows;
				eye_data[LT_GAZE_X] = double(p_calibrated.x) / frame_scene_cam.cols;
				eye_data[LT_GAZE_Y] = double(p_calibrated.y) / frame_scene_cam.rows;


				if (calibration.ar_canvas.valid())
				{

					// jitter filter (updated at eyecam fps)
					eye_data[LT_SCREEN_X] = p_projected_x;
					eye_data[LT_SCREEN_Y] = p_projected_y;
					eye_data[LT_SCREEN_X_FILTERED] = gaze_filter_x(p_projected_x);
					eye_data[LT_SCREEN_Y_FILTERED] = gaze_filter_y(p_projected_y);
				}

				lsl_out_eye.push_sample(eye_data);


				// save eye tracking data to file.
				if (fstream_gaze_data.is_open())
				{
					fstream_gaze_data << timestamp << "\t" <<
						pupil_pos.x << "\t" <<
						pupil_pos.y << "\t" <<
						p_calibrated.x << "\t" <<
						p_calibrated.y << "\t" <<
						p_projected_x << "\t" <<
						p_projected_y << "\t" <<
						"\n";
				}

				if (save_eye_cam_video)
				{
					eye_cam_video_saver.write(frame_eye_cam, timestamp);
				}

				if (show_eye_cam_during_recording)
				{
					pupil_tracker->draw(frame_eye_cam);
					imshow("eye_cam", frame_eye_cam);
				}
			}
			cv::waitKey(1); // needed or not ?!
			timer.tock();
		}
	};

	thread scn_cam_thread(scn_cam_thread_func);
	thread eye_cam_thread(eye_cam_thread_func);
	
	while (run)
	{
		// process events
		//if (sdl.waitKey().sym == SDLK_ESCAPE) { break; }


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
		this_thread::sleep_for(100ms);
	}

	// wait for threads to finish
	scn_cam_thread.join();
	eye_cam_thread.join();


	//thread_scenecam.stop();
	//thread_eyecam.stop();

	scene_cam_video_saver.close();
	eye_cam_video_saver.close();
	fstream_gaze_data.close();

	sg.show();

	// todo restore all other windows 
}

#else
void Eyetracking_speller::run_ssvep()
{
}
#endif
