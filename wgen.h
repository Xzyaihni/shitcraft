#ifndef WGEN_H
#define WGEN_H

#include <mutex>
#include <map>

#include <glcyan.h>

#include "noise.h"
#include "types.h"
#include "worldtypes.h"
#include "wblock.h"


class world_chunk;

class world_generator
{
protected:
	struct vec_pos
	{
		vec3d<int> chunk_pos;
		world_types::wall_states walls;
		vec3d<int> block_pos;
		world_block block;
	};

public:
	typedef std::array<world_types::climate_point, chunk_size*chunk_size> climate_noise;

	world_generator() {};
	
	void seed(unsigned seed);
	
	std::array<float, chunk_size*chunk_size> generate_noise(vec3d<int> pos, float noise_scale, float noise_strength);
	climate_noise generate_climate(vec3d<int> pos, float temperature_scale, float humidity_scale);
	
	world_chunk chunk_gen(const vec3d<int> position);
	world_types::biome get_biome(float temperature, float humidity);
	void gen_plants(world_chunk& gen_chunk, climate_noise& climate_arr);
	
	vec3d<int> get_ground(world_chunk& check_chunk, int x, int z);
	
	void shared_place(world_chunk& chunk, const vec3d<int> position, const world_block block);
	
	void place_in_chunk(vec3d<int> chunk_pos, world_types::wall_states walls, vec3d<int> block_pos, world_block block);
	void place_in_chunk(std::vector<vec_pos>& blocks);

protected:
	std::mutex _mtx_block_place;
	
	std::vector<vec_pos> _block_place_vec;

	noise_generator _noise_gen;

	unsigned _seed = 1;
	
	friend class world_controller;
};

#endif
