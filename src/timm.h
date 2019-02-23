#pragma once 


#include <array>
#include <vector>
#include <thread>

#include <opencv2/imgproc.hpp>

#include <Eigen/Eigen>

// needed to access Vector Extension Instructions
#ifdef _WIN32
#include <intrin.h>
#else
	#ifdef __arm__
	#include <arm_neon.h> 
	#else
	#include <x86intrin.h>
	#endif
#endif



using namespace std;

#include "deps/dependencies.h"

// ugly hack because OpenCV has no flexible window handling
extern int debug_window_pos_x;

enum enum_simd_variant
{
	USE_NO_VEC=32,
	USE_SSE=128,
	USE_AVX2=256,
	USE_AVX512=512,
	USE_OPENCL=4096
};



// the template parameter simd_width specifies the vector register bit width
// e.g. 512 for AVX512, 256 for AVX2 and 128 for SSE
class Timm
{
protected:

	int simd_width = USE_AVX2;
	// optimised for SIMD: 
	// this vector stores sequential chunks of floats for x,y,gx,gy
	std::vector<float> gradients;
	vector<float> simd_data;

	cv::Mat gradient_x;
	cv::Mat gradient_y;
	cv::Mat img_scaled;
	cv::Mat mags;
	cv::Mat weight;
	cv::Mat weight_float;
	cv::Mat out_sum;
	cv::Mat out;
	cv::Mat floodClone;
	cv::Mat mask;
	cv::Mat debug_img1;
	cv::Mat debug_img2;

	Timer timer1, timer2;

	// storage for threads used by pupil_center	
	std::vector<std::thread> threads;
public:
	int n_threads = 1;
	// for displaying debug images
	string debug_window_name = "Timm's algorithm";
	array<bool, 4> debug_toggles{ false, false, false , false };


	// for timing measurements
	float measure_timings[2] = { 0, 0 };
	
	void setup(enum_simd_variant simd_width_)
	{
		simd_width = simd_width_;
	}

	// Algorithm Parameters 
	struct options
	{
		int down_scaling_width = 85;
		int blur = 5; // gaussan blurr kernel size. must be an odd number > 0
		int sobel = 5; // must be either -1, 1, 3, 5, 7
		float gradient_threshold = 50.0f; //50.0f;
		float postprocess_threshold = 0.97f;
	} opt;

	// estimates the pupil center
	// inputs: eye image, reagion of interest (rio) and an optional window name for debug output
	cv::Point pupil_center(const cv::Mat& eye_img);


protected:

	void pre_process(const cv::Mat& img);
	cv::Point post_process();


	inline cv::Point undo_scaling(cv::Point p, float original_width)
	{
		float s = original_width / float(opt.down_scaling_width);
		int x = round( s * p.x );
		int y = round( s * p.y );
		return cv::Point(x, y);
	}


	float calc_dynamic_threshold(const cv::Mat &mat, float stdDevFactor);

	void imshow_debug(cv::Mat& img, std::string debug_window);

	void prepare_data();

	inline bool inside_mat(cv::Point p, const cv::Mat &mat)
	{
		return p.x >= 0 && p.x < mat.cols && p.y >= 0 && p.y < mat.rows;
	}

	// TODO: optimize !
	// returns a mask
	void floodKillEdges(cv::Mat& mask, cv::Mat &mat);


	///////////////////// original code Tristan Hume, 2012  https://github.com/trishume/eyeLike ///////////////////// 
	void testPossibleCentersFormula(int x, int y, const cv::Mat &weight, double gx, double gy, cv::Mat &out);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	float kernel_orig(float cx, float cy, const cv::Mat& gradientX, const cv::Mat& gradientY);

	#ifndef __arm__
	inline float kernel_op_sse(float cx, float cy, const float* sd)
	{

		// wenn das gut klappt, dann für raspi mal die sse2neon lib anschauen: https://github.com/jratcliff63367/sse2neon

		__m128 zero = _mm_set_ps(0.0f, 0.0f, 0.0f, 0.0f); // this should be faster

		__m128 cx_sse = _mm_set_ps(cx, cx, cx, cx);
		__m128 cy_sse = _mm_set_ps(cy, cy, cy, cy);

		__m128 dx_in = _mm_load_ps(sd);
		__m128 dy_in = _mm_load_ps(sd+4);
		__m128 gx_in = _mm_load_ps(sd+8);
		__m128 gy_in = _mm_load_ps(sd+12);


		// calc the dot product	for the four vec2f		// Emits the Streaming SIMD Extensions 4 (SSE4) instruction dpps. This instruction computes the dot product of single precision floating point values.  // https://msdn.microsoft.com/en-us/library/bb514054(v=vs.120).aspx
		dx_in = _mm_sub_ps(dx_in, cx_sse);
		dy_in = _mm_sub_ps(dy_in, cy_sse);
		__m128 tmp1 = _mm_mul_ps(dx_in, dx_in);
		__m128 tmp2 = _mm_mul_ps(dy_in, dy_in);
		tmp1 = _mm_add_ps(tmp1, tmp2);
		
		// now cals the reciprocal square root
		tmp1 = _mm_rsqrt_ps(tmp1);
		
		// now normalize by multiplying
		dx_in = _mm_mul_ps(dx_in, tmp1);
		dy_in = _mm_mul_ps(dy_in, tmp1);

		// now calc the dot product with the gradient
		tmp1 = _mm_mul_ps(dx_in, gx_in);
		tmp2 = _mm_mul_ps(dy_in, gy_in);
		tmp1 = _mm_add_ps(tmp1, tmp2);

		// now calc the maximum // does this really help ???
		tmp1 = _mm_max_ps(tmp1, zero);

		// multiplication 
		tmp1 = _mm_mul_ps(tmp1, tmp1);

		// and finally, summation of all 4 floats
		// two horizontal adds do the trick:)
		tmp1 = _mm_hadd_ps(tmp1, tmp1);
		tmp1 = _mm_hadd_ps(tmp1, tmp1);

		return _mm_cvtss_f32(tmp1);
	}


	// https://stackoverflow.com/questions/13219146/how-to-sum-m256-horizontally#13222410
	// x = ( x7, x6, x5, x4, x3, x2, x1, x0 )
	inline float sum8(__m256 x)
	{
		// hiQuad = ( x7, x6, x5, x4 )
		const __m128 hiQuad = _mm256_extractf128_ps(x, 1);
		// loQuad = ( x3, x2, x1, x0 )
		const __m128 loQuad = _mm256_castps256_ps128(x);
		// sumQuad = ( x3 + x7, x2 + x6, x1 + x5, x0 + x4 )
		const __m128 sumQuad = _mm_add_ps(loQuad, hiQuad);
		// loDual = ( -, -, x1 + x5, x0 + x4 )
		const __m128 loDual = sumQuad;
		// hiDual = ( -, -, x3 + x7, x2 + x6 )
		const __m128 hiDual = _mm_movehl_ps(sumQuad, sumQuad);
		// sumDual = ( -, -, x1 + x3 + x5 + x7, x0 + x2 + x4 + x6 )
		const __m128 sumDual = _mm_add_ps(loDual, hiDual);
		// lo = ( -, -, -, x0 + x2 + x4 + x6 )
		const __m128 lo = sumDual;
		// hi = ( -, -, -, x1 + x3 + x5 + x7 )
		const __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1);
		// sum = ( -, -, -, x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 )
		const __m128 sum = _mm_add_ss(lo, hi);
		return _mm_cvtss_f32(sum);
	}

	inline float sum8_alt(__m256 x)
	{
		__m256 x2 = _mm256_permute2f128_ps(x, x, 1);
		x = _mm256_add_ps(x, x2);
		x = _mm256_hadd_ps(x, x);
		x = _mm256_hadd_ps(x, x);
		return _mm256_cvtss_f32(x);
	}

	inline float kernel_op_avx2(float cx, float cy, const float* sd)
	{

		//__declspec(align(16)) float dx[4]; // no effect - compiler seems to automatically align code		

		__m256 zero = _mm256_set_ps(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); // this should be faster

		__m256 cx_sse = _mm256_set_ps(cx, cx, cx, cx, cx, cx, cx, cx);
		__m256 cy_sse = _mm256_set_ps(cy, cy, cy, cy, cy, cy, cy, cy);

		__m256 dx_in = _mm256_load_ps(sd);
		__m256 dy_in = _mm256_load_ps(sd+8);
		__m256 gx_in = _mm256_load_ps(sd+16);
		__m256 gy_in = _mm256_load_ps(sd+24);

		// calc the difference vector
		dx_in = _mm256_sub_ps(dx_in, cx_sse);
		dy_in = _mm256_sub_ps(dy_in, cy_sse);

		// calc the dot product	for the eight vec2f		// Emits the Streaming SIMD Extensions 4 (SSE4) instruction dpps. This instruction computes the dot product of single precision floating point values.  // https://msdn.microsoft.com/en-us/library/bb514054(v=vs.120).aspx
		__m256 tmp1 = _mm256_mul_ps(dx_in, dx_in);
		__m256 tmp2 = _mm256_mul_ps(dy_in, dy_in);
		tmp1 = _mm256_add_ps(tmp1, tmp2);

		// now cals the reciprocal square root
		tmp1 = _mm256_rsqrt_ps(tmp1);

		// now normalize by multiplying
		dx_in = _mm256_mul_ps(dx_in, tmp1);
		dy_in = _mm256_mul_ps(dy_in, tmp1);

		// now calc the dot product with the gradient
		tmp1 = _mm256_mul_ps(dx_in, gx_in);
		tmp2 = _mm256_mul_ps(dy_in, gy_in);
		tmp1 = _mm256_add_ps(tmp1, tmp2);

		// now calc the maximum // does this really help ???
		tmp1 = _mm256_max_ps(tmp1, zero);

		// multiplication 
		tmp1 = _mm256_mul_ps(tmp1, tmp1);

		return sum8(tmp1); // a tiny bit faster
		//return sum8_alt(tmp1);
	}

	inline float kernel_op_avx512(float cx, float cy, const float* sd)
	{

		//__declspec(align(16)) float dx[4]; // no effect - compiler seems to automatically align code		

		__m512 zero = _mm512_set_ps(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); // this should be faster

		__m512 cx_sse = _mm512_set_ps(cx, cx, cx, cx, cx, cx, cx, cx, cx, cx, cx, cx, cx, cx, cx, cx);
		__m512 cy_sse = _mm512_set_ps(cy, cy, cy, cy, cy, cy, cy, cy, cy, cy, cy, cy, cy, cy, cy, cy);

		__m512 dx_in = _mm512_load_ps(sd);
		__m512 dy_in = _mm512_load_ps(sd+16);
		__m512 gx_in = _mm512_load_ps(sd+32);
		__m512 gy_in = _mm512_load_ps(sd+48);

		// calc the difference vector
		dx_in = _mm512_sub_ps(dx_in, cx_sse);
		dy_in = _mm512_sub_ps(dy_in, cy_sse);

		// calc the dot product	for the sixteen vec2f		// Emits the Streaming SIMD Extensions 4 (SSE4) instruction dpps. This instruction computes the dot product of single precision floating point values.  // https://msdn.microsoft.com/en-us/library/bb514054(v=vs.120).aspx
		__m512 tmp1 = _mm512_mul_ps(dx_in, dx_in);
		__m512 tmp2 = _mm512_mul_ps(dy_in, dy_in);
		tmp1 = _mm512_add_ps(tmp1, tmp2);

		// now cals the reciprocal square root
		tmp1 = _mm512_rsqrt14_ps(tmp1);

		// now normalize by multiplying
		dx_in = _mm512_mul_ps(dx_in, tmp1);
		dy_in = _mm512_mul_ps(dy_in, tmp1);

		// now calc the dot product with the gradient
		tmp1 = _mm512_mul_ps(dx_in, gx_in);
		tmp2 = _mm512_mul_ps(dy_in, gy_in);
		tmp1 = _mm512_add_ps(tmp1, tmp2);

		// now calc the maximum // does this really help ???
		tmp1 = _mm512_max_ps(tmp1, zero);

		// multiplication 
		tmp1 = _mm512_mul_ps(tmp1, tmp1);

		// summation of all 16 floats
		return _mm512_reduce_add_ps(tmp1);
	}

	#endif
	
	inline float kernel_op(float cx, float cy, const float* sd)	
	{
		
		float x  = sd[0];
		float y  = sd[1];
		float gx = sd[2];
		float gy = sd[3];
		
		float dx = x - cx;
		float dy = y - cy;

		// normalize d
		float magnitude = (dx * dx) + (dy * dy);

		// very time consuming.. ? // with this 28.8 ms
		//magnitude = sqrt(magnitude); 
		//dx = dx / magnitude;
		//dy = dy / magnitude;

		//magnitude = fast_inverse_sqrt_quake(magnitude); // with this: 26 ms.
		//magnitude = fast_inverse_sqrt_around_one(magnitude); // not working .. 

		fast_inverse_sqrt(&magnitude, &magnitude); // MUCH FASTER !
		dx = dx * magnitude;
		dy = dy * magnitude;

		float dotProduct = dx * gx + dy * gy;
		dotProduct = std::max(0.0f, dotProduct);

		return dotProduct * dotProduct;
	}

	float kernel(float cx, float cy, const vector<float>& gradients);
};
