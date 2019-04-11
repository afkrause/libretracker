#pragma once

#define __TIMM_OPENCL__

#include "timm.h"

#include "opencl_kernel.h"


class Timm_opencl : public Timm
{
private:
	Opencl_kernel& kernel;
public:
	
	Timm_opencl(Opencl_kernel& k) : kernel(k)
	{

	}

	cv::Point pupil_center(cv::Mat& eye_img)
	{

		if (simd_width == USE_OPENCL)
		{
			pre_process(eye_img);

			timer2.tick(); 
			#ifdef _WIN32
			_ReadWriteBarrier();
			#endif
			kernel.prepare_data(gradient_x, gradient_y);
			kernel.compute(out_sum);
			cv::multiply(out_sum, weight_float, out);
			#ifdef _WIN32
			_ReadWriteBarrier();
			#endif
			measure_timings[1] = timer2.tock(false);

			return undo_scaling(post_process(), eye_img.cols);
		}
		else
		{
			return Timm::pupil_center(eye_img);
		}
	}
};
