#include "pupil_tracking.h"

#include <random>

void Pupil_tracking::setup(enum_simd_variant simd_width)
{
	timm.setup(simd_width);
	is_running = true; // set to false to exit while loop
}


void Pupil_tracking::run(enum_simd_variant simd_width, int eye_cam_id)
{
	setup(simd_width);

	// generate a random image
	using namespace cv;
	auto img_rand = Mat(480, 640, CV_8UC3);
	randu(img_rand, Scalar::all(0), Scalar::all(255));


	bool do_init_windows = true;
	array<bool, 4> debug_toggles_old = debug_toggles;

	cv::Mat frame;
	cv::Mat frame_gray;
	
	auto capture = select_camera("select the eye camera id:", eye_cam_id);


	opt = load_parameters(SETTINGS_LPW);
	params = set_params(opt);

	setup_gui();

	Timer timer(100);
	while (is_running)
	{
		opt = set_options(params);
		timm.set_options(opt);

		n_threads = round(n_threads);
		timm.stage1.n_threads = n_threads;
		timm.stage2.n_threads = n_threads;

		if (debug_toggles_old != debug_toggles)
		{
			debug_toggles_old = debug_toggles;
			do_init_windows = true;
			cout << "reinit windows..\n";
		}
		timm.set_debug_toggles(debug_toggles);

		// initialize windows
		if (do_init_windows)
		{
			cv::destroyAllWindows();
			do_init_windows = false;
		}

		// read a frame from the camera
		capture->read(frame);

		// Apply the classifier to the frame
		if (!frame.empty())
		{
			cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);

			if (opt.blur > 0) { GaussianBlur(frame_gray, frame_gray, cv::Size(opt.blur, opt.blur), 0); }

			timer.tick();
			// auto[pupil_pos, pupil_pos_coarse] = ...  (structured bindings available with C++17)
			cv::Point pupil_pos, pupil_pos_coarse;
			std::tie(pupil_pos, pupil_pos_coarse) = timm.pupil_center(frame_gray);
			//pupil_pos = timm.stage1.pupil_center(frame_gray);
			timer.tock();
			timm.visualize_frame(frame_gray, pupil_pos, pupil_pos_coarse);

		}

		int c = cv::waitKeyEx(10);
		if ((char)c == 'q') { break; }
		if ((char)c == '1') { toggle(debug_toggles[0]); do_init_windows = true; }
		if ((char)c == '2') { toggle(debug_toggles[1]); do_init_windows = true; }
		if ((char)c == '3') { toggle(debug_toggles[2]); do_init_windows = true; }
		if ((char)c == '4') { toggle(debug_toggles[3]); do_init_windows = true; }
		if ((char)c == 'f')
		{
			imwrite("frame.png", frame);
		}
		//if (c != -1) { cout << "key code: " << int(c) << endl; }

		debug_window_pos_x = 20; // reset

		Fl::check();
	}
}

void Pupil_tracking::setup_gui()
{
	sg = Simple_gui(1000, 100, 640, 540);
	sg.add_separator_box("algorithm parameters");
	sg.add_slider("window size", params[0], 10, 300, 1);
	sg.add_slider("gradient coarse", params[1], 0, 255, 1);
	sg.add_slider("gradient local", params[2], 0, 255, 1);
	sg.add_slider("sobel filter ksize coarse", params[3], -1, 7, 1);
	sg.add_slider("sobel filter ksize local", params[4], -1, 7, 1);
	sg.add_slider("postprocess coarse", params[5], 0, 1, 0.01);
	sg.add_slider("postprocess local", params[6], 0, 1, 0.01);
	sg.add_slider("blur coarse", params[7], 0, 30, 1);
	sg.add_slider("blur local", params[8], 0, 30, 1);
	sg.add_slider("initial blur", params[9], 0, 30, 1);
	sg.add_slider("subsampling width", params[10], 10, 85, 5);

	sg.add_separator_box("debug output switches");
	sg.add_checkbox("show gradients", debug_toggles[0], 2, 0);
	sg.add_checkbox("show weighted grad.", debug_toggles[1], 2, 1);
	sg.add_checkbox("show output sum", debug_toggles[2], 2, 0);
	sg.add_checkbox("show postproc. mask", debug_toggles[3], 2, 1);

	sg.add_separator_box("program functionality");
	sg.add_slider("number of threads", n_threads, 1, 16, 1);
	sg.add_button("load defaults", [&]() {opt = load_parameters(SETTINGS_DEFAULT); params = set_params(opt); sg.update_widgets(); }, 3, 0);
	sg.add_button("load LPW settings", [&]() {opt = load_parameters(SETTINGS_LPW); params = set_params(opt); sg.update_widgets(); }, 3, 1);
	sg.add_button("quit", [&]() { sg.hide(); Fl::check(); is_running = false; }, 3, 2);
	sg.finish();
	sg.show();
}


void Pupil_tracking::run_lpw_test_all(enum_simd_variant simd_width)
{
	setup(simd_width);

	using namespace EL;
	using namespace std;

	const int n = 10;    // dimensionality of the problem
	auto param_set = zeros(n);
	// evolution_run_00, last generation
	// param_set << 0.842027, 0.325653, -0.336364, 0.3906, 0.912289, 0.954837, 5.68097e-05, -0.329308, 0.122322, 0.0202567;
	// evolution_run_01, last generation
	// param_set << 0.373361, 2.04845,   2.03933,  4.12211, 0.875497, 0.827602, 0.228724, -1.44102, -0.404099, 1.36295;

	// evolution_run_02, last generation
	//param_set << 0.4888, -2.4859, 7.8003, 0.7273, 0.8912, 0.8169, -2.1479, -0.6381, 2.1311, -10.4530; // best 
	//param_set << 0.4538, 0.7123, 3.8442, 0.5766, 0.8605, 1.0964, -1.1526, -1.2263, 0.8033, -1.1277; // mean 
	//param_set << 0.4466, 1.5513, 4.8263, 0.5755, 0.8611, 0.9782, -1.2012, -0.7066, 0.6820, -1.7584; // median 

	// evolution_run_02, generation 50
	//param_set << 0.4752, -2.0751, -1.7855, 0.4032, 0.8737, 0.9685, 0.2481, -0.5875, 0.5459, 4.9798; // best
	//param_set << 0.3663, -0.4273, -0.3210, 0.6515, 0.8611, 1.0380, -0.2506, -0.3039, 0.3189, 0.8899; // mean 
	//param_set << 0.3697, -0.5936, -0.5643, 0.6328, 0.8638, 0.9050, -0.1898, -0.2449, 0.3988, 0.9054; // median 

	// evolution_run_03
	//param_set << 0.6148, -0.8637, 2.6297, 0.7482, 0.8118, 0.8191, 0.0238, 0.2185, -0.9518, -1.4373; // best, last gen
	//param_set << 0.4793, -0.0066, 0.6587, 0.7503, 0.8567, 0.8438, 0.0230, 0.1118, 0.2524, 0.6526; // median, last gen
	//param_set << 0.6299, -0.0761, 0.4752, 0.5558, 0.7939, 0.9613, 0.2240, 0.1617, 0.0449, 0.3408; // best, generation 30
	//param_set << 0.1512, 0.7533, 0.2214, 0.6330, 0.8325, 0.9528, -0.0745, 0.3211, 0.2837, 0.7139; // best, generation 13

	// evolution_run_04
	//param_set << 0.3825, 2.3019, 0.9136, 0.4437, 0.8829, 1.6972, -2.5671, 0.0978, 4.1379, -5.3829; // gen 49, best

	// evolution_run_05
	//param_set << 0.2571, 1.5829, 0.3368, 0.7742, 0.8637, 0.8864, -0.7547, 0.3968, 0.6074, 2.7195;

	// evolution_run_06
	//param_set << -2.7514, 0.8224, -0.9765, 0.6042, 0.8924, 0.8335, -1.7056, 0.4420, 2.9767, 0.7377;

	// evolution_run_10 longest survivor
	//param_set << 0.7451, -0.1891, -1.4631, 0.4415, 0.8864, 1.0141, 0.6502, 0.1529, 0.5218, 0.8459;

	// evolution_run_12 
	//param_set << 0.3366, 1.9808, 1.2792, 0.3460, 0.8712, 1.9486, 0.0717, 0.1335, 0.4176, -1.0802; // longest survivor
	param_set << 0.3692, 1.8457, -0.6424, 0.3509, 0.8782, 1.5772, -0.4682, 0.1485, 0.3652, 0.7056; // median <-- best, final solution !
	//param_set << 0.3835, 1.7978, -0.5531, 0.3584, 0.8785, 1.6041, -0.6937, 0.1523, 0.3551, 0.7801; // mean
	//param_set << 0.4001, 1.8485, -2.0000, 0.3535, 0.8718, 1.7392, -0.2811, 0.1492, 0.3259, 0.7056; // best fitness val (not really meaningful in competition mode)
	if (false) // load best solution from file
	{
		fstream f; f.open("best_solutions.txt", ios::in);
		char buff[256];
		int generation_idx = -1;
		cout << "load the best solution. enter generation number (start index 1) or enter -1 for last generation: "; cin >> generation_idx; cout << "\n";
		if (f.is_open())
		{
			int k = 1;
			while ((k == -1 || k <= generation_idx) && !f.eof() && f.getline(buff, 256)) { k++; }
			//string sbuff = buff;
			std::stringstream s(buff);
			float best_fitness = 0.0f;  s >> best_fitness;
			cout << "best fitness of generation " << generation_idx << " is " << best_fitness << "\n";
			//for (auto&x : param_set) { s>> x; } // works with eigen 3.4
			for (int i = 0; i < param_set.size(); i++) { s >> param_set[i]; }
		}
		else { cerr << "could not open best_solutions.txt\n"; }
	}


	// labeled pupils in the wild - load all file names
	vector<string> fnames;

	//fstream f("c:/datasets/LPW/subselection.txt", ios::in); 
	fstream f("c:/datasets/LPW/all.txt", ios::in);
	while (f.is_open() && !f.eof()) { string fname;  f >> fname; fnames.push_back(fname); }
	cv::Mat frame, frame_gray;


	fstream f2;
	f2.open("images_all_errors_best_paramset.txt", ios::out);// | ios::app);
	int frame_increment = 100;
	cout << "enter frame increment (1..n): "; cin >> frame_increment; cout << "\n";
	//for (int subsampling_width = 20; subsampling_width <= 120; subsampling_width += 10)
	int subsampling_width = 85;
	{
		for (auto fname : fnames)
		{
			// load ground truth positions
			vector<cv::Point2f> ground_truth_pos;
			string fname2 = fname;
			fname2.replace(fname2.size() - 3, 3, "txt");
			fstream f(fname2, ios::in); while (f.is_open() && !f.eof()) { cv::Point2f p; f >> p.x >> p.y; ground_truth_pos.push_back(p); }
			vector<cv::Mat> images;
			vector<cv::Point2f> ground_truth;

			images.reserve(131000 / frame_increment);
			ground_truth.reserve(131000 / frame_increment);

			auto capture = new cv::VideoCapture(fname);
			int k = 0;
			while (capture->read(frame))
			{
				cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
				images.push_back(frame_gray.clone());
				ground_truth.push_back(ground_truth_pos[k]);

				k += frame_increment; // skip frames
				capture->set(cv::CAP_PROP_POS_FRAMES, k);
			}
			delete capture;
			vector<int> rp_img(images.size());
			for (size_t i = 0; i < rp_img.size(); i++) { rp_img[i] = i; }
			auto error = eval_fitness(param_set, rp_img, images.size(), images, ground_truth, false, subsampling_width);
			f2 << error.transpose() << " "; //f2.close(); //f2 << "\n"; f2.close();
			cout << "avi processed: " << fname << " mean error: " << error.mean() << endl;
		}
		f2 << "\n";
		cout << "\n\nsubsampling width=" << subsampling_width << endl;
	}
	f2.close();

	// mearured timings
	f2.open("measured_timings.txt", ios::out);
	for (auto v : timings_vector) { for (auto x : v) { f2 << x << "\t"; }; f2 << "\n"; ; }
	f.close();
}

void Pupil_tracking::run_swirski_test(enum_simd_variant simd_width)
{
	setup(simd_width);

	using namespace EL;
	using namespace std;

	const int n = 10;    // dimensionality of the problem
	auto param_set = zeros(n);

	// evolution_run_12 
	//param_set << 0.3366, 1.9808, 1.2792, 0.3460, 0.8712, 1.9486, 0.0717, 0.1335, 0.4176, -1.0802; // longest survivor
	param_set << 0.3692, 1.8457, -0.6424, 0.3509, 0.8782, 1.5772, -0.4682, 0.1485, 0.3652, 0.7056; // median <-- best, final solution !
	//param_set << 0.3835, 1.7978, -0.5531, 0.3584, 0.8785, 1.6041, -0.6937, 0.1523, 0.3551, 0.7801; // mean
	//param_set << 0.4001, 1.8485, -2.0000, 0.3535, 0.8718, 1.7392, -0.2811, 0.1492, 0.3259, 0.7056; // best fitness val (not really meaningful in competition mode)

	// labeled pupils in the wild - load all file names

	//string path = "g:/andre_daten/datasets/swirski/";
	string path = "c:/datasets/swirski/";

	vector<string> fnames;
	fnames.push_back(path + "p1-left/");
	fnames.push_back(path + "p1-right/");
	fnames.push_back(path + "p2-left/");
	fnames.push_back(path + "p2-right/");

	cv::Mat frame, frame_gray;
	int subsampling_width = 85;

	// stream for data output 
	fstream f2;
	f2.open("images_all_errors_best_paramset.txt", ios::out);// | ios::app);

	for (auto fname : fnames)
	{
		// load ground truth positions
		vector<cv::Point2f> ground_truth_pos;
		vector<cv::Mat> images;

		fstream f(fname + "pupil-ellipses.txt", ios::in);
		while (f.is_open() && !f.eof())
		{
			char ctmp = 0; float ftmp = 0;
			int frame_nr = 0;
			cv::Point2f p;

			f >> frame_nr;
			if (f.eof()) { break; }
			f >> ctmp;// read "|"  
			if (ctmp != '|') { cerr << "error loading swirski data" << endl; return; }
			f >> p.x >> p.y;
			ground_truth_pos.push_back(p);
			f >> ftmp; f >> ftmp; f >> ftmp; // skip other values

			frame = cv::imread(fname + "frames/" + to_string(frame_nr) + "-eye.png");

			cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
			images.push_back(frame_gray.clone());
		}
		f.close();

		vector<int> rp_img(images.size());
		for (size_t i = 0; i < rp_img.size(); i++) { rp_img[i] = i; }
		auto error = eval_fitness(param_set, rp_img, images.size(), images, ground_truth_pos, true, subsampling_width);
		f2 << error.transpose() << " "; //f2.close(); //f2 << "\n"; f2.close();
		cout << "subset processed: " << fname << " mean error: " << error.mean() << endl;

	}
	f2.close();
}


void Pupil_tracking::run_excuse_test(enum_simd_variant simd_width)
{
	setup(simd_width);

	using namespace EL;
	using namespace std;

	const int n = 10;    // dimensionality of the problem
	auto param_set = zeros(n);

	// evolution_run_12 
	//param_set << 0.3366, 1.9808, 1.2792, 0.3460, 0.8712, 1.9486, 0.0717, 0.1335, 0.4176, -1.0802; // longest survivor
	param_set << 0.3692, 1.8457, -0.6424, 0.3509, 0.8782, 1.5772, -0.4682, 0.1485, 0.3652, 0.7056; // median <-- best, final solution !
	//param_set << 0.3835, 1.7978, -0.5531, 0.3584, 0.8785, 1.6041, -0.6937, 0.1523, 0.3551, 0.7801; // mean
	//param_set << 0.4001, 1.8485, -2.0000, 0.3535, 0.8718, 1.7392, -0.2811, 0.1492, 0.3259, 0.7056; // best fitness val (not really meaningful in competition mode)

	const float scale_factor = float(640) / float(384);  // images in this dataset are 384x288 pixel, while params were optimized for 640x480 px

	// labeled pupils in the wild - load all file names

	//string path = "c:/datasets/tuebingen/";
	//string path = "c:/datasets/tuebingen/ExCuSe/";
	//string path = "c:/datasets/tuebingen/ELSe/";
	string path = "c:/datasets/tuebingen/pupilnet/";
	//string path = "g:/andre_daten/datasets/pupilnet/";

	fstream f; f.open(path + "all.txt", ios::in);
	//fstream f; f.open(path + "else.txt", ios::in);
	//fstream f; f.open(path + "excuse.txt", ios::in);
	vector<string> fnames;
	while (!f.eof()) { string tmp; getline(f, tmp); fnames.push_back(tmp); }

	cv::Mat frame0, frame, frame_gray;
	int subsampling_width = 85;

	// stream for data output 
	fstream f2;
	f2.open("images_all_errors_best_paramset.txt", ios::out);// | ios::app);

	for (auto fname : fnames)
	{
		string subpath = path + string(fname.begin(), fname.end() - 4) + "/";
		string fname_datafile = path + fname;
		// load ground truth positions
		vector<cv::Point2f> ground_truth_pos;
		vector<cv::Mat> images;

		fstream f(fname_datafile, ios::in);
		string tmp_header; getline(f, tmp_header); // skip header
		while (!f.eof())
		{
			int tmp = 0, frame_nr = 0;
			cv::Point2f p;

			f >> tmp;
			if (f.eof()) { break; }
			f >> frame_nr;
			string fname_img = add_leading_zeros<10>(to_string(frame_nr)) + ".png"; // 0000002871.png				
			f >> p.x >> p.y;
			p = 0.5f * p; // for some unknown reason, values in the data file are 2x the real pixel position
			p.y = 287 - p.y; // and origin is in the lower left area, but with other datasets (e.g. LPW, swirski), the origin is in the upper left corner
			p = scale_factor * p;
			ground_truth_pos.push_back(p);

			frame0 = cv::imread(subpath + fname_img);
			cv::resize(frame0, frame, cv::Size(640, 480));

			cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
			images.push_back(frame_gray.clone());
		}
		f.close();

		vector<int> rp_img(images.size());
		for (size_t i = 0; i < rp_img.size(); i++) { rp_img[i] = i; }
		auto error = eval_fitness(param_set, rp_img, images.size(), images, ground_truth_pos, false, subsampling_width);
		f2 << error.transpose() << " "; //f2.close(); //f2 << "\n"; f2.close();
		cout << "subset processed: " << fname << " mean error: " << error.mean() << endl;

	}
	f2.close();
}


void Pupil_tracking::run_differential_evolution_optim(enum_simd_variant simd_width)
{
	setup(simd_width);

	using namespace EL;
	//////////////////////////////////////////////////
	// labeled pupils in the wild - load all file names
	vector<string> fnames;

	//fstream f("c:/datasets/LPW/subselection.txt", ios::in); 
	fstream f("c:/datasets/LPW/all.txt", ios::in);
	while (f.is_open() && !f.eof()) { string fname;  f >> fname; fnames.push_back(fname); }


	/*
	// filtere die einfachen bilder heraus, welche schon genau genug getrackt werden und konzentriere die suche des evol. alg auf die schwierigen fälle !
	// ergebnis: das ist deutlich schlechter als random subselection !
	vector<int> subidx;
	{
		fstream f; f.open("d:/andre/C/brain_machine/eyetracking/pupil_tracking_gradient_simple/results/subselection_indices.txt", ios::in);
		 while (f.is_open() && !f.eof()){  float tmp=0;  f >> tmp;  subidx.push_back(round(tmp)); }
		 cout << "subindex size: " << subidx.size() << endl;
	}
	*/

	vector<cv::Point2f> ground_truth_pos;
	// preload all images 
	vector<cv::Mat> images_all;
	vector<cv::Point2f> ground_truth;

	int frame_increment = 100;
	cout << "enter frame increment (1..n): "; cin >> frame_increment; cout << "\n";

	images_all.reserve(131000 / frame_increment);
	ground_truth.reserve(131000 / frame_increment);
	//images_all.reserve(subidx.size());
	//ground_truth.reserve(subidx.size());

	int k3 = 0;
	for (auto fname : fnames)
		//for (int i = 0; i < 2; i++)
	{
		//auto fname = fnames[i];

		// load ground truth positions
		ground_truth_pos.clear();
		string fname2 = fname;
		fname2.replace(fname2.size() - 3, 3, "txt");
		fstream f(fname2, ios::in); while (f.is_open() && !f.eof()) { cv::Point2f p; f >> p.x >> p.y; ground_truth_pos.push_back(p); } f.close();

		int k = 0;
		cv::Mat frame, frame_gray;
		auto capture = new cv::VideoCapture(fname);
		while (capture->read(frame))
		{
			cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
			//if (k2 == subidx[k3])
			{
				images_all.push_back(frame_gray.clone());
				ground_truth.push_back(ground_truth_pos[k]);
				k3++;
			}

			// skip every nth frame, otherwise the whole dataset wont fit into memory
			k += frame_increment; // skip frames
			capture->set(cv::CAP_PROP_POS_FRAMES, k);

			//k++; k2++;
		}
		delete capture;

		cout << "avi loaded: " << fname << endl;
	}

	cout << "total number of frames: " << images_all.size() << endl;



	// %%%%%%%%%% differential evolution %%%%%%%%%%%%%

	std::random_device rand_device;
	std::mt19937 rand_mt(rand_device());

	using namespace Eigen;

	//% differential evolution  parameters
	//float im = 0.75f;  // interpolation multiplier
	float im = 0.5f;  // interpolation multiplier
	float cr = 0.5f; // 0.25f; // crossover - ratio
	const int n = 10;    // dimensionality of the problem
	float max_iter = 500;
	//const int population_size = 10 * n; // rule of thumb: population size should be 5-10x the number of dimensions / parameters
	const int population_size = 50;

	// random values from 0.0 .. 1.0
	MatrixXf population = 0.5f * rand(population_size, n).array() + 0.5f;

	array<int, population_size> rp;
	for (int i = 0; i < population_size; i++) { rp[i] = i; }

	vector<int> rp_img(images_all.size());
	for (size_t i = 0; i < rp_img.size(); i++) { rp_img[i] = i; }


	VectorXf fitness = 1000.0f * ones(population_size);
	VectorXf best_solution;

	float im_dyn = im;
	float best_fitness = sqrt(640 * 640 + 480 * 480); // worst case fitness is equal to the diagonal span of the image // 1000.0f;

	// plots for evolutionary algorithm
	//using namespace multiplot;
	//Multiplot mp(25, 25, 1200, 800);
	//mp.grid(MP_LINEAR_GRID, MP_LINEAR_GRID);



	const float err_threshold = 15.0f; //15.0f; // pixel error theshold 
	for (int i = 0; i < max_iter; i++)
	{
		//mp.scaling(MP_FIXED_SCALE, 0, i, 20, 120);

		//best_fitness = 800.0f; // reset best_fitness here to avoid a "lucky" constellation of image subselection

		for (int k = 0; k < population_size; k++)
		{
			std::shuffle(rp.begin(), rp.end(), rand_mt);
			std::shuffle(rp_img.begin(), rp_img.end(), rand_mt);

			auto x = population.row(rp[0]);
			auto a = population.row(rp[1]);
			auto b = population.row(rp[2]);
			auto c = population.row(rp[3]);

			VectorXf y = a + im_dyn * (b - c);

			//cross over
			for (int l = 0; l < n; l++) { if (rnd(0, 1) < cr) { y[l] = x[l]; } }

			// limit value in genome to -2 .. 2
			for (int l = 0; l < y.size(); l++) { y[l] = clip<float>(y[l], -2.0f, 2.0f); }

			// soft cross over
			//VectorXf tmp = 0.5f*VectorXf::Random(n).array() + 0.5f;
			//y = tmp * y + (1.0f - tmp.array()).matrix() * x;

			// evaluate fitness on a subselection of images
			//const int nsub_img = 1000; 
			//const int nsub_img = 60;
			//const int nsub_img = 6000;
			const int nsub_img = 3000;

			// or, in case of already subselected images
			//const int nsub_img = images_all.size();


			auto err1 = eval_fitness(y, rp_img, nsub_img, images_all, ground_truth, false);
			float f_y_1 = err1.mean();

			// additionally, count number of images below pixel error threshold
			int tmp = 0; for (int l = 0; l < err1.size(); l++) { if (err1[l] <= err_threshold) { tmp++; } }
			f_y_1 = 100.0f - 100.0 * tmp / float(nsub_img);// +f_y_1 / 800.0f; // scale such that pixel error is secondary to the frame percentage

			// compare with the exact same random subsample of images (only needed if not all images are evaluated by the fitness function)
			auto err2 = eval_fitness(x, rp_img, nsub_img, images_all, ground_truth, false);
			float f_y_2 = err2.mean();

			// additionally, count number of images below pixel error threshold
			tmp = 0; for (int l = 0; l < err2.size(); l++) { if (err2[l] <= err_threshold) { tmp++; } }
			f_y_2 = 100.0f - 100.0f*tmp / float(nsub_img);// +f_y_2 / 800.0f;

			// check if new solution is better
			if (f_y_1 < f_y_2)
				//if (f_y_1 < fitness(rp[0]))
			{
				population.row(rp[0]) = y;
				fitness(rp[0]) = f_y_1;
			}

			if (f_y_1 < best_fitness)
			{
				best_solution = y;
				best_fitness = f_y_1;
				eval_fitness(y, rp_img, 1, images_all, ground_truth, true); // visualize with a random image
			}
		}


		cout << "generation: " << i << "\tbest fitness: " << best_fitness << endl;
		//cout << "fitness vector: " << fitness << endl;

		{
			fstream f_out("best_solutions.txt", ios::out | ios::app);
			f_out << best_fitness << "\t"; for (int k = 0; k < n; k++) { f_out << best_solution[k] << "\t"; }; f_out << endl;
			f_out.close();
		}

		/*
		mp.trace(0); mp.color3f(0, 1, 0); mp.plot(i, fitness.minCoeff());
		mp.trace(1); mp.color3f(0, 1, 0); mp.plot(i, fitness.maxCoeff());
		mp.trace(2); mp.color3f(1, 1, 0); mp.plot(i, fitness.mean());
		mp.redraw();
		*/

		// save the whole population
		{
			fstream f_out("population.txt", ios::out | ios::app);
			f_out << population << "\n";
			f_out.close();
		}

		// save the population's fitness values
		{
			fstream f_out("population_fitness.txt", ios::out | ios::app);
			for (int k = 0; k < fitness.size(); k++) { f_out << fitness[k] << "\t"; }; f_out << endl;
			f_out.close();
		}


		/*
		fitness_best(i) = min(fitness);

		% dynamic f
		if i > 1
			%{
			if fitness_best(i-1) == fitness_best(i)
				f_dyn = 1.0025*f_dyn;
				stall_count = stall_count + 1;
			else
				f_dyn = 0.98*f_dyn;
			end
			%}
			if fitness_best(i-1) == fitness_best(i)
				stall_count = stall_count + 1;
				f_dyn = 0.95*f_dyn;
			end
		end

		if stall_count > 25
			stall_count = 0;
			f_dyn = f;
			tmp = floor(0.5*population_size);
			population(1:tmp,:) = rnd(tmp, n, range(1), range(2));
			population(1,:) = best_solution;
			fitness(2:tmp) = NaN(1,tmp-1);
			disp('random reset of 50% of current population');
		end

		f_value(i) = f_dyn;
		figure(1);
		plot(i, fitness, 'kx');
		hold on;
		figure(2);
		plot(f_value);
	end % i

		*/

		// not a good idea, very time consuming.. 
		// every 10 generations, evaluate the whole image stack with the currently best genom.
		if (false)// (i %	11 == 10)
		{
			// TOO SLOW .. float overall_error = eval_fitness(best_solution, rp_img, images_all.size(), images_all, ground_truth, false);
			auto overall_error = eval_fitness(best_solution, rp_img, 5000, images_all, ground_truth, false); // approximate with 1000 images
			int tmp = 0; for (int l = 0; l < overall_error.size(); l++) { if (overall_error[l] <= err_threshold) { tmp++; } }
			float frames_below_threshold = 100.0f * tmp / float(overall_error.size());

			float mean_overall_error = overall_error.mean();
			cout << "error over all images: " << mean_overall_error << endl;
			cout << "frames below threshold: " << frames_below_threshold << endl;

			/*
			mp.trace(3);
			mp.color3f(0, 1, 1);
			mp.plot(i, mean_overall_error);
			mp.redraw();
			*/
		}
	}
	char c; cin.get(c);

}


shared_ptr<Camera> Pupil_tracking::select_camera(string message, int id)
{
	// first try the provided id
	shared_ptr<Camera> capture = nullptr;
	if (id != -1)
	{
		capture = make_shared<Camera>(id);
		if (capture->isOpened())
		{
			return capture;
		}
	}

	// if that fails, start interactive selection dialog
	cout << "\n=== Menu Camera Selection ===\n";
	#ifdef _WIN32
	{
		DeviceEnumerator de;
		// Video Devices
		auto devices = de.getVideoDevicesMap();
		if (devices.size() == 0) { std::cout << "no video devices detected.\n"; }

		// Print information about the devices
		for (auto const &device : devices)
		{
			std::cout << "id:" << device.first << " Name: " << device.second.deviceName << std::endl;
		}
	}
	#endif
	while (true)
	{
		cout << message;
		int cam_nr = 0;
		cin >> cam_nr;
		capture = make_shared<Camera>(cam_nr);
		if (capture->isOpened()) { break; }
		cerr << "\ncould not open and initialize camera nr. " << cam_nr << ". please try again!\n";
	}

	// TODO: dialog for webcam resolution entry
	//capture->set(cv::CAP_PROP_FRAME_WIDTH, width);
	//capture->set(cv::CAP_PROP_FRAME_HEIGHT, height);

	return capture;
}



Pupil_tracking::options_type Pupil_tracking::load_parameters(enum_parameter_settings s)
{
	using namespace EL;
	auto param_set = zeros(10);
	options_type opt;

	switch (s)
	{

	case SETTINGS_LPW:
		// optimal parameters for LPW dataset from differential evolution result
		param_set << 0.3692, 1.8457, -0.6424, 0.3509, 0.8782, 1.5772, -0.4682, 0.1485, 0.3652, 0.7056; // median <-- best, final solution !
		opt = decode_genom(param_set);
		break;

	case SETTINGS_DEFAULT:
	default:
		break;
	}

	return opt;
}


Pupil_tracking::params_type Pupil_tracking::set_params(options_type opt)
{
	params_type params;
	params[0] = opt.window_width;
	params[1] = opt.stage1.gradient_threshold;
	params[2] = opt.stage2.gradient_threshold;
	params[3] = opt.stage1.sobel;
	params[4] = opt.stage2.sobel;
	params[5] = opt.stage1.postprocess_threshold;
	params[6] = opt.stage2.postprocess_threshold;
	params[7] = opt.stage1.blur;
	params[8] = opt.stage2.blur;
	params[9] = opt.blur;
	params[10] = opt.stage1.down_scaling_width; // assuming for now that this value is identical for both stages
	return params;
}

Pupil_tracking::options_type Pupil_tracking::set_options(params_type params)
{
	options_type opt;
	// set options from gui parameter array
	opt.window_width = params[0];

	opt.stage1.gradient_threshold = params[1];
	opt.stage2.gradient_threshold = params[2];

	opt.stage1.sobel = to_closest(params[3], sobel_kernel_sizes);
	opt.stage2.sobel = to_closest(params[4], sobel_kernel_sizes);

	opt.stage1.postprocess_threshold = params[5];
	opt.stage2.postprocess_threshold = params[6];

	opt.stage1.blur = to_closest(params[7], blur_kernel_sizes);
	opt.stage2.blur = to_closest(params[8], blur_kernel_sizes);

	opt.blur = to_closest(params[9], blur_kernel_sizes);

	opt.stage1.down_scaling_width = params[10];
	opt.stage2.down_scaling_width = params[10];
	return opt;
}


Eigen::VectorXf Pupil_tracking::eval_fitness(Eigen::VectorXf params, const vector<int>& idx, const int n, const vector<cv::Mat>& images_all, const vector<cv::Point2f>& ground_truth, bool visualize, const int subsampling_width)
{
	using namespace EL;

	cv::Mat frame;
	cv::Mat frame_gray;

	auto opt = decode_genom(params);
	// test how much speed up can be gained by reducing subsampling width
	//opt.stage1.down_scaling_width = 60;
	//opt.stage2.down_scaling_width = 60;
	timm.set_options(opt);


	// estimate fitness over n randomly selected images
	Eigen::VectorXf error = Eigen::VectorXf::Zero(n);

	for (int i = 0; i < n; i++)
	{
		//int idx = round(rnd(0, images_all.size() - 1));

		auto frame_gray = images_all[idx[i]].clone();

		//if (kSmoothFaceFactor > 0.0f)
		if (opt.blur > 0)
		{
			//float sigma = kSmoothFaceFactor * frame_gray.cols;
			//GaussianBlur(frame_gray, frame_gray, cv::Size(0, 0), sigma);
			GaussianBlur(frame_gray, frame_gray, cv::Size(opt.blur, opt.blur), 0, 0);
		}
		cv::Point pupil_pos, pupil_pos_coarse;
		tie(pupil_pos, pupil_pos_coarse) = timm.pupil_center(frame_gray);
		error[i] = cv::norm(ground_truth[idx[i]] - cv::Point2f(pupil_pos));

		// temporary, for speed tests
		timings_vector.push_back(timm.get_timings());
		//*
		if (visualize)
		{
			timm.visualize_frame(frame_gray, pupil_pos, pupil_pos_coarse, &(ground_truth[idx[i]]));
			cv::waitKeyEx(1); // propagate events
		}
		//*/

	}
	return error;
}


Pupil_tracking::options_type Pupil_tracking::decode_genom(Eigen::VectorXf params)
{
	// allowed sobel kernel sizes
	array<float, 5> sobel_kernel_sizes{ -1, 1, 3, 5, 7 };

	options_type opt;

	opt.blur = 2 * clip<int>(30 * params[0], 0, 30) + 1;
	opt.window_width = clip<int>(250 * params[3], 50, 250);

	opt.stage1.sobel = to_closest<5>(4 * params[1], sobel_kernel_sizes);
	opt.stage2.sobel = to_closest<5>(4 * params[2], sobel_kernel_sizes);

	opt.stage1.postprocess_threshold = clip<float>(params[4], 0, 1); // 65; // for videos with dark eye lashes
	opt.stage2.postprocess_threshold = clip<float>(params[5], 0, 1); // 65; // for videos with dark eye lashes

	// set params
	opt.stage1.gradient_threshold = clip<float>(255 * params[6], 0, 255);
	opt.stage2.gradient_threshold = clip<float>(255 * params[7], 0, 255);

	// this must be odd numbers
	opt.stage1.blur = 2 * clip<int>(10 * params[8], 0, 10) + 1;
	opt.stage2.blur = 2 * clip<int>(10 * params[9], 0, 10) + 1;

	// for now, this value is hardcoded 
	opt.stage1.down_scaling_width = 85;
	opt.stage2.down_scaling_width = 85;

	return  opt;
}
