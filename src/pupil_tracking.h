#pragma once

#include "deps/dependencies.h"

#ifdef _WIN32
#include "deps/DeviceEnumerator.h"
#endif

#include "timm_two_stage.h"



class Pupil_tracking
{

protected:

	Timm_two_stage timm;


	std::vector<std::array<float, 4>> timings_vector;
	//cv::RNG rng(12345);

	// toggle a debug window
	void toggle(bool& b)
	{
		if (b) { b = false; }
		else { b = true; }
	}



	// fitness function
	Eigen::VectorXf eval_fitness(Eigen::VectorXf params, const vector<int>& idx, const int n, const vector<cv::Mat>& images_all, const vector<cv::Point2f>& ground_truth, bool visualize, const int subsampling_width = 100);



	enum enum_parameter_settings
	{
		SETTINGS_DEFAULT,
		SETTINGS_LPW,
		SETTINGS_FUTURE1,
		SETTINGS_FUTURE2,
		SETTINGS_FUTURE3,
		SETTINGS_FROM_FILE,
	};

	using options_type = typename Timm_two_stage::options;

	options_type decode_genom(Eigen::VectorXf params);

	options_type load_parameters(enum_parameter_settings s);

	// params helper array for fltk gui
	array<double, 11> set_params(options_type opt);


	// generic opencv camera selection dialog
	shared_ptr<Camera> select_camera(string message = "select video camera nr. (default=0):");


	options_type opt;
	array<double, 11> params;
	array<bool, 4> debug_toggles{ false,false, false,false };
	bool is_running = true; // set to false to exit while loop
	double n_threads = 1;
	
	Simple_gui sg;
	
public:

	//void setup_gui();

	void setup(enum_simd_variant simd_width)
	{
		timm.setup(simd_width);
	}

	void update()
	{
		// TODO !
	}


	template<size_t n> string add_leading_zeros(string s)
	{ auto to_add = clip<size_t>(n - s.length(), 0, n); for (size_t a = 0; a < to_add; a++) { s = "0" + s; }; return s; }


	void setup_gui();

	// capture from the usb webcam 
	void run_webcam(enum_simd_variant simd_width);

	// evaluate the best parameter set over ALL images of the EXCUSE and ELSE dataset
	// run tests on different datasets
	void run_lpw_test_all(enum_simd_variant simd_width);
	void run_swirski_test(enum_simd_variant simd_width);
	void run_excuse_test(enum_simd_variant simd_width);
	void run_differential_evolution_optim(enum_simd_variant simd_width);

};

