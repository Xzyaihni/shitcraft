#ifndef NOISE_H
#define NOISE_H

class noise_generator
{
public:

	noise_generator();
	noise_generator(unsigned seed);
	
	float noise(float x, float y);
	
private:
	static unsigned fast_random(unsigned seed);
	static unsigned fast_hash(float val);

	float vec_gradient(float x_h, float x_y, float x_p, float y_p);

	static float smoothstep(float val);

	unsigned _s_offset;
};

#endif
