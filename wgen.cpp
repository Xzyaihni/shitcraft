#include <array>
#include <iostream>
#include <random>

#include "wgen.h"
#include "chunk.h"

using namespace yanderegl;
using namespace world_types;

void world_generator::seed(unsigned seed)
{
	_seed = seed;
	_noise_gen = noise_generator(seed);
}

std::array<float, chunk_size*chunk_size> world_generator::generate_noise(vec3d<int> pos, float noise_scale, float noise_strength)
{
	float add_noise = noise_scale/static_cast<float>(chunk_size);

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

std::array<climate_point, chunk_size*chunk_size> world_generator::generate_climate(vec3d<int> pos, float temperature_scale, float humidity_scale)
{
	float add_temperature = temperature_scale/static_cast<float>(chunk_size);
	float add_humidity = humidity_scale/static_cast<float>(chunk_size);

	std::array<climate_point, chunk_size*chunk_size> noise_arr;
	
	int noise_index = 0;
	
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int z = 0; z < chunk_size; ++z, ++noise_index)
		{
			float temperature_noise = _noise_gen.noise(pos.x*temperature_scale+x*add_temperature, pos.z*temperature_scale+z*add_temperature);
			float humidity_noise = _noise_gen.noise(pos.x*humidity_scale+x*add_humidity, pos.z*humidity_scale+z*add_humidity);
		
			noise_arr[noise_index] = climate_point{temperature_noise, humidity_noise};
		}
	}
	
	return noise_arr;
}

world_chunk world_generator::chunk_gen(const vec3d<int> position)
{
	world_chunk chunk(position);

	const float gen_height = 2.25f;
	const float gen_depth = 0;

	if(position.y>gen_height)
		return chunk;
	
	chunk.set_empty(false);
	
	bool overground = position.y>=gen_depth;
	
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
				int maps_index = x*chunk_size+z;
				float c_noise;
			
				biome c_biome = get_biome(climate_arr[maps_index].temperature, climate_arr[maps_index].humidity);
			
				switch(c_biome)
				{
					case biome::hell:
					{
						c_noise = (large_noise_arr[maps_index]/4*medium_noise_arr[maps_index]/4+small_noise_arr[maps_index]/4) * chunk_size;
						break;
					}
				
					default:
					{
						c_noise = (large_noise_arr[maps_index]*medium_noise_arr[maps_index]+small_noise_arr[maps_index]) * chunk_size;
						break;
					}
				}
			
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

biome world_generator::get_biome(float temperature, float humidity)
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

vec3d<int> world_generator::get_ground(world_chunk& check_chunk, int x, int z)
{
	vec3d<int> ground_pos = {0, 0, 0};
					
	for(int i = 0; i < chunk_size; ++i)
	{
		if(check_chunk.block({x, i, z}).transparent())
		{
			ground_pos = {x, i, z};
			break;
		}
	}
	
	return ground_pos;
}

void world_generator::gen_plants(world_chunk& gen_chunk, std::array<climate_point, chunk_size*chunk_size>& climate_arr)
{
	std::mt19937 s_gen(_seed^(gen_chunk.position().x)^(gen_chunk.position().z));
	std::uniform_int_distribution distrib(1, 1000);
	
	std::uniform_int_distribution plant_distrib(1, 8);

	int plants_generated = 0;

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
						++plants_generated;
					
						vec3d<int> ground_pos = get_ground(gen_chunk, x, z);
					
						if(ground_pos.y==0)
							continue;
						
						
						int cactus_height = 2+plant_distrib(s_gen);
							
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
						++plants_generated;
						
						vec3d<int> ground_pos = get_ground(gen_chunk, x, z);
					
						if(ground_pos.y==0)
							continue;
							
						
						int tree_height = plant_distrib(s_gen);
						
						for(int i = 0; i < tree_height; ++i)
						{
							shared_place(gen_chunk, {ground_pos.x, ground_pos.y+i, ground_pos.z}, world_block{block::log});
							
							int nearestSquare = (std::clamp(tree_height-i, 0, 2))*2+1;
							
							int halfSquare = nearestSquare/2;
							
							for(int tx = 0; tx < nearestSquare; ++tx)
							{
								for(int ty = 0; ty < nearestSquare; ++ty)
								{
									if(tx-halfSquare==0 && ty-halfSquare==0)
										continue;
									
									shared_place(gen_chunk, {ground_pos.x+tx-halfSquare, ground_pos.y+i+1, ground_pos.z+ty-halfSquare}, world_block{block::leaf});
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
	
	gen_chunk.set_plants_amount(plants_generated);
}


void world_generator::shared_place(world_chunk& chunk, const vec3d<int> position, const world_block block)
{
	if(position.x<0 || position.y<0 || position.z<0
	|| position.x>(chunk_size-1) || position.y>(chunk_size-1) || position.z>(chunk_size-1))
	{
		//outside of current chunk
		vec3d<int> place_chunk = world_chunk::active_chunk(position);
		
		place_in_chunk(chunk.position()+place_chunk,
		wall_states{position.x%chunk_size==chunk_size-1, position.x%chunk_size==0,
		position.y%chunk_size==chunk_size-1, position.y%chunk_size==0,
		position.z%chunk_size==chunk_size-1, position.z%chunk_size==0},
		(position-place_chunk*chunk_size), block);
	} else
	{
		chunk.block(position) = block;
	}
}

void world_generator::place_in_chunk(vec3d<int> chunk_pos, wall_states walls, vec3d<int> block_pos, world_block block)
{
	std::lock_guard<std::mutex> lock_b(_mtx_block_place);

	_block_place_vec.reserve(1);
	_block_place_vec.emplace_back(chunk_pos, walls, block_pos, block);
}

void world_generator::place_in_chunk(std::vector<vec_pos>& blocks)
{
	std::lock_guard<std::mutex> lock_b(_mtx_block_place);

	_block_place_vec.insert(_block_place_vec.end(), blocks.begin(), blocks.end());
}
