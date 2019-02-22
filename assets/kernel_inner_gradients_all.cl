__kernel void kernel_inner_gradients(__write_only image2d_t  img_out, const  __constant float * data, const int n, const  int w, const int h)
{
	//#pragma OPENCL EXTENSION cl_khr_fp16 : enable
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

	// Store each work-item's unique row and column
	const int x = get_global_id(0);
	const int y = get_global_id(1);

	// Each work-item iterates around its local area based on the size of the filter
	const int2 coords = { x, y };  // Coordinates for accessing the image

	// center positions
	const float2 c = { x, y };
	float2 p;
	float2 g;
	float2 d;
	float dp = 0.0f;
	float sum = 0.0f;
	size_t idx = 0;

	//d = fast_normalize(d); // uses reciprocal square root internally .. but still slower than manual half_sqrt !! (but why??)
	//d = d * half_rsqrt(dot(d, d)); also not faster ..

	for (int i = 0; i < h; i++)
	{
		p.y = i;
		for (int k = 0; k < w; k++)
		{
			g.x = data[idx++];
			g.y = data[idx++];
			p.x = k; 
			d = p - c;
			d = d * rsqrt(dot(d, d));
			dp = dot(d, g);
			dp = max(0.0f, dp);
			sum += dp*dp;
		}
	}

	//sum = 0.5+0.5*sin( fx * fy / fn);

	//barrier(CLK_GLOBAL_MEM_FENCE);

	//Same channel is copied in all three channels
	//write_imagef(img_out, coords, (float4)(sum, sum, sum, 1.0f));
	write_imagef(img_out, coords, sum);
}