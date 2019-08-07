#include "pupil_tracking.h"


void Pupil_tracking::setup(enum_simd_variant simd_width, enum_pupil_tracking_variant pupil_tracking_variant)
{
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
	setup(simd_width, PUPIL_TRACKING_PUREST);

	// generate a random image
	using namespace cv;
	auto img_rand = Mat(480, 640, CV_8UC3);
	randu(img_rand, Scalar::all(0), Scalar::all(255));


	cv::Mat frame;
	cv::Mat frame_gray;
	
	auto capture = select_camera("select the eye camera id:", eye_cam_id);


	Timer timer(100);
	while (is_running)
	{

		// read a frame from the camera
		capture->read(frame);

		// Apply the classifier to the frame
		if (!frame.empty())
		{
			pupil_tracker->update(frame);
			pupil_tracker->draw(frame);

			/*
			cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);

			if (opt.blur > 0) { GaussianBlur(frame_gray, frame_gray, cv::Size(opt.blur, opt.blur), 0); }

			timer.tick();
			// auto[pupil_pos, pupil_pos_coarse] = ...  (structured bindings available with C++17)
			cv::Point pupil_pos, pupil_pos_coarse;
			std::tie(pupil_pos, pupil_pos_coarse) = timm.pupil_center(frame_gray);
			//pupil_pos = timm.stage1.pupil_center(frame_gray);
			timer.tock();
			timm.visualize_frame(frame_gray, pupil_pos, pupil_pos_coarse);
			*/

			cv::imshow("eye_cam", frame);
		}
		cv::waitKey(1);
	}
}

