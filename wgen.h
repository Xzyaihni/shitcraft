#ifndef WGEN_H
#define WGEN_H

#include <mutex>
#include <map>

#include "noise.h"
#include "types.h"
#include "worldtypes.h"
#include "wblock.h"
#include "cmodel.h"


class world_generator
{
protected:
	struct block_pos
	{
		vec3d<int> pos;
		world_block block;

		block_pos(vec3d<int> pos, world_block block) : pos(pos), block(block) {};
	};

public:
	typedef std::array<world_types::climate_point, world_types::chunk_size*world_types::chunk_size> climate_noise;

	world_generator(const graphics_state graphics);
	
	void seed(unsigned seed);
	
	full_chunk full_chunk_gen(const vec3d<int> position) noexcept;
	world_chunk chunk_gen(const vec3d<int> position);
	world_types::biome get_biome(const float temperature, const float humidity) const noexcept;
	void gen_plants(world_chunk& gen_chunk, const climate_noise& climate_arr) noexcept;
	
	void shared_place(world_chunk& chunk, const vec3d<int> position, const world_block block) noexcept;
	
	void place_in_chunk(const vec3d<int> chunk_pos, const vec3d<int> pos, const world_block block) noexcept;
	void place_in_chunk(const vec3d<int> chunk_pos, const std::vector<block_pos>& blocks) noexcept;

protected:
	std::array<float, world_types::chunk_size*world_types::chunk_size>
	generate_noise(const vec3d<int> pos, const float noise_scale, const float noise_strength) const noexcept;

	climate_noise
	generate_climate(const vec3d<int> pos, const float temperature_scale, const float humidity_scale) const noexcept;

	vec3d<int> get_ground(const world_chunk& check_chunk, const int x, const int z) const noexcept;

	std::mutex _mtx_block_place;
	
	std::map<vec3d<int>, std::vector<block_pos>> _blocks_map;

	noise_generator _noise_gen;

	graphics_state _graphics;

	unsigned _seed = 1;
	
	friend class world_controller;
};

#endif
