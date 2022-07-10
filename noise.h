#ifndef NOISE_H
#define NOISE_H

class noise_generator
{
public:

	noise_generator();
	noise_generator(const unsigned seed);
	
	float noise(const float x, const float y) const noexcept;
	
private:
	static unsigned fast_random(const unsigned seed) noexcept;
	static unsigned fast_hash(const float val) noexcept;

	float vec_gradient(const float x_h, const float x_y, const float x_p, const float y_p) const noexcept;

	static float smoothstep(const float val) noexcept;

	unsigned _s_offset;
};

#endif
