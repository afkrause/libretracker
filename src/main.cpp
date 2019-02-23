

// requires the corresponding SDK. Nvidia: CUDA SDK, AMD: AMDGPU-PRO or ROCm drivers come bundled with OpenCL libs
#define ENABLE_OPENCL_CODE 

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


#include "pupil_tracking.h"

// include this header to enable eyetracking speller demo. this adds a dependency on the aruco marker tracking library
//#include "eyetracking_speller.h"

// detects capabilites of the CPU and OS. Helps in selecting the suitable vectorization option.
#ifdef _win32
#include "deps/cpu_features/cpu_x86.h"
#endif

// hack because openCV has no flexible window handling
int debug_window_pos_x = 10;

void main_menu(enum_simd_variant simd_width)
{
	#ifndef EYETRACKING_SPELLER_DEMO
	Pupil_tracking p;
	#else
	Eyetracking_speller p;
	#endif

	p.setup(simd_width);

PRINT_MENU:
	cout << "\n=== Menu ===\n";
	cout << "[0] Eye Cam live capture\n";
	cout << "[1] test accuracy over whole dataset\n";
	cout << "[2] run differential evolution\n";
	#ifdef EYETRACKING_SPELLER_DEMO
	cout << "[3] eyetracking speller demo\n";
	#endif
	cout << "enter selection:\n";
	int sel = 0; cin >> sel;
	switch (sel)
	{
	case 0: p.run_webcam(); break;
	case 1: p.run_lpw_test_all(); break;
		//case 1: p.run_swirski_test(); break;
		//case 1: p.run_excuse_test(); break;
	case 2: p.run_differential_evolution_optim(); break;
	#ifdef EYETRACKING_SPELLER_DEMO
	case 3: p.run(); break;
	#endif
	default: cerr << "wrong input. please try again:" << endl; goto PRINT_MENU;
	}
}



int main( int argc, const char** argv )
{
	try
	{

		/*
		Eyetracking_speller<512> p;
		p.setup();
		p.calibrate();
		return 0;
		//*/

		#ifdef _win32
		// first, detect CPU and OS features
		cout << "CPU Vendor String: " << cpu_feature_detector::cpu_x86::get_vendor_string() << endl;
		cout << endl;
		cpu_feature_detector::cpu_x86::print_host();
		#endif

	PRINT_MENU:
		cout << "\n=== Menu Vectorization Level ===\n";
		cout << "[0] no vectorization\n";
		cout << "[1] 128bit SSE OR ARM NEON\n";
		cout << "[2] 256bit AVX2\n";
		cout << "[3] 512bit AVX512\n";
		cout << "[4] OpenCL\n";
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
	catch (exception& e)
	{
		cerr << "Exception caught: " << e.what() << endl;
	}


	return EXIT_SUCCESS;
}
