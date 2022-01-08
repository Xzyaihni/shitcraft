#include <random>
#include <cmath>
#include <functional>
#include <iostream>

#include "noise.h"

NoiseGenerator::NoiseGenerator() : _randGen()
{
}

NoiseGenerator::NoiseGenerator(unsigned seed) : _sOffset(seed), _randGen()
{
}

float NoiseGenerator::vec_gradient(float xH, float yH, float xP, float yP)
{
	int yS = reinterpret_cast<int&>(yH);
	_randGen.seed(reinterpret_cast<int&>(xH)^((yS<<6)+(yS>>2)));
	
	float valX = std::generate_canonical<float, 16>(_randGen);
	float valY = std::sqrt(1-valX*valX);
	
	return valX*xP+valY*yP;
}

float NoiseGenerator::smoothstep(float val)
{
	return val*val * (3 - 2*val);
}

float NoiseGenerator::noise(float x, float y)
{	
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

	return 0.5f+noiseVal;
}
