#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>

#include <random>

#include "chunk.h"
#include "inventory.h"
#include "types.h"

using namespace yanderegl;
using namespace world_types;


world_chunk::world_chunk(const vec3d<int> pos)
: _position(pos)
{
}

bool world_chunk::has_transparent() const noexcept
{
	auto iter = std::find_if(blocks.begin(), blocks.end(), [](const world_block& block){return block.transparent();});
	return iter!=blocks.end();
}

bool world_chunk::check_empty() const noexcept
{
	auto iter = std::find_if(blocks.begin(), blocks.end(), [](const world_block& block){return block.block_type!=block::air;});
	return iter==blocks.end();
}


void world_chunk::update_states()
{
	if(_empty)
		return;

	std::for_each(std::execution::par_unseq, blocks.begin(), blocks.end(), [](world_block& block)
	{
			block.update();
	});
}

vec3d<int> world_chunk::active_chunk(const vec3d<int> pos) noexcept
{
	vec3d<int> ret_pos = vec3d<int>{0, 0, 0};
	
	if(pos.x<0)
		ret_pos.x = (pos.x-chunk_size)/chunk_size;
	
	if(pos.x>0)
		ret_pos.x = pos.x/chunk_size;
			
	if(pos.y<0)
		ret_pos.y = (pos.y-chunk_size)/chunk_size;
			
	if(pos.y>0)
		ret_pos.y = pos.y/chunk_size;
			
	if(pos.z<0)
		ret_pos.z = (pos.z-chunk_size)/chunk_size;
			
	if(pos.z>0)
		ret_pos.z = pos.z/chunk_size;
	
	return ret_pos;
}

vec3d<int> world_chunk::active_chunk(const vec3d<float> pos) noexcept
{
	vec3d<int> ret_pos = vec3d<int>{0, 0, 0};
	
	if(pos.x<0)
		ret_pos.x = (static_cast<int>(pos.x)-chunk_size)/chunk_size;
	
	if(pos.x>0)
		ret_pos.x = (static_cast<int>(pos.x))/chunk_size;
			
	if(pos.y<0)
		ret_pos.y = (static_cast<int>(pos.y)-chunk_size)/chunk_size;
			
	if(pos.y>0)
		ret_pos.y = (static_cast<int>(pos.y))/chunk_size;
			
	if(pos.z<0)
		ret_pos.z = (static_cast<int>(pos.z)-chunk_size)/chunk_size;
			
	if(pos.z>0)
		ret_pos.z = (static_cast<int>(pos.z))/chunk_size;
	
	return ret_pos;
}

vec3d<int> world_chunk::closest_block(const vec3d<float> pos) noexcept
{
	return {static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(pos.z)};
}

world_block& world_chunk::block(const vec3d<int> pos) noexcept
{
	assert((pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z)<(chunk_size*chunk_size*chunk_size));
	return blocks[pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z];
}

const world_block& world_chunk::block(const vec3d<int> pos) const noexcept
{
	assert((pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z)<(chunk_size*chunk_size*chunk_size));
	return blocks[pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z];
}

void world_chunk::set_empty(const bool state) noexcept
{
	_empty = state;
}

bool world_chunk::empty() const noexcept
{
	return _empty;
}

int world_chunk::plants_amount() const noexcept
{
	return _plants_amount;
}

void world_chunk::set_plants_amount(const int amount) noexcept
{
	_plants_amount = amount;
}

const vec3d<int> world_chunk::position() const noexcept
{
	return _position;
}


model_chunk::model_chunk(world_chunk& owner, yanderegl::yandere_shader_program shader, const texture_atlas& atlas)
: _own_chunk(owner),
_draw_object(shader, yandere_model(&_model), atlas.texture,
yan_transforms{{static_cast<float>(owner.position().x)*chunk_size,
static_cast<float>(owner.position().y)*chunk_size,
static_cast<float>(owner.position().z)*chunk_size}, {chunk_size, chunk_size, chunk_size}}),
_texture_distance_x(atlas.tex_offset_x), _texture_distance_y(atlas.tex_offset_y)
{
}

void model_chunk::update_mesh() noexcept
{
	std::lock_guard<std::mutex> lock(_model_mutex.mtx);

	_model.clear();
	_walls_empty = wall_states{};
	_walls_full = false;

	if(_own_chunk.empty())
		return;
		
	_index_offset = 0;
		
	int block_index = 0;
	
	const world_chunk::chunk_blocks& blocks = _own_chunk.blocks;
		
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int y = 0; y < chunk_size; ++y)
		{
			for(int z = 0; z < chunk_size; ++z, ++block_index)
			{	
				if(blocks[block_index].block_type!=block::air)
					update_block_walls({x, y, z}, block_index);
			}
		}
	}
}

void model_chunk::update_wall(const ytype::direction wall, const world_chunk& check_chunk) noexcept
{
	std::lock_guard<std::mutex> lock(_model_mutex.mtx);

	if(_own_chunk.empty())
		return;
		

	switch(wall)
	{
		case ytype::direction::right:
			if(_walls_empty.right)
				update_right_wall(check_chunk);
			break;
		
		case ytype::direction::left:
			if(_walls_empty.left)
				update_left_wall(check_chunk);
			break;
		
		case ytype::direction::up:
			if(_walls_empty.up)
				update_up_wall(check_chunk);
			break;
		
		case ytype::direction::down:
			if(_walls_empty.down)
				update_down_wall(check_chunk);
			break;
		
		case ytype::direction::forward:
			if(_walls_empty.forward)
				update_forward_wall(check_chunk);
			break;
		
		case ytype::direction::back:
			if(_walls_empty.back)
				update_back_wall(check_chunk);
			break;
	}
	
	_walls_full = !_walls_empty.walls_or();
}

void model_chunk::draw() const noexcept
{
	if(_model_mutex.mtx.try_lock())
	{
		_draw_object.draw_update();
		
		_model_mutex.mtx.unlock();
	}
}

wall_states model_chunk::walls() const noexcept
{
	return _walls_empty;
}

bool model_chunk::walls_full() const noexcept
{
	return _walls_full;
}

void model_chunk::update_block_walls(const vec3d<int> pos) noexcept
{
	update_block_walls(pos, pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z);
}

void model_chunk::update_block_walls(const vec3d<int> pos, const int index) noexcept
{
	//if both the current and the check block are the same, don't draw the side (makes stuff like leaves connect into a single thing)
	const world_chunk::chunk_blocks& blocks = _own_chunk.blocks;
	
	const world_block& c_block = blocks[index];
	const int c_block_type = c_block.block_type;
	
	const texture_face& c_text = c_block.texture();
	
	const vec3d<float> pos_f = vec3d_cvt<float>(pos)*block_model_size;
	
	if(pos.z!=(chunk_size-1) && blocks[index+1].transparent() && (c_block_type != blocks[index+1].block_type))
		a_forward_face(pos_f, c_text.forward);
	
	if(pos.z!=0 && blocks[index-1].transparent() && (c_block_type != blocks[index-1].block_type))
		a_back_face(pos_f, c_text.back);

	if(pos.y!=(chunk_size-1) && blocks[index+chunk_size].transparent() && (c_block_type != blocks[index+chunk_size].block_type))
		a_up_face(pos_f, c_text.up);

	if(pos.y!=0 && blocks[index-chunk_size].transparent() && (c_block_type != blocks[index-chunk_size].block_type))
		a_down_face(pos_f, c_text.down);

	if(pos.x!=(chunk_size-1) && blocks[index+chunk_size*chunk_size].transparent() && (c_block_type != blocks[index+chunk_size*chunk_size].block_type))
		a_right_face(pos_f, c_text.right);

	if(pos.x!=0 && blocks[index-chunk_size*chunk_size].transparent() && (c_block_type != blocks[index-chunk_size*chunk_size].block_type))
		a_left_face(pos_f, c_text.left);
}

void model_chunk::update_right_wall(const world_chunk& check_chunk) noexcept
{
	int starting_index = (chunk_size-1)*chunk_size*chunk_size;
	int block_index = starting_index;
	
	for(int y = 0; y < chunk_size; ++y)
	{
		for(int z = 0; z < chunk_size; ++z, ++block_index)
		{
			if(_own_chunk.blocks[block_index].block_type!=block::air)
			{
				if(check_chunk.empty() || (check_chunk.blocks[block_index-starting_index].transparent()
				&& (_own_chunk.blocks[block_index].block_type != check_chunk.blocks[block_index-starting_index].block_type)))
					a_right_face(vec3d_cvt<float>(chunk_size-1, y, z)*block_model_size, _own_chunk.blocks[block_index].texture().right);
			}
		}
	}
	
	_walls_empty.right = false;
}

void model_chunk::update_left_wall(const world_chunk& check_chunk) noexcept
{
	int starting_index = (chunk_size-1)*chunk_size*chunk_size;
	int block_index = starting_index;
	
	for(int y = 0; y < chunk_size; ++y)
	{
		for(int z = 0; z < chunk_size; ++z, ++block_index)
		{
			if(_own_chunk.blocks[block_index-starting_index].block_type!=block::air)
			{
				if(check_chunk.empty() || (check_chunk.blocks[block_index].transparent()
				&& (_own_chunk.blocks[block_index-starting_index].block_type != check_chunk.blocks[block_index].block_type)))
					a_left_face(vec3d_cvt<float>(0, y, z)*block_model_size, _own_chunk.blocks[block_index-starting_index].texture().left);
			}
		}
	}
	
	_walls_empty.left = false;
}

void model_chunk::update_up_wall(const world_chunk& check_chunk) noexcept
{
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int z = 0; z < chunk_size; ++z)
		{
			const world_block& c_block = _own_chunk.block({x, chunk_size-1, z});
			const world_block& check_block = check_chunk.block({x, 0, z});
					
			if(c_block.block_type!=block::air)
			{
				if(check_chunk.empty() || (check_block.transparent()
				&& (c_block.block_type != check_block.block_type)))
					a_up_face(vec3d_cvt<float>(x, chunk_size-1, z)*block_model_size, c_block.texture().up);
			}
		}
	}
	
	_walls_empty.up = false;
}

void model_chunk::update_down_wall(const world_chunk& check_chunk) noexcept
{
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int z = 0; z < chunk_size; ++z)
		{
			const world_block& c_block = _own_chunk.block({x, 0, z});
			const world_block& check_block = check_chunk.block({x, chunk_size-1, z});
		
			if(c_block.block_type!=block::air)
			{
				if(check_chunk.empty() || (check_block.transparent()
				&& (c_block.block_type != check_block.block_type)))
					a_down_face(vec3d_cvt<float>(x, 0, z)*block_model_size, c_block.texture().down);
			}
		}
	}
	
	_walls_empty.down = false;
}

void model_chunk::update_forward_wall(const world_chunk& check_chunk) noexcept
{
	int block_index = 0;

	for(int x = 0; x < chunk_size; ++x)
	{
		for(int y = 0; y < chunk_size; ++y, block_index+=chunk_size)
		{
			if(_own_chunk.blocks[block_index+chunk_size-1].block_type!=block::air)
			{
				if(check_chunk.empty() || (check_chunk.blocks[block_index].transparent()
				&& (_own_chunk.blocks[block_index+chunk_size-1].block_type != check_chunk.blocks[block_index].block_type)))
					a_forward_face(vec3d_cvt<float>(x, y, chunk_size-1)*block_model_size, _own_chunk.blocks[block_index+chunk_size-1].texture().forward);
			}
		}
	}
	
	_walls_empty.forward = false;
}

void model_chunk::update_back_wall(const world_chunk& check_chunk) noexcept
{
	int block_index = 0;

	for(int x = 0; x < chunk_size; ++x)
	{
		for(int y = 0; y < chunk_size; ++y, block_index+=chunk_size)
		{
			if(_own_chunk.blocks[block_index].block_type!=block::air)
			{
				if(check_chunk.empty() || (check_chunk.blocks[block_index+chunk_size-1].transparent()
				&& (_own_chunk.blocks[block_index].block_type != check_chunk.blocks[block_index+chunk_size-1].block_type)))
					a_back_face(vec3d_cvt<float>(x, y, 0)*block_model_size, _own_chunk.blocks[block_index].texture().back);
			}
		}
	}
	
	_walls_empty.back = false;
}

void model_chunk::a_forward_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	//i cant write any better code for these, it literally HAS to be hardcoded :/
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y,
	pos_f.x+block_model_size, pos_f.y, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y,
	pos_f.x, pos_f.y+block_model_size, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y+_texture_distance_y,
	pos_f.x+block_model_size, pos_f.y+block_model_size, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y+_texture_distance_y});

	_model.indices_insert({_index_offset, _index_offset+1, _index_offset+2, _index_offset+1, _index_offset+3, _index_offset+2});
	
	_index_offset += 4;
}

void model_chunk::a_back_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y,
	pos_f.x+block_model_size, pos_f.y, pos_f.z, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y,
	pos_f.x, pos_f.y+block_model_size, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y+_texture_distance_y,
	pos_f.x+block_model_size, pos_f.y+block_model_size, pos_f.z, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y+_texture_distance_y});

	_model.indices_insert({_index_offset, _index_offset+2, _index_offset+1, _index_offset+1, _index_offset+2, _index_offset+3});
	
	_index_offset += 4;
}

void model_chunk::a_left_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y,
	pos_f.x, pos_f.y, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y,
	pos_f.x, pos_f.y+block_model_size, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y+_texture_distance_y,
	pos_f.x, pos_f.y+block_model_size, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y+_texture_distance_y});
	
	_model.indices_insert({_index_offset, _index_offset+1, _index_offset+2, _index_offset+1, _index_offset+3, _index_offset+2});
	
	_index_offset += 4;
}

void model_chunk::a_right_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x+block_model_size, pos_f.y, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y,
	pos_f.x+block_model_size, pos_f.y, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y,
	pos_f.x+block_model_size, pos_f.y+block_model_size, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y+_texture_distance_y,
	pos_f.x+block_model_size, pos_f.y+block_model_size, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y+_texture_distance_y});	
	
	_model.indices_insert({_index_offset, _index_offset+2, _index_offset+1, _index_offset+1, _index_offset+2, _index_offset+3});
	
	_index_offset += 4;
}

void model_chunk::a_up_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y+block_model_size, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y,
	pos_f.x+block_model_size, pos_f.y+block_model_size, pos_f.z, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y,
	pos_f.x, pos_f.y+block_model_size, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y+_texture_distance_y,
	pos_f.x+block_model_size, pos_f.y+block_model_size, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y+_texture_distance_y});
	
	_model.indices_insert({_index_offset, _index_offset+2, _index_offset+1, _index_offset+1, _index_offset+2, _index_offset+3});
	
	_index_offset += 4;
}

void model_chunk::a_down_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y,
	pos_f.x+block_model_size, pos_f.y, pos_f.z, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y,
	pos_f.x, pos_f.y, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x, _texture_distance_y*texture_pos.y+_texture_distance_y,
	pos_f.x+block_model_size, pos_f.y, pos_f.z+block_model_size, _texture_distance_x*texture_pos.x+_texture_distance_x, _texture_distance_y*texture_pos.y+_texture_distance_y});
	
	_model.indices_insert({_index_offset, _index_offset+1, _index_offset+2, _index_offset+1, _index_offset+3, _index_offset+2});
	
	_index_offset += 4;
}

full_chunk::full_chunk(world_chunk chunk, yanderegl::yandere_shader_program shader, const texture_atlas& atlas)
: chunk(chunk), model(this->chunk, shader, atlas)
{
	model.update_mesh();
}
