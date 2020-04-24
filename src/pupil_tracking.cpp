#include "helpers.h"
#include "pupil_tracking.h"
#include "deps/s/cv_camera_control.h"

using namespace std;


void Pupil_tracking::setup(enum_simd_variant simd_width, enum_pupil_tracking_variant pupil_tracking_variant)
{

	// initialize with a random image
	using namespace cv;
	frame_eye_cam = Mat(480, 640, CV_8UC3);
	randu(frame_eye_cam, Scalar::all(0), Scalar::all(255));

	switch (pupil_tracking_variant)
	{
	case PUPIL_TRACKING_TIMM:	pupil_tracker = make_shared<Pupil_tracker_timm>(); break;
	case PUPIL_TRACKING_PURE:	pupil_tracker = make_shared<Pupil_tracker_pure>(); break;
	case PUPIL_TRACKING_PUREST: pupil_tracker = make_shared<Pupil_tracker_purest>(); break;
	default: pupil_tracker = make_shared<Pupil_tracker_timm>(); break;
	}

	pupil_tracker->setup(simd_width);
	is_running = true; // set to false to exit while loop
}


void Pupil_tracking::run(enum_simd_variant simd_width, int eye_cam_id)
{
	Camera_control eye_cam_controls;
	auto eye_cam = select_camera("select the eye camera id:", eye_cam_id);

	// test xtal video
	//auto eye_cam = make_shared<Camera>("d:/temp/xtal_eye_videos/2019-09-06_142643_470_b.avi");
	//auto eye_cam = make_shared<Camera>("c:/Users/Frosch/Downloads/eye_200fps_200x200__h642.mp4");



	auto sg = Simple_gui(20, 60, 400, 300);

	sg.num_columns(1);
	sg.add_separator_box("adjust camera:");	
	sg.add_button("eye-camera", [&]() { eye_cam_controls.setup(eye_cam, 20, 20, 400, 400, "Eye-Camera Controls"); }, "adjust the eye-camera settings.");

	sg.add_separator_box("switch Pupil-Tracking algorithm:");
	sg.num_columns(1);
	sg.add_radio_button("Timm's algorithm", [&, s = simd_width]() { setup(s, PUPIL_TRACKING_TIMM); }, "Timms Algorithm is a gradient based algorithm. License: GPL3.");
	sg.add_radio_button("PuRe (for research only!)", [&, s = simd_width]() {setup(s, PUPIL_TRACKING_PURE); }, "PuRe is a high accuracy- and performance algorithm from the university of t�bingen. License: research only! You are not allowed to use this algorithm and its code for commercial applications.");
	auto button = sg.add_radio_button("PuReST (for research only!)", [&, s = simd_width]() {setup(s, PUPIL_TRACKING_PUREST); }, "PuReST is a high accuracy - and performance algorithm from the university of t�bingen.License: research only!You are not allowed to use this algorithm and its code for commercial applications.");
	button->value(true);
	sg.add_button("adjust settings", [&]() { pupil_tracker->show_gui(); });
	sg.finish();
	sg.show();

	setup(simd_width, PUPIL_TRACKING_PUREST);

	// generate a random image
	using namespace cv;
	auto img_rand = Mat(480, 640, CV_8UC3);
	randu(img_rand, Scalar::all(0), Scalar::all(255));

	cv::Mat frame, frame_tmp;
	cv::Mat frame_gray;
	
	cout << "\nCAP_PROP_FOURCC: " << to_fourcc_string(eye_cam->get(CAP_PROP_FOURCC));
	cout << "\nCAP_PROP_CONVERT_RGB: " << eye_cam->get(CAP_PROP_CONVERT_RGB);
	
	Timer timer(100);
	while (is_running)
	{
		// read a frame from the camera
		eye_cam->read(frame);


		if (!frame.empty())
		{
			/* // code for XTAL VR Headset 
			cv::rotate(frame, frame_tmp, ROTATE_180);
			frame = frame_tmp;
			cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
			cv::imshow("eye_gray", frame_gray);
			// */

			pupil_tracker->update(frame);
			pupil_tracker->draw(frame);

			cv::imshow("eye_cam", frame);
		}
		cv::waitKey(1);
		sg.update();
	}
}

