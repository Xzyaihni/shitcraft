#include <array>
#include <iostream>
#include <random>
#include <algorithm>

#include "wgen.h"
#include "chunk.h"


using namespace world_types;

world_generator::world_generator(const graphics_state graphics)
: _graphics(graphics),
_seed(time(NULL))
{
}

void world_generator::seed(unsigned seed)
{
	_seed = seed;
	_noise_gen = noise_generator(seed);
}

std::array<float, chunk_size*chunk_size>
world_generator::generate_noise(const vec3d<int> pos, const float noise_scale, const float noise_strength) const noexcept
{
	const float add_noise = noise_scale/static_cast<float>(chunk_size);

	std::array<float, chunk_size*chunk_size> noise_arr;
	
	int noise_index = 0;
	
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int z = 0; z < chunk_size; ++z, ++noise_index)
		{
			noise_arr[noise_index] = _noise_gen.noise(pos.x*noise_scale+x*add_noise, pos.z*noise_scale+z*add_noise)*noise_strength;
		}
	}
	
	return noise_arr;
}

std::array<climate_point, chunk_size*chunk_size>
world_generator::generate_climate(const vec3d<int> pos, const float temperature_scale, const float humidity_scale) const noexcept
{
	const float add_temperature = temperature_scale/static_cast<float>(chunk_size);
	const float add_humidity = humidity_scale/static_cast<float>(chunk_size);

	std::array<climate_point, chunk_size*chunk_size> noise_arr;
	
	int noise_index = 0;
	
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int z = 0; z < chunk_size; ++z, ++noise_index)
		{
			const float temperature_noise = _noise_gen.noise(pos.x*temperature_scale+x*add_temperature, pos.z*temperature_scale+z*add_temperature);
			const float humidity_noise = _noise_gen.noise(pos.x*humidity_scale+x*add_humidity, pos.z*humidity_scale+z*add_humidity);
		
			noise_arr[noise_index] = climate_point{temperature_noise, humidity_noise};
		}
	}
	
	return noise_arr;
}

full_chunk world_generator::full_chunk_gen(const vec3d<int> position) noexcept
{
	return full_chunk(chunk_gen(position), _graphics);
}

world_chunk world_generator::chunk_gen(const vec3d<int> position)
{
	assert(_graphics.camera!=nullptr && _graphics.shader!=nullptr);

	world_chunk chunk(position);

	const float gen_height = 2.25f;
	const float gen_depth = 0;

	if(position.y>gen_height)
		return chunk;
	
	chunk.set_empty(false);
	
	const bool overground = position.y>=gen_depth;
	
	if(!overground)
	{
		chunk.blocks.fill(world_block{block::stone});
		
		chunk.update_states();
		
		return chunk;
	}
	
	std::array<float, chunk_size*chunk_size> small_noise_arr = generate_noise(position, 1.05f, 0.25f);
	std::array<float, chunk_size*chunk_size> medium_noise_arr = generate_noise(position, 0.22f, 1);
	std::array<float, chunk_size*chunk_size> large_noise_arr = generate_noise(position, 0.005f, 2);
	
	std::array<climate_point, chunk_size*chunk_size> climate_arr = generate_climate(position, 0.0136f, 0.0073f);
	
	
	int block_index = 0;
	
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int y = 0; y < chunk_size; ++y)
		{
			for(int z = 0; z < chunk_size; ++z, ++block_index)
			{
				const int maps_index = x*chunk_size+z;
				const float c_temperature = climate_arr[maps_index].temperature;
				const biome c_biome = get_biome(c_temperature, climate_arr[maps_index].humidity);
			
				const float c_noise = (large_noise_arr[maps_index]*(1-c_temperature)*medium_noise_arr[maps_index]+small_noise_arr[maps_index]) * chunk_size;
			
				if(position.y*chunk_size+y<c_noise)
				{
					switch(c_biome)
					{
						case biome::desert:
						{
							chunk.blocks[block_index] = world_block{block::sand};
							break;
						}
						
						case biome::hell:
						{
							chunk.blocks[block_index] = world_block{block::lava};
							break;
						}
						
						default:
						case biome::forest:
						{
							bool c_grass = (position.y*chunk_size+y+1)>=c_noise;
							chunk.blocks[block_index] = world_block{block::dirt, block_info{c_grass}};
							break;
						}
					}
				} else
				{
					chunk.blocks[block_index] = world_block{block::air};
				}
			}
		}
	}
	
	gen_plants(chunk, climate_arr);
	
	chunk.update_states();
	
	return chunk;
}

biome world_generator::get_biome(float temperature, float humidity) const noexcept
{
	if(temperature>0.5f && humidity<0.5f)
	{
		if(temperature>0.65f)
			return biome::hell;
			
		return biome::desert;
	} else
	{
		return biome::forest;
	}
}

vec3d<int>
world_generator::get_ground(const world_chunk& check_chunk, const int x, const int z) const noexcept
{
	for(int i = 0; i < chunk_size; ++i)
	{
		if(check_chunk.block({x, i, z}).transparent())
		{
			return vec3d<int>{x, i, z};
		}
	}

	return vec3d<int>{x, 0, z};
}

void world_generator::gen_plants(world_chunk& gen_chunk, const std::array<climate_point, chunk_size*chunk_size>& climate_arr) noexcept
{
	std::mt19937 s_gen(_seed^(gen_chunk.position().x)^(gen_chunk.position().z));
	std::uniform_int_distribution distrib(1, 1000);
	
	std::uniform_int_distribution plant_distrib(1, 8);

	int point_index = 0;
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int z = 0; z < chunk_size; ++z, ++point_index)
		{
			switch(get_biome(climate_arr[point_index].temperature, climate_arr[point_index].humidity))
			{
				case biome::desert:
				{
					if(distrib(s_gen) < (climate_arr[point_index].humidity-0.10f)*10)
					{
						const vec3d<int> ground_pos = get_ground(gen_chunk, x, z);
					
						if(ground_pos.y==0)
							continue;
						
						
						const int cactus_height = 2+plant_distrib(s_gen);
							
						for(int i = 0; i < cactus_height; ++i)
						{
							shared_place(gen_chunk, {ground_pos.x, ground_pos.y+i, ground_pos.z}, world_block{block::cactus});
						}
					}
					break;
				}
			
				case biome::forest:
				{
					if(distrib(s_gen) < (climate_arr[point_index].humidity-0.45f)*50)
					{
						const vec3d<int> ground_pos = get_ground(gen_chunk, x, z);
					
						if(ground_pos.y==0)
							continue;
							
						
						const int tree_height = plant_distrib(s_gen);
						
						for(int i = 0; i < tree_height; ++i)
						{
							shared_place(gen_chunk, {ground_pos.x, ground_pos.y+i, ground_pos.z}, world_block{block::log});
							
							const int nearest_square = (std::clamp(tree_height-i, 0, 2))*2+1;
							
							const int half_square = nearest_square/2;
							
							for(int tx = 0; tx < nearest_square; ++tx)
							{
								for(int ty = 0; ty < nearest_square; ++ty)
								{
									if(tx-half_square==0 && ty-half_square==0)
										continue;
									
									shared_place(gen_chunk, {ground_pos.x+tx-half_square, ground_pos.y+i+1, ground_pos.z+ty-half_square}, world_block{block::leaf});
								}
							}
						}
						
						shared_place(gen_chunk, {ground_pos.x, ground_pos.y+tree_height, ground_pos.z}, world_block{block::leaf});
					}
					break;
				}
				
				default:
					break;
			}
		}
	}
}


void world_generator::shared_place(world_chunk& chunk, const vec3d<int> position, const world_block block) noexcept
{
	if(position.x<0 || position.y<0 || position.z<0
	|| position.x>(chunk_size-1) || position.y>(chunk_size-1) || position.z>(chunk_size-1))
	{
		//outside of current chunk
		const vec3d<int> place_chunk = world_chunk::active_chunk(position);
		
		place_in_chunk(chunk.position()+place_chunk,
			world_chunk::closest_bound_block(position), block);
	} else
	{
		chunk.block(position) = block;
	}
}

void world_generator::place_in_chunk(const vec3d<int> chunk_pos, const vec3d<int> pos, const world_block block) noexcept
{
	std::lock_guard<std::mutex> lock_b(_mtx_block_place);

	_blocks_map[chunk_pos].emplace_back(pos, block);
}

void world_generator::place_in_chunk(const vec3d<int> chunk_pos, const std::vector<block_pos>& blocks) noexcept
{
	std::lock_guard<std::mutex> lock_b(_mtx_block_place);

	std::vector<block_pos>& c_blocks = _blocks_map[chunk_pos];
	c_blocks.insert(c_blocks.end(), blocks.begin(), blocks.end());
}
