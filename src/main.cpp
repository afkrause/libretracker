// OpenCL acceleration for timms algorithm 
// requires the corresponding SDK. 
// Nvidia: CUDA SDK, AMD: AMDGPU-PRO or ROCm drivers come bundled with OpenCL libs

// libraries + paths (specific for my setup, adjust to your own paths)
#ifdef OPENCL_ENABLED
#pragma comment(lib, "opencl_cu10/lib/x64/opencl.lib")
#endif

#ifdef _DEBUG
#pragma comment(lib, "opencv42/build/x64/vc15/lib/opencv_world420d.lib")
#pragma comment(lib, "fltk14/bin/lib/Debug/fltkd.lib")
#pragma comment(lib, "fltk14/bin/lib/Debug/fltk_gld.lib")
#else
#pragma comment(lib, "opencv43/build/lib/opencv_core430.lib")
#pragma comment(lib, "opencv43/build/lib/opencv_imgcodecs430.lib")
#pragma comment(lib, "opencv43/build/lib/opencv_imgproc430.lib")
#pragma comment(lib, "opencv43/build/lib/opencv_highgui430.lib")
#pragma comment(lib, "opencv43/build/lib/opencv_videoio430.lib")
#pragma comment(lib, "opencv43/build/lib/opencv_calib3d430.lib")
#pragma comment(lib, "opencv43/build/lib/opencv_video430.lib")




#pragma comment(lib, "fltk14/bin/lib/Release/fltk.lib")
#pragma comment(lib, "fltk14/bin/lib/Release/fltk_gl.lib")
#endif

#ifdef LSL_ENABLED
#pragma comment(lib, "labstreaminglayer/build/install/LSL/lib/liblsl64.lib")
#endif

//#include<SDL2/SDL_main.h>
//#pragma comment(lib, "SDL2/lib/x64/SDL2.lib")
//#pragma comment(lib, "SDL2/lib/x64/SDL2main.lib")


#ifdef _WIN32
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winspool.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#endif

#include "eyetracking.h"

// detects capabilites of the CPU and OS. Helps in selecting the suitable vectorization option.
#ifdef _WIN32
#include "deps/cpu_features/cpu_x86.h"
#include "deps/DeviceEnumerator.h"
#endif
using namespace std;


// hack because openCV has no flexible window handling
int debug_window_pos_x = 10;


void main_menu(enum_simd_variant simd_width)
{ }

/*
// old code.. 
PRINT_MENU:
	cout << "\n=== Menu ===\n";
	cout << "[0] Eye Cam pupil tracking\n";
	cout << "[1] eyetracking speller demo\n";
	cout << "[2] test accuracy on LPW Dataset\n";
	cout << "[3] test accuracy on Swirski Dataset\n";
	cout << "[4] test accuracy on Excuse Dataset\n";
	cout << "[5] run differential evolution\n";
	
	cout << "enter selection:\n";
	int sel = 0; cin >> sel;
	// TODO - selection menu for pupil tracking algorithm
	auto f0 = [&]() {Pupil_tracking p; p.run(simd_width, PUPIL_TRACKING_TIMM); };
	auto f1 = [&]() {Eyetracking p; p.run(simd_width, PUPIL_TRACKING_TIMM); };
	auto f2 = [&]() {Pupil_tracker_timm_tests p; p.run_lpw_test_all(simd_width); };
	auto f3 = [&]() {Pupil_tracker_timm_tests p; p.run_swirski_test(simd_width); };
	auto f4 = [&]() {Pupil_tracker_timm_tests p; p.run_excuse_test(simd_width); };
	auto f5 = [&]() {Pupil_tracker_timm_tests p; p.run_differential_evolution_optim(simd_width); };

	switch (sel)
	{
	case 0: f0(); break;
	case 1: f1(); break;
	case 2: f2(); break;
	case 3: f3(); break;
	case 4: f4(); break;
	case 5: f5(); break;
	default: cerr << "wrong input. please try again:" << endl; goto PRINT_MENU;
	}
}
*/

#include <FL/fl_ask.H>

// converts a string into for example a float value
template <class T> T string2val(const char* s)
{
	stringstream conv;
	conv.str(s);
	T val;
	conv >> val;
	return val;
}



int main(int argc, char* argv[])
{
	try
	{
		cout << "\n=== OpenCV Build Informations ===\n";
		cout << cv::getBuildInformation() << endl;


		/*
		Eyetracking_speller<512> p;
		p.setup();
		p.calibrate();
		return 0;
		//*/

		#ifdef _WIN32
		// first, detect CPU and OS features		
		// disabled for now, because this crashes on Intel Atom x5 .. 
//		cout << "CPU Vendor String: " << cpu_feature_detector::cpu_x86::get_vendor_string() << endl;
//		cout << endl;
		
		cpu_feature_detector::cpu_x86 cpu_features;
		cpu_features.detect_host();
		cpu_features.print();
		#endif


		if (true)
		{

			///////////////////////
			// camera selection and configuration variables
			bool preview_eye_cam = false;
			bool preview_scene_cam = false;
			Camera_control eye_cam_control;
			shared_ptr<Camera> eye_cam;
			eye_cam_control.setup(eye_cam, 460, 50, 400, 400, "Eye Camera Configuration");
			eye_cam_control.hide();
			
			Camera_control scene_cam_control;
			shared_ptr<Camera> scene_cam;
			scene_cam_control.setup(scene_cam, 460, 500, 400, 400, "Scene Camera Configuration");
			scene_cam_control.hide();

			// use graphical gui to select program options
			Simple_gui sg(30, 50, 400, 600, "== Program Settings ==");
			sg.num_columns(1);

			enum_simd_variant simd_width = USE_NO_VEC;
			sg.add_separator_box("Vectorization Level:");
			auto button = sg.add_radio_button("no vectorization", [&]() {simd_width = USE_NO_VEC; });
			auto button_to_set_select = button;
			
			#ifdef _WIN32
			
			if (cpu_features.HW_SSE)
			{
				button = sg.add_radio_button("128bit SSE OR ARM NEON", [&]() {simd_width = USE_VEC128; });
				simd_width = USE_VEC128; button_to_set_select = button;
			}
			
			
			if (cpu_features.HW_AVX && cpu_features.OS_AVX)
			{
				button = sg.add_radio_button("256bit AVX2", [&]() {simd_width = USE_VEC256; });
				simd_width = USE_VEC256; button_to_set_select = button;
			}
			
			if (cpu_features.HW_AVX512_F && cpu_features.OS_AVX512)
			{
				button = sg.add_radio_button("512bit AVX512", [&]() {simd_width = USE_VEC512; });
				simd_width = USE_VEC512; button_to_set_select = button;
			}
			#endif


			#ifdef __arm__
			button = sg.add_radio_button("128bit ARM NEON", [&]() { simd_width = USE_VEC128; });
			simd_width = USE_VEC128;
			button_to_set_select = button;
			#endif

			#ifdef OPENCL_ENABLED
			button = sg.add_radio_button("OpenCL", [&]() {simd_width = USE_OPENCL; });
			simd_width = USE_OPENCL;
			button_to_set_select = button;
			#endif

			button_to_set_select->value(true);


			/////////////////
			// TODO: create another menu to select the OpenCL accelerator (GPU / CPU / special hardware)
			/////////////////


			auto preview_cam = [&](int id, cv::VideoCaptureAPIs backend)
			{
				try
				{
					using namespace cv;
					cv::Mat img;
					const int n_frames = 200;
					auto capture = VideoCapture(id, backend);
					if (capture.isOpened())
					{
						cout << "\n selected or auto-detected video capture backend = " << capture.getBackendName();
						for (int i = 0; i < 200; i++)
						{
							capture.read(img);
							imshow("camera_preview", img);
							if (waitKey(1) == 27) { break; }
						}
					}
					else
					{
						cerr << "\n could not open VideoCapture with id=" << id << " and backend=" << backend;
					}
					capture.release();
					destroyWindow("camera_preview");
				}
				catch (exception & e) { cerr << "\n error previewing camera output: " << e.what(); }
			};

			int eye_cam_id = -1; int scene_cam_id = -1;
			vector<string> str_video_devices;
			for (int i = 0; i < 4; i++)
			{
				str_video_devices.push_back("Camera id:" + to_string(i));
			}

			#ifdef _WIN32
			DeviceEnumerator de;
			// Video Devices
			auto devices = de.getVideoDevicesMap();
			if (devices.size() == 0)
			{
				std::cout << "No video devices detected using Windows Device Enumeration. you can still try to use a device by ID, only.\n";
			}
			else
			{
				str_video_devices.clear();
				for (auto const& device : devices)
				{
					str_video_devices.push_back("id:" + to_string(device.first) + " Name: " + device.second.deviceName);
					std::cout << str_video_devices.back() << std::endl; // Print information about the devices			
				}
			}
			#endif			

			auto create_cam_func = [&](shared_ptr<Camera>& cam, int id, shared_ptr<Camera>& the_other_cam)
			{
				if (the_other_cam && id == the_other_cam->get_index())
				{
					fl_message("the eye- and scene cameras must be different. please change the camera selection, close the other camera if necessary and try again.");
					return;
				}
				bool msg = false;
				if (!cam) { cam = make_shared<Camera>(id); msg = true; }
				if (cam->get_index() != id) { cam->release(); cam = make_shared<Camera>(id); msg = true; }
				if (msg) { cout << "\nCamera object created. id=" << cam->get_index(); }
			};



			sg.add_separator_box("Select and configure the eye-camera:");			
			//backend_selection_func(videocap_backend_eye_cam);
			//sg.add_separator_box("Select the eye-camera:");
			sg.num_columns(1);
			for (int i = 0; i< str_video_devices.size();i++)
			{
				auto b = sg.add_radio_button(str_video_devices[i].c_str(), [&,i]() {eye_cam_id = i; });
				if (eye_cam_id == -1) { eye_cam_id = i; b->value(true); } // preselect
			}
			sg.num_columns(3);
			auto create_eye_cam_func = [&]() {create_cam_func(eye_cam, eye_cam_id, scene_cam); };
			sg.add_button("preview", [&]() {create_eye_cam_func(); });
			sg.add_button("setup", [&]()
			{
				create_eye_cam_func();
				eye_cam_control.set_camera(eye_cam);
				eye_cam_control.show();

			});
			sg.add_button("close", [&]()
				{
					eye_cam_control.hide();
					if (eye_cam)
					{
						eye_cam->release();
						eye_cam = nullptr;
					}
				});


			sg.add_separator_box("Select the scene-camera video-capture backend:");
			//backend_selection_func(videocap_backend_scene_cam);
			//sg.add_separator_box("Select the scene-camera:");			
			sg.num_columns(1);
			for (int i = 0; i < str_video_devices.size(); i++)
			{
				auto b = sg.add_radio_button(str_video_devices[i].c_str(), [&,i]() {scene_cam_id = i; });
				if(i==1) { scene_cam_id = 1; b->value(true); } // preselect
			}
			sg.num_columns(3);
			auto create_scene_cam_func = [&]() {create_cam_func(scene_cam, scene_cam_id, eye_cam); };
			sg.add_button("preview", [&]() {create_scene_cam_func(); });
			sg.add_button("setup", [&]()
			{
					create_scene_cam_func();
					scene_cam_control.set_camera(scene_cam);
					scene_cam_control.show();
			});
			sg.add_button("close camera", [&]()
				{
					scene_cam_control.hide();
					if (scene_cam)
					{
						scene_cam->release();
						scene_cam = nullptr;
					}
				});

			// end camera selection
			///////////////////////
			

			bool is_running = true;
			sg.add_separator_box("Start a Module:");
			sg.num_columns(2);
			sg.add_button("Eyecam Pupil-Tracking", [&]()
			{
				cv::destroyAllWindows();
				create_eye_cam_func();
				sg.hide();
				Fl::check();
				Pupil_tracking p;
				p.run(simd_width, eye_cam);// _id, videocap_backend_eye_cam);
				is_running = false; 
			});
			
			sg.add_button("Eyetracking", [&]()
			{
				cv::destroyAllWindows();
				create_eye_cam_func();
				create_scene_cam_func();
				sg.hide(); 
				Fl::check(); 
				Eyetracking p;
				//cout << "eye cam id, scene cam id: " << eye_cam_id << ", " << scene_cam_id << endl;
				p.run(simd_width, eye_cam, scene_cam);
				is_running = false; 
			});


			sg.finish();
			sg.show();

			cv::Mat eye_img;
			cv::Mat scene_img;
			while (Fl::check() && is_running)
			{
				if (eye_cam)
				{ 
					if (eye_cam->isOpened())
					{
						eye_cam->read(eye_img);
						if (!eye_img.empty()) { cv::imshow("Eye camera preview", eye_img); }
					}
				}
				else { cv::destroyWindow("Eye camera preview"); }

				if (scene_cam)
				{ 
					if (scene_cam->isOpened())
					{
						scene_cam->read(scene_img);
						if (!scene_img.empty()) { cv::imshow("Scene camera preview", scene_img); }
					}
				}
				else { cv::destroyWindow("Scene camera preview"); }

				eye_cam_control.update();
				scene_cam_control.update();

				//Fl::wait();
				//Fl::wait(0.001);
				Fl::check();
				cv::waitKey(1);
			}
		}
		else
		{
			// fallback: use command line gui
		PRINT_MENU:
			cout << "\n=== Menu Vectorization Level ===\n";
			cout << "[0] no vectorization\n";
			#ifdef _WIN32
			cout << "[1] 128bit SSE OR ARM NEON\n";
			cout << "[2] 256bit AVX2\n";
			cout << "[3] 512bit AVX512\n";
			#endif
			#ifdef __arm__
			cout << "[1] 128bit ARM NEON\n";
			#endif
			#ifdef OPENCL_ENABLED
			cout << "[4] OpenCL\n";
			#endif
			cout << "enter selection:\n";
			int sel = 0; cin >> sel;

			switch (sel)
			{
			case 0: main_menu(USE_NO_VEC); break;
			case 1: main_menu(USE_VEC128); break;
			case 2: main_menu(USE_VEC256); break;
			case 3: main_menu(USE_VEC512); break;
			case 4: main_menu(USE_OPENCL); break;
			default: cerr << "wrong input. please try again:" << endl; goto PRINT_MENU;
			}
		}


	}
	catch (exception& e)
	{
		cerr << "Exception caught: " << e.what() << endl;
	}


	return EXIT_SUCCESS;
}