#pragma once
#include "easyfft.h"
//#include "gpufft.h"

inline Array2D<vec2> getInvKernelf(ivec2 size, function<float(float)> func)
{
	Array2D<float> kernel(size, 0);
	auto center = size / 2;
	forxy(kernel) {
		ivec2 p2 = p;
		if (p2.x >= center.x) p2.x -= size.x;
		if (p2.y >= center.y) p2.y -= size.y;
		auto dist = length(vec2(p2));
		if (dist == 0.0f)
			kernel(p) = func(0.5);
		else
			kernel(p) = 1.0 / sq(dist);
	}
	return fft(kernel, FFTW_MEASURE);
}


vec2 compmul(vec2 p, vec2 q) {
	float a = p.x, b = p.y;
	float c = q.x, d = q.y;
	float ac = a * c;
	float bd = b * d;
	vec2 out;
	out.x = ac - bd;
	out.y = (a + b) * (c + d) - ac - bd;
	return out;
}
inline Array2D<float> convolveFft(Array2D<float> Img, Array2D<vec2> kernelf)
{
	auto Imgf = fft(Img, FFTW_MEASURE);
	forxy(Imgf) {
		Imgf(p) = compmul(Imgf(p), kernelf(p));
	}
	auto convolved = ifft(Imgf, FFTW_MEASURE);
	return convolved;
}
