#ifndef NOISE_H
#define NOISE_H

class NoiseGenerator
{
public:

	NoiseGenerator();
	NoiseGenerator(unsigned seed);
	
	float noise(float x, float y);
	
private:
	static unsigned fast_random(unsigned seed);
	static unsigned fast_hash(float val);

	float vec_gradient(float xH, float xY, float xP, float yP);

	static float smoothstep(float val);

	unsigned _sOffset;
};

#endif
