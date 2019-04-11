#pragma once

#include <opencv2/opencv.hpp>

#include <boost/compute.hpp>
#include <boost/compute/interop/opencv/core.hpp>

#include<vector>
#include<iostream>

// separate class for the kernel and device data
// because otherwise, context, queue, kernel and device data would be allocated twice
// this is not possible
namespace compute = boost::compute;
class Opencl_kernel
{
protected:
	
	compute::device gpu;
	compute::context context;
	compute::command_queue queue;
	compute::program kernel_code;
	compute::vector<float> device_data;
	compute::kernel kernel_opencl;

	const size_t origin[2] = { 0, 0 };
	const size_t local_work_size[2] = { 64, 64 }; // not used yet
	size_t region[2] = { 0, 0 };
	
	std::vector<float> data;
	compute::image2d img_out_device1;
	compute::image2d img_out_device2;

	void prepare_data_tmp(compute::image2d& img_out_device, const cv::Mat& gradient_x, const cv::Mat& gradient_y);
	bool use_img_out_device1 = true;

	void menu_select_device();
public:
	void print_device_info();

	// TODO: check the proper allocation of compute::image2d(context, 85, 85, ..
	void setup();

	void prepare_data(const cv::Mat& gradient_x, const cv::Mat& gradient_y)
	{
		if (gradient_x.cols == gradient_x.rows) //this is a bad hack. needs proper implementation
		{
			prepare_data_tmp(img_out_device1, gradient_x, gradient_y);
			use_img_out_device1 = true;
		}
		else
		{
			prepare_data_tmp(img_out_device2, gradient_x, gradient_y);
			use_img_out_device1 = false;
		}

	}



	void compute(cv::Mat img_out)
	{
		queue.enqueue_nd_range_kernel(kernel_opencl, 2, origin, region, nullptr);// local_work_size); // nullptr);//
		queue.finish();

		//cout << "Show GPU image" << endl;
		if (use_img_out_device1)
		{
			compute::opencv_copy_image_to_mat(img_out_device1, img_out, queue);
		}
		else
		{
			compute::opencv_copy_image_to_mat(img_out_device2, img_out, queue);
		}
	}

};
