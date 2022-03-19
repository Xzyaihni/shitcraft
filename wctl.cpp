#include <execution>
#include <utility>
#include <tuple>

#include "wctl.h"

using namespace yanderegl;
using namespace world_types;

world_controller::world_controller(yandere_controller* yan_control, GLFWwindow* main_window, character* main_character, yandere_camera* main_camera, yandere_shader_program block_shader) :
_yan_control(yan_control), _main_window(main_window), _main_character(main_character), _main_camera(main_camera), _block_shader(block_shader), _empty(false)
{
}

void world_controller::create_world(unsigned block_textures_id, unsigned seed)
{
	assert(!_empty);
	
	int max_threads = std::thread::hardware_concurrency()-1;
	
	int wall_update_threads = 1;
	max_threads -= wall_update_threads;
	int chunk_load_threads = std::min(1, max_threads);
	
	_chunk_gen_pool = std::make_unique<cgen_pool_type>(chunk_load_threads, &world_controller::chunk_loader, this, vec3d<int>{});
	_wall_update_pool = std::make_unique<wall_pool_type>(wall_update_threads, &world_controller::wall_updater, this);
	
	const core::texture atlas_texture = _yan_control->resources().texture(block_textures_id);
	
	//the block size is 16 pixels both directions
	_atlas = texture_atlas(_yan_control->resources().create_texture(block_textures_id), atlas_texture.width(), atlas_texture.height(), 16, 16);

	_world_gen = std::make_unique<world_generator>();
	
	_world_gen->seed(seed);

	_world_created = true;
}

void world_controller::destroy_block(const vec3d<int> chunk, const vec3d<int> block)
{
	auto iter = world_chunks.find(chunk);
	if(iter!=world_chunks.end())
	{
		iter->second.chunk.block(block).destroy();
		chunk_update_full(iter->second, block);
	}
	
	_wall_update_pool->run();
}

void world_controller::place_block(const vec3d<int> chunk, const vec3d<int> block_pos, const world_block block, const ytype::direction side)
{
	auto iter = world_chunks.find(chunk);
	if(iter!=world_chunks.end())
	{
		const vec3d<int> place_pos = direction_add(block_pos, side, -1);
	
		_world_gen->shared_place(iter->second.chunk, place_pos, block);
	
		update_queued(_queued_blocks);
		
		const vec3d<int> place_chunk_pos = world_chunk::active_chunk(place_pos);
		
		auto place_iter = world_chunks.find(chunk+place_chunk_pos);
		
		if(place_iter!=world_chunks.end())
		{
			chunk_update_full(place_iter->second, place_pos-place_chunk_pos*chunk_size);
		}
	}
	
	_wall_update_pool->run();
}

void world_controller::chunk_update_full(full_chunk& f_chunk, vec3d<int> block_pos)
{
	chunk_update_full(f_chunk, wall_states{block_pos.x==chunk_size-1, block_pos.x==0, block_pos.y==chunk_size-1, block_pos.y==0, block_pos.z==chunk_size-1, block_pos.z==0});
}

void world_controller::chunk_update_full(full_chunk& f_chunk, wall_states walls)
{
	const vec3d<int>& pos = f_chunk.chunk.position();

	f_chunk.model.update_mesh();
	
	if(walls.left)
	{
		auto left_iter = world_chunks.find({pos.x-1, pos.y, pos.z});
		if(left_iter!=world_chunks.end())
			left_iter->second.model.update_mesh();
	}
	
	if(walls.right)
	{
		auto right_iter = world_chunks.find({pos.x+1, pos.y, pos.z});
		if(right_iter!=world_chunks.end())
			right_iter->second.model.update_mesh();
	}
	
	if(walls.down)
	{
		auto down_iter = world_chunks.find({pos.x, pos.y-1, pos.z});
		if(down_iter!=world_chunks.end())
			down_iter->second.model.update_mesh();
	}
	
	if(walls.up)
	{
		auto up_iter = world_chunks.find({pos.x, pos.y+1, pos.z});
		if(up_iter!=world_chunks.end())
			up_iter->second.model.update_mesh();
	}
		
	if(walls.back)
	{
		auto back_iter = world_chunks.find({pos.x, pos.y, pos.z-1}); 	
		if(back_iter!=world_chunks.end())
			back_iter->second.model.update_mesh();
	}
		
	if(walls.forward)
	{
		auto forward_iter = world_chunks.find({pos.x, pos.y, pos.z+1});
		if(forward_iter!=world_chunks.end())
			forward_iter->second.model.update_mesh();
	}
}


void world_controller::wait_threads()
{
	assert(_world_created);
	
	_chunk_gen_pool->exit_threads();
}

void world_controller::chunk_loader(const vec3d<int> pos)
{
	world_chunk chunk = _world_gen->chunk_gen(pos);

	std::lock_guard<std::mutex> lock(_process_chunks_mtx);
	_processing_chunks.emplace_back(std::move(chunk));
}

void world_controller::wall_updater()
{
	for(auto& [key, f_chunk] : world_chunks)
	{
		if(f_chunk.model.walls_full() || f_chunk.chunk.empty())
			continue;
	
		const wall_states c_walls = f_chunk.model.walls();
	
		if(c_walls.right)
		{
			const auto& c_iter = world_chunks.find(vec3d<int>{key.x+1, key.y, key.z});
			if(c_iter!=world_chunks.end())
			{
				f_chunk.model.update_wall(ytype::direction::right, c_iter->second.chunk);
			}
		}
		
		if(c_walls.left)
		{
			const auto& c_iter = world_chunks.find(vec3d<int>{key.x-1, key.y, key.z});
			if(c_iter!=world_chunks.end())
			{
				f_chunk.model.update_wall(ytype::direction::left, c_iter->second.chunk);
			}
		}
		
		if(c_walls.up)
		{
			const auto& c_iter = world_chunks.find(vec3d<int>{key.x, key.y+1, key.z});
			if(c_iter!=world_chunks.end())
			{
				f_chunk.model.update_wall(ytype::direction::up, c_iter->second.chunk);
			}
		}
		
		if(c_walls.down)
		{
			const auto& c_iter = world_chunks.find(vec3d<int>{key.x, key.y-1, key.z});
			if(c_iter!=world_chunks.end())
			{
				f_chunk.model.update_wall(ytype::direction::down, c_iter->second.chunk);
			}
		}
		
		if(c_walls.forward)
		{
			const auto& c_iter = world_chunks.find(vec3d<int>{key.x, key.y, key.z+1});
			if(c_iter!=world_chunks.end())
			{
				f_chunk.model.update_wall(ytype::direction::forward, c_iter->second.chunk);
			}
		}
		
		if(c_walls.back)
		{
			const auto& c_iter = world_chunks.find(vec3d<int>{key.x, key.y, key.z-1});
			if(c_iter!=world_chunks.end())
			{
				f_chunk.model.update_wall(ytype::direction::back, c_iter->second.chunk);
			}
		}
	}
}

float world_controller::chunk_outside(const vec3d<int> pos) const
{
	return vec3d<int>::magnitude(
	vec3d<int>{std::abs(pos.x), std::abs(pos.y), std::abs(pos.z)}
	- vec3d<int>{std::abs(_main_character->active_chunk_pos.x), std::abs(_main_character->active_chunk_pos.y), std::abs(_main_character->active_chunk_pos.z)});
}

void world_controller::range_remove()
{
	//unloads chunks which are outside of a certain range
	for(auto it = world_chunks.begin(); it != world_chunks.end();)
	{
		if(chunk_outside(it->first) > _chunk_radius+1)
		{
			//unload the chunk
			_been_loaded_chunks.erase(it->first);
				
			it = world_chunks.erase(it);
		} else
		{
			++it;
		}
	}
}

void world_controller::add_chunks()
{
	bool update_shared = false;

	bool new_loaded = false;

	{	
		std::lock_guard<std::mutex> lock(_process_chunks_mtx);
		
		new_loaded = !_processing_chunks.empty();
		if(new_loaded)
		{
			for(auto& chunk : _processing_chunks)
			{
				update_shared = update_shared || (chunk.plants_amount()!=0);
			
				world_chunks.emplace(std::piecewise_construct,
				std::forward_as_tuple(chunk.position()),
				std::forward_as_tuple(std::move(chunk), _block_shader, _atlas));
			}
			
			_processing_chunks.clear();
		}
	}
	
	//OPTIMIZE THIS SOMEHOW!!!!!
	if(update_shared)
		update_queued(_queued_blocks);
	
	if(new_loaded)
	{
		//update out of chunk blocks if any chunks were generated
		for(auto it = _queued_blocks.begin(); it!=_queued_blocks.end();)
		{
			auto chunk_iter = world_chunks.find(it->first);
			if(chunk_iter!=world_chunks.end())
			{
				chunk_update_full(chunk_iter->second, it->second);
				
				it = _queued_blocks.erase(it);
			} else
			{
				++it;
			}
		}
	}
	
	if(new_loaded)
		_wall_update_pool->run();
}


void world_controller::full_update()
{
	assert(_world_created);

	const int render_diameter = _chunk_radius*2;

		
	for(int x = 0; x < render_diameter; ++x)
	{
		for(int y = 0; y < render_diameter; ++y)
		{
			for(int z = 0; z < render_diameter; ++z)
			{
				const vec3d<int> calc_chunk = vec3d<int>{x-_chunk_radius, y-_chunk_radius, z-_chunk_radius};
				
				if(vec3d<int>::magnitude(calc_chunk) < _chunk_radius)
				{
					const vec3d<int> check_chunk = _main_character->active_chunk_pos + calc_chunk;
						
					if(_been_loaded_chunks.insert(check_chunk).second)
						_chunk_gen_pool->run(check_chunk);
				}
			}
		}
	}
	
	range_remove();
	add_chunks();
}

void world_controller::draw_update() const
{
	for(auto& [pos, f_chunk] : world_chunks)
	{
		if(f_chunk.chunk.empty())
			continue;
	
		const vec3d<int>& chunk_pos = f_chunk.chunk.position();
		const vec3d<int> diff_pos = chunk_pos-_main_character->active_chunk_pos;
		
		//check if chunk is too far away to render
		if(vec3d<int>::magnitude(diff_pos) > _render_dist)
			continue;

		//check if the chunk is in the camera frustum
		const vec3d<float> check_pos_f = vec3d_cvt<float>(chunk_pos)*chunk_size+vec3d<float>{chunk_size/2, chunk_size/2, chunk_size/2};
		if(!_main_camera->cube_in_frustum({check_pos_f.x, check_pos_f.y, check_pos_f.z}, chunk_size))
			continue;
		
		f_chunk.model.draw();
	}
}

void world_controller::update_queued(std::map<vec3d<int>, wall_states>& update_chunks)
{
	std::lock_guard<std::mutex> lock_b(_world_gen->_mtx_block_place);
		
	for(auto it = _world_gen->_block_place_vec.begin(); it != _world_gen->_block_place_vec.end();)
	{
		const world_generator::vec_pos& c_vec_pos = *it;
		const vec3d<int>& c_pos = c_vec_pos.chunk_pos;
	
		auto c_iter = world_chunks.find(c_pos);
		if(c_iter!=world_chunks.end())
		{	
			world_chunk& c_chunk = c_iter->second.chunk;
				
			c_chunk.block(c_vec_pos.block_pos) = c_vec_pos.block;
			
			update_chunks.try_emplace(c_pos, wall_states{false, false, false, false, false, false}).first->second.add_walls(c_vec_pos.walls);
			
			if(c_chunk.empty())
				c_chunk.set_empty(c_chunk.check_empty());
				
		} else
		{
			if(chunk_outside(c_pos) <= _chunk_radius+2)
			{
				++it;
				continue;
			}
		}
		
		auto new_vec_end = _world_gen->_block_place_vec.end()-1;
		
		if(new_vec_end!=it)
			*it = std::move(*new_vec_end);
			
		_world_gen->_block_place_vec.pop_back();
	}
}

int world_controller::render_dist()
{
	return _render_dist;
}

int world_controller::chunk_radius()
{
	return _chunk_radius;
}
