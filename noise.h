#ifndef NOISE_H
#define NOISE_H

#include <random>

class NoiseGenerator
{
public:

	NoiseGenerator();
	NoiseGenerator(unsigned seed);
	
	float noise(float x, float y);
	
private:
	float vec_gradient(float xH, float xY, float xP, float yP);

	static float smoothstep(float val);

	unsigned _sOffset = 0;

	std::minstd_rand _randGen;
};

#endif
