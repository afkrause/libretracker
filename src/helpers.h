#pragma once
// given input value x, set the output value to the closest value to x found in allowed_values
template<size_t s> float to_closest(float x, const array<float, s>& allowed_values)
{
	float tmp = FLT_MAX;
	float y = 0;

	for (auto v : allowed_values)
	{
		if (abs(x - v) < tmp)
		{
			tmp = abs(x - v);
			y = v;
		}
	}

	return y;
}

// clip value x to range min..max
template<class T> inline T clip(T x, const T& min, const T& max)
{
	if (x < min)x = min;
	if (x > max)x = max;
	return x;
}

// produce a random number between [a,b] , a and b inclusive
inline float rnd(float a, float b) { return a + (b - a)*(rand() / float(RAND_MAX)); }
