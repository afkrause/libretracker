

// requires the corresponding SDK. Nvidia: CUDA SDK, AMD: AMDGPU-PRO or ROCm drivers come bundled with OpenCL libs
// #define ENABLE_OPENCL_CODE 

// libraries + paths (specific for my setup, adjust to your own paths)
#ifdef _DEBUG
#pragma comment(lib, "opencv40/build/x64/vc15/lib/opencv_world401d.lib")
#pragma comment(lib, "fltk14/bin/lib/Debug/fltkd.lib")
#pragma comment(lib, "fltk14/bin/lib/Debug/fltk_gld.lib")
#else
#pragma comment(lib, "opencv40/build/x64/vc15/lib/opencv_world401.lib")
#pragma comment(lib, "fltk14/bin/lib/Release/fltk.lib")
#pragma comment(lib, "fltk14/bin/lib/Release/fltk_gl.lib")
#endif

#include<SDL2/SDL_main.h>
#pragma comment(lib, "SDL2/lib/x64/SDL2.lib")
#pragma comment(lib, "SDL2/lib/x64/SDL2main.lib")

#pragma comment(lib, "opencl_cu10/lib/x64/opencl.lib")

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

#include "eyetracking_speller.h"

// detects capabilites of the CPU and OS. Helps in selecting the suitable vectorization option.
#ifdef _WIN32
#include "deps/cpu_features/cpu_x86.h"
#include "deps/DeviceEnumerator.h"
#endif


// hack because openCV has no flexible window handling
int debug_window_pos_x = 10;

void main_menu(enum_simd_variant simd_width)
{
	

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

	auto f0 = [&]() {Pupil_tracking p; p.run(simd_width); };
	auto f1 = [&]() {Eyetracking_speller p; p.run(simd_width); };
	auto f2 = [&]() {Pupil_tracking p; p.run_lpw_test_all(simd_width); };
	auto f3 = [&]() {Pupil_tracking p; p.run_swirski_test(simd_width); };
	auto f4 = [&]() {Pupil_tracking p; p.run_excuse_test(simd_width); };
	auto f5 = [&]() {Pupil_tracking p; p.run_differential_evolution_optim(simd_width); };

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
		cout << "CPU Vendor String: " << cpu_feature_detector::cpu_x86::get_vendor_string() << endl;
		cout << endl;
		cpu_feature_detector::cpu_x86 cpu_features;
		cpu_features.detect_host();
		cpu_features.print();
		#endif


		if (true)
		{
			// use graphical gui to select program options
			Simple_gui sg(150, 150, 400, 550, "== Program Settings ==");

			enum_simd_variant simd_width = USE_NO_VEC;
			sg.add_separator_box("Vectorization Level:");
			auto button = sg.add_radio_button("no vectorization", [&]() {simd_width = USE_NO_VEC; });
			auto button_to_set_select = button;
			
			#ifdef _WIN32
			button = sg.add_radio_button("128bit SSE OR ARM NEON", [&]() {simd_width = USE_VEC128; });
			if (cpu_features.HW_SSE) { simd_width = USE_VEC128; button_to_set_select = button; }
			
			button = sg.add_radio_button("256bit AVX2", [&]() {simd_width = USE_VEC256; });
			if (cpu_features.HW_AVX && cpu_features.OS_AVX) { simd_width = USE_VEC256; button_to_set_select = button; }
			
			button = sg.add_radio_button("512bit AVX512", [&]() {simd_width = USE_VEC512; });
			if (cpu_features.HW_AVX512_F && cpu_features.OS_AVX512) { simd_width = USE_VEC512; button_to_set_select = button; }
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


			///////////////////////
			// camera selection
			auto preview_cam = [&](int id)
			{
				try
				{
					using namespace cv;
					auto capture = VideoCapture(id);
					const int n_frames = 200;
					cv::Mat img;
					if (capture.isOpened())
					{

						for (int i = 0; i < 200; i++)
						{
							capture.read(img);
							imshow("camera_preview", img);
							if (cv::waitKey(1) != -1) { break; }
						}
					}
					capture.release();
					cv::destroyWindow("camera_preview");
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

			sg.add_separator_box("Select the eye-camera:");			
			for (int i = 0; i< str_video_devices.size();i++)
			{
				auto b = sg.add_radio_button(str_video_devices[i].c_str(), [&,i]() {eye_cam_id = i; });
				if (eye_cam_id == -1) { eye_cam_id = i; b->value(true); } // preselect
			}
			sg.add_button("preview camera", [&]() {preview_cam(eye_cam_id); }, 1, 0);

			sg.add_separator_box("Select the scene-camera:");
			for (int i = 0; i < str_video_devices.size(); i++)
			{
				auto b = sg.add_radio_button(str_video_devices[i].c_str(), [&,i]() {scene_cam_id = i; });
				if(i==1) { scene_cam_id = 1; b->value(true); } // preselect
			}
			sg.add_button("preview camera", [&]() {preview_cam(scene_cam_id); }, 1, 0);
			// end camera selection
			///////////////////////
			

			bool is_running = true;
			sg.add_separator_box("Start a Module:");
			sg.add_button("Eyecam Pupil-Tracking", [&]() 
			{ 
				sg.hide(); 
				Fl::check(); 
				Pupil_tracking p;
				p.run(simd_width, eye_cam_id); 
				is_running = false; 
			});
			
			sg.add_button("Eyetracking - Speller", [&]()
			{ 
				sg.hide(); 
				Fl::check(); 
				Eyetracking_speller p;
				cout << "eye cam id, scene cam id: " << eye_cam_id << ", " << scene_cam_id << endl;
				p.run(simd_width, eye_cam_id, scene_cam_id); 
				is_running = false; 
			});


			sg.finish();

			while (Fl::check() && is_running)
			{
				Fl::wait();
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


#ifdef USE_OPENCL
#include "opencl_kernel.cpp"
#endif
