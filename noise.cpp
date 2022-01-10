#include <cmath>
#include <functional>
#include <iostream>
#include <climits>

#include "noise.h"

NoiseGenerator::NoiseGenerator() : _sOffset(0), _hasher()
{
}

NoiseGenerator::NoiseGenerator(unsigned seed) : _sOffset(seed), _hasher()
{
}

float NoiseGenerator::fast_random(unsigned seed)
{
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);

	return seed;
}

float NoiseGenerator::vec_gradient(float xH, float yH, float xP, float yP)
{
	int yS = _hasher(yH);

	float valX = fast_random(_hasher(xH)^((yS<<6)+(yS>>2)))/static_cast<float>(UINT_MAX);
	float valY = std::sqrt(1-valX*valX);
	
	return valX*xP+valY*yP;
}

float NoiseGenerator::smoothstep(float val)
{
	return val*val * (3 - 2*val);
}

float NoiseGenerator::noise(float x, float y)
{	
	constexpr float twiceMaxVal = std::sqrt(2)/2;
	constexpr float maxVal = std::sqrt(2)/4;

	int cellX = std::floor(x);
	int cellY = std::floor(y);
	
	float distX = x-cellX;
	float distY = y-cellY;
	
	float smoothX = smoothstep(distX);
	float noiseVal = std::lerp(std::lerp(
	vec_gradient(cellX, cellY, distX, distY),
	vec_gradient(cellX+1, cellY, distX-1, distY), smoothX),
	std::lerp(
	vec_gradient(cellX, cellY+1, distX, distY-1),
	vec_gradient(cellX+1, cellY+1, distX-1, distY-1), smoothX), smoothstep(distY));

	//the range should be between 0 and 1
	return (maxVal+noiseVal)/twiceMaxVal;
}
