#ifndef NOISE_H
#define NOISE_H

#include <functional>

class NoiseGenerator
{
public:

	NoiseGenerator();
	NoiseGenerator(unsigned seed);
	
	float noise(float x, float y);
	
private:
	static float fast_random(unsigned seed);

	float vec_gradient(float xH, float xY, float xP, float yP);

	static float smoothstep(float val);

	unsigned _sOffset;

	std::hash<float> _hasher;
};

#endif
