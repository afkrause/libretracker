

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
#endif

// hack because openCV has no flexible window handling
int debug_window_pos_x = 10;

void main_menu(enum_simd_variant simd_width)
{
	Eyetracking_speller p;

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
	switch (sel)
	{
	case 0: p.run_webcam(simd_width); break;
	case 1: p.run(simd_width); break;
	case 2: p.run_lpw_test_all(simd_width); break;
	case 3: p.run_swirski_test(simd_width); break;
	case 4: p.run_excuse_test(simd_width); break;
	case 5: p.run_differential_evolution_optim(simd_width); break;
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

		#ifdef _WIN32
		// first, detect CPU and OS features
		cout << "CPU Vendor String: " << cpu_feature_detector::cpu_x86::get_vendor_string() << endl;
		cout << endl;
		cpu_feature_detector::cpu_x86::print_host();
		#endif

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
	catch (exception& e)
	{
		cerr << "Exception caught: " << e.what() << endl;
	}


	return EXIT_SUCCESS;
}


#ifdef USE_OPENCL
#include "opencl_kernel.cpp"
#endif