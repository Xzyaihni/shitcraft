#include <cmath>
#include <functional>
#include <iostream>
#include <climits>
#include <cstring>
#include <random>

#include "noise.h"

noise_generator::noise_generator()
{
	std::mt19937 s_gen(1);

	_s_offset = s_gen();
}

noise_generator::noise_generator(unsigned seed)
{
	std::mt19937 s_gen(seed);

	_s_offset = s_gen();
}

unsigned noise_generator::fast_random(unsigned seed)
{
	seed ^= (seed << 13);
	seed ^= (seed >> 7);
	seed ^= (seed << 17);

	return seed;
}

unsigned noise_generator::fast_hash(float val)
{
	static_assert(sizeof(unsigned)==sizeof(val), "type sizes dont match, yikes");
	
	unsigned hashed;
	std::memcpy(&hashed, &val, sizeof(val));
	return hashed & 0xfffffff8;
}

float noise_generator::vec_gradient(float x_h, float y_h, float x_p, float y_p)
{
	int y_s = fast_hash(y_h);

	float val_x = (fast_random((fast_hash(x_h) ^ ((y_s<<6)+(y_s>>2)))) ^ ((_s_offset<<7)+(_s_offset>>3)))/static_cast<float>(UINT_MAX);
	float val_y = std::sqrt(1-val_x*val_x);
	
	return val_x*x_p+val_y*y_p;
}

float noise_generator::smoothstep(float val)
{
	return val*val * (3 - 2*val);
}

float noise_generator::noise(float x, float y)
{	
	const float twice_max_val = std::sqrt(2)/2;
	const float max_val = std::sqrt(2)/4;

	int cell_x = std::floor(x);
	int cell_y = std::floor(y);
	
	float dist_x = x-cell_x;
	float dist_y = y-cell_y;
	
	float smoothX = smoothstep(dist_x);
	float noise_val = std::lerp(std::lerp(
	vec_gradient(cell_x, cell_y, dist_x, dist_y),
	vec_gradient(cell_x+1, cell_y, dist_x-1, dist_y), smoothX),
	std::lerp(
	vec_gradient(cell_x, cell_y+1, dist_x, dist_y-1),
	vec_gradient(cell_x+1, cell_y+1, dist_x-1, dist_y-1), smoothX), smoothstep(dist_y));

	//the range should be between 0 and 1
	return (max_val+noise_val)/twice_max_val;
}
