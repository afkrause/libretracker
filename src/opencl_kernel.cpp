#include "opencl_kernel.h"

void Opencl_kernel::setup()
{
	using namespace std;
	// init OpenCl, load and compile kernel 
	try
	{
		// let the user select the OpenCL device
		menu_select_device();

		// Get default device and setup context
		// gpu = compute::system::default_device();
		// gpu = compute::system::find_device("Intel(R) HD Graphics 530");
		// cout << "\nname of selected device: " << gpu.name() << endl;

		context = compute::context(gpu);
		queue = compute::command_queue(context, gpu);

		print_device_info();
		//cout << "press enter to continue..\n"; char c; cin.get(c);

		device_data = compute::vector<float>(context);
		img_out_device1 = compute::image2d(context, 85, 85, compute::image_format(CL_R, CL_FLOAT), compute::image2d::write_only);
		img_out_device2 = compute::image2d(context, 85, 85, compute::image_format(CL_R, CL_FLOAT), compute::image2d::write_only);

		//compute::program filter_program = compute::program::create_with_source(source1b, context);
		compute::program filter_program;

		auto const fname = "assets/kernel_inner_gradients_all.cl";
		cout << "building opencl kernel: " << fname << endl;
		kernel_code = compute::program::create_with_source_file(fname, context);
		//kernel_code.build();
		kernel_code.build("-cl-fast-relaxed-math -cl-finite-math-only -cl-unsafe-math-optimizations -cl-no-signed-zeros -cl-mad-enable");

		cout << "building opencl kernel finished." << endl;
	}
	catch (compute::opencl_error e)
	{
		auto tmp = kernel_code.build_log();
		std::cerr << "Build Error: " << tmp << "\n\n" << e.what() << std::endl; return;
	}


	// create fliter kernel and set arguments
	kernel_opencl = compute::kernel(kernel_code, "kernel_inner_gradients");
}

void Opencl_kernel::prepare_data_tmp(compute::image2d& img_out_device, const cv::Mat& gradient_x, const cv::Mat& gradient_y)
{
	const int w = gradient_x.cols;
	const int h = gradient_x.rows;

	region[0] = w;
	region[1] = h;

	// change output image if necessary
	if (img_out_device.width() != w || img_out_device.height() != h)
	{
		img_out_device = compute::image2d(context, w, h, compute::image_format(CL_R, CL_FLOAT), compute::image2d::write_only);
	}


	//////// prepare gradients and transfer them to the compute device /////////// 
	data.clear();
	auto cols = gradient_x.cols;
	auto gx_p = gradient_x.ptr<float>(0);
	auto gy_p = gradient_y.ptr<float>(0);

	for (size_t y = 0; y < gradient_x.rows; y++)
	{
		for (size_t x = 0; x < gradient_x.cols; x++)
		{
			size_t idx = y * cols + x;
			float gx = gx_p[idx];
			float gy = gy_p[idx];

			// for kernel source 1b
			data.push_back(gx);
			data.push_back(gy);
		}
	}

	const int s = data.size();
	device_data.resize(s);
	compute::copy(data.begin(), data.end(), device_data.begin(), queue);

	// set kernel parameters
	kernel_opencl.set_arg(0, img_out_device);
	kernel_opencl.set_arg(1, device_data); // after a resize, the internal pointer of device_data might have changed, so we need to set the parameter, again
	kernel_opencl.set_arg(2, s);
	kernel_opencl.set_arg(3, w);
	kernel_opencl.set_arg(4, h);

}


#include "deps/s/simple_gui_fltk.h"

void Opencl_kernel::menu_select_device()
{
	using namespace std;
	auto devices_list = compute::system::devices();


	if (true) // if fltk is available, build a selection menu for the opencl device
	{
		Simple_gui sg(50, 50, 400, 100 + 25 * devices_list.size(), "== OpenCL Device Selection ==");
		sg.add_separator_box("Select the OpenCL Device:");
		vector<string> device_names; for (auto& d : devices_list) { device_names.push_back(d.name()); };
		for (int i = 0; i < devices_list.size(); i++)
		{
			auto b = sg.add_radio_button(device_names[i].c_str(), [&, i]() { gpu = devices_list[i]; });
			if (i == 0)
			{
				b->value(true);
				gpu = devices_list[i];
			} // simply select the first entry.. (until a better heuristic is known)
		}
		bool is_running = true;
		sg.add_button("OK", [&]() {is_running = false; });
		sg.finish();
		while (is_running)
		{
			Fl::check();
			Fl::wait(0.1);
		}
		sg.hide();
	}
	else
	{
		while (true)
		{
			cout << "\n=== Menu: Opencl Device Selection ===\n";
			cout << "select an OpenCL device:\n";
			for (size_t i = 0; i < devices_list.size(); i++)
			{
				cout << "[" << i << "] " << devices_list[i].name() << "\n";
			}
			cout << "enter selection:\n";
			int sel = 0; cin >> sel;
			if (sel >= 0 && sel < int(devices_list.size()))
			{
				gpu = devices_list[sel];
				break;
			}
			else
			{
				cerr << "wrong input. please try again:" << endl;
			}
		}
	}

}

void Opencl_kernel::print_device_info()
{
	using namespace std;
	cout << "OpenCL: CL_DEVICE_NAME: " << gpu.get_info<CL_DEVICE_NAME>() << "\n";
	cout << "OpenCL: CL_DEVICE_VENDOR: " << gpu.get_info<CL_DEVICE_VENDOR>() << "\n";
	cout << "OpenCL: CL_DRIVER_VERSION: " << gpu.get_info<CL_DRIVER_VERSION>() << "\n";
	cout << "OpenCL: CL_DEVICE_VERSION: " << gpu.get_info<CL_DEVICE_VERSION>() << "\n";
	cout << "OpenCL: CL_DEVICE_MAX_COMPUTE_UNITS: " << gpu.get_info<CL_DEVICE_MAX_COMPUTE_UNITS>() << "\n";
	cout << "OpenCL: CL_DEVICE_MAX_WORK_GROUP_SIZE: " << gpu.get_info<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << "\n";
	cout << "OpenCL: preferred vector<float> width: " << gpu.get_info<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>() << "\n";
	cout << "OpenCL: preferred vector<double> width: " << gpu.get_info<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>() << "\n";
	cout << "OpenCL: max constant buffer size: " << gpu.get_info<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() << "\n";

	auto tmp = gpu.get_info<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
	cout << "OpenCL: CL_DEVICE_MAX_WORK_ITEM_SIZES: "; for (auto &x : tmp) { cout << x << " "; } cout << "\n";

	string str_extensions = gpu.get_info<CL_DEVICE_EXTENSIONS>();
	std::transform(str_extensions.begin(), str_extensions.end(), str_extensions.begin(), [](char ch) {return ch == ' ' ? '\n' : ch; });
	cout << "OpenCL driver: available extensions:\n" << str_extensions << "\n";
}
