#include "cmodel.h"

using namespace yangl;
using namespace world_types;

model_holder::model_holder()
{
}

model_holder::model_holder(const camera* cam, const generic_shader* shader, const texture_atlas& atlas)
: _model(),
_draw_object(cam, shader, &_model, atlas.texture),
_distance_x(atlas.tex_offset_x), _distance_y(atlas.tex_offset_y)
{
}

model_holder::model_holder(const model_holder& other)
: _model(other._model),
_draw_object(other._draw_object),
_index(other._index),
_distance_x(other._distance_x), _distance_y(other._distance_y)
{
	_draw_object.set_model(&_model);
}

model_holder::model_holder(model_holder&& other) noexcept
: _model(std::move(other._model)),
_draw_object(std::move(other._draw_object)),
_index(other._index),
_distance_x(other._distance_x), _distance_y(other._distance_y)
{
	_draw_object.set_model(&_model);
}

model_holder& model_holder::operator=(const model_holder& other)
{
	if(this!=&other)
	{
		_model = other._model;
		_draw_object = other._draw_object;
		_draw_object.set_model(&_model);

		_index = other._index;

		_distance_x = other._distance_x;
		_distance_y = other._distance_y;
	}
	return *this;
}

model_holder& model_holder::operator=(model_holder&& other) noexcept
{
	if(this!=&other)
	{
		_model = std::move(other._model);
		_draw_object = std::move(other._draw_object);
		_draw_object.set_model(&_model);

		_index = other._index;

		_distance_x = other._distance_x;
		_distance_y = other._distance_y;
	}
	return *this;
}

void model_holder::set_position(const vec3d<float> pos) noexcept
{
	_draw_object.set_position({pos.x, pos.y, pos.z});
}

void model_holder::set_scale(const float scale) noexcept
{
	_draw_object.set_scale({scale, scale, scale});
}

void model_holder::draw() noexcept
{
	_model.generate_buffers();
	_draw_object.draw();
}

void model_holder::clear() noexcept
{
	_model.clear();
	_index = 0;
}

void model_holder::a_forward_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	//i cant write any better code for these, it literally HAS to be hardcoded :/
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z+block_size, _distance_x*texture_pos.x, _distance_y*texture_pos.y,
		pos_f.x+block_size, pos_f.y, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y,
		pos_f.x, pos_f.y+block_size, pos_f.z+block_size, _distance_x*texture_pos.x, _distance_y*texture_pos.y+_distance_y,
		pos_f.x+block_size, pos_f.y+block_size, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y+_distance_y});

	_model.indices_insert({_index, _index+1, _index+2, _index+1, _index+3, _index+2});

	_index += 4;
}

void model_holder::a_back_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y,
		pos_f.x+block_size, pos_f.y, pos_f.z, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y,
		pos_f.x, pos_f.y+block_size, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y+_distance_y,
		pos_f.x+block_size, pos_f.y+block_size, pos_f.z, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y+_distance_y});

	_model.indices_insert({_index, _index+2, _index+1, _index+1, _index+2, _index+3});

	_index += 4;
}

void model_holder::a_left_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y,
		pos_f.x, pos_f.y, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y,
		pos_f.x, pos_f.y+block_size, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y+_distance_y,
		pos_f.x, pos_f.y+block_size, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y+_distance_y});

	_model.indices_insert({_index, _index+1, _index+2, _index+1, _index+3, _index+2});

	_index += 4;
}

void model_holder::a_right_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x+block_size, pos_f.y, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y,
		pos_f.x+block_size, pos_f.y, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y,
		pos_f.x+block_size, pos_f.y+block_size, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y+_distance_y,
		pos_f.x+block_size, pos_f.y+block_size, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y+_distance_y});

	_model.indices_insert({_index, _index+2, _index+1, _index+1, _index+2, _index+3});

	_index += 4;
}

void model_holder::a_up_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y+block_size, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y,
		pos_f.x+block_size, pos_f.y+block_size, pos_f.z, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y,
		pos_f.x, pos_f.y+block_size, pos_f.z+block_size, _distance_x*texture_pos.x, _distance_y*texture_pos.y+_distance_y,
		pos_f.x+block_size, pos_f.y+block_size, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y+_distance_y});

	_model.indices_insert({_index, _index+2, _index+1, _index+1, _index+2, _index+3});

	_index += 4;
}

void model_holder::a_down_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept
{
	_model.vertices_insert({pos_f.x, pos_f.y, pos_f.z, _distance_x*texture_pos.x, _distance_y*texture_pos.y,
		pos_f.x+block_size, pos_f.y, pos_f.z, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y,
		pos_f.x, pos_f.y, pos_f.z+block_size, _distance_x*texture_pos.x, _distance_y*texture_pos.y+_distance_y,
		pos_f.x+block_size, pos_f.y, pos_f.z+block_size, _distance_x*texture_pos.x+_distance_x, _distance_y*texture_pos.y+_distance_y});

	_model.indices_insert({_index, _index+1, _index+2, _index+1, _index+3, _index+2});

	_index += 4;
}


model_chunk::model_chunk()
{
}

model_chunk::model_chunk(const world_chunk* owner,
	const graphics_state& graphics)
: _own_chunk(owner),
_opaque_model(graphics.camera, graphics.shader, graphics.opaque_atlas),
_transparent_model(graphics.camera, graphics.shader, graphics.transparent_atlas)
{
	const vec3d<float> chunk_pos{owner->position().cast<float>()*chunk_size};

	_opaque_model.set_position(chunk_pos);
	_transparent_model.set_position(chunk_pos);

	_opaque_model.set_scale(chunk_size);
	_transparent_model.set_scale(chunk_size);
}

void model_chunk::update_mesh() noexcept
{
	_opaque_model.clear();
	_transparent_model.clear();

	_walls_empty = wall_states{};
	_walls_full = false;

	assert(_own_chunk!=nullptr);
	if(_own_chunk->empty())
		return;
		
	int block_index = 0;
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int y = 0; y < chunk_size; ++y)
		{
			for(int z = 0; z < chunk_size; ++z, ++block_index)
			{	
				if(_own_chunk->blocks[block_index].block_type!=block::air)
					update_block_walls({x, y, z}, block_index);
			}
		}
	}
}

void model_chunk::update_wall(const world_chunk& check_chunk, const ytype::direction wall) noexcept
{
	assert(_own_chunk!=nullptr);
	if(_own_chunk->empty())
		return;
		

	switch(wall)
	{
		default:
		case ytype::direction::right:
			if(_walls_empty.right)
				update_wall_x<true>(check_chunk);
			break;
		
		case ytype::direction::left:
			if(_walls_empty.left)
				update_wall_x<false>(check_chunk);
			break;
		
		case ytype::direction::up:
			if(_walls_empty.up)
				update_wall_y<true>(check_chunk);
			break;
		
		case ytype::direction::down:
			if(_walls_empty.down)
				update_wall_y<false>(check_chunk);
			break;
		
		case ytype::direction::forward:
			if(_walls_empty.forward)
				update_wall_z<true>(check_chunk);
			break;
		
		case ytype::direction::back:
			if(_walls_empty.back)
				update_wall_z<false>(check_chunk);
			break;
	}
	
	_walls_full = !_walls_empty.walls_or();
}

void model_chunk::draw_opaque() noexcept
{
	_opaque_model.draw();
}

void model_chunk::draw_transparent() noexcept
{
	_transparent_model.draw();
}

wall_states model_chunk::walls() const noexcept
{
	return _walls_empty;
}

bool model_chunk::walls_full() const noexcept
{
	return _walls_full;
}

void model_chunk::set_owner(const world_chunk* owner) noexcept
{
	_own_chunk = owner;
}

void model_chunk::update_block_walls(const vec3d<int> pos) noexcept
{
	update_block_walls(pos, pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z);
}

void model_chunk::update_block_walls(const vec3d<int> pos, const int index) noexcept
{
	const world_chunk::chunk_blocks& blocks = _own_chunk->blocks;
	
	const world_block& c_block = blocks[index];
	const texture_face& c_texture = c_block.texture();

	model_holder& c_model = c_block.transparent() ? _transparent_model : _opaque_model;
	const vec3d<float> pos_f{pos.cast<float>()*c_model.block_size};

	if(pos.z!=(chunk_size-1) && draw_side(c_block, blocks[index+1]))
		c_model.a_forward_face(pos_f, c_texture.forward);
	
	if(pos.z!=0 && draw_side(c_block, blocks[index-1]))
		c_model.a_back_face(pos_f, c_texture.back);

	if(pos.y!=(chunk_size-1) && draw_side(c_block, blocks[index+chunk_size]))
		c_model.a_up_face(pos_f, c_texture.up);

	if(pos.y!=0 && draw_side(c_block, blocks[index-chunk_size]))
		c_model.a_down_face(pos_f, c_texture.down);

	if(pos.x!=(chunk_size-1) && draw_side(c_block, blocks[index+chunk_size*chunk_size]))
		c_model.a_right_face(pos_f, c_texture.right);

	if(pos.x!=0 && draw_side(c_block, blocks[index-chunk_size*chunk_size]))
		c_model.a_left_face(pos_f, c_texture.left);
}

template<bool right_wall>
void model_chunk::update_wall_x(const world_chunk& check_chunk) noexcept
{
	const world_chunk::chunk_blocks& blocks = _own_chunk->blocks;
	const world_chunk::chunk_blocks& check_blocks = check_chunk.blocks;

	const int starting_index = (chunk_size-1)*chunk_size*chunk_size;
	int block_index = starting_index;
	
	for(int y = 0; y < chunk_size; ++y)
	{
		for(int z = 0; z < chunk_size; ++z, ++block_index)
		{
			const world_block c_block = right_wall?
				blocks[block_index]
				: blocks[block_index-starting_index];

			if(c_block.block_type!=block::air)
			{
				if(!check_chunk.empty())
				{
					const world_block check_block = right_wall?
						check_blocks[block_index-starting_index]
						: check_blocks[block_index];

					if(!draw_side(c_block, check_block))
						continue;
				}

				model_holder& c_model = c_block.transparent() ? _transparent_model : _opaque_model;

				if(right_wall)
					c_model.a_right_face(
						vec3d<int>{chunk_size-1, y, z}.cast<float>()*c_model.block_size,
						c_block.texture().right);
				else
					c_model.a_left_face(
						vec3d<int>{0, y, z}.cast<float>()*c_model.block_size,
						c_block.texture().left);
			}
		}
	}
	
	if(right_wall)
		_walls_empty.right = false;
	else
		_walls_empty.left = false;
}

template<bool up_wall>
void model_chunk::update_wall_y(const world_chunk& check_chunk) noexcept
{
	for(int x = 0; x < chunk_size; ++x)
	{
		for(int z = 0; z < chunk_size; ++z)
		{
			const world_block c_block = up_wall?
				_own_chunk->block({x, chunk_size-1, z})
				: _own_chunk->block({x, 0, z});
					
			if(c_block.block_type!=block::air)
			{
				const world_block check_block = up_wall?
					check_chunk.block({x, 0, z})
					: check_chunk.block({x, chunk_size-1, z});

				if(check_chunk.empty() || draw_side(c_block, check_block))
				{
					model_holder& c_model = c_block.transparent() ? _transparent_model : _opaque_model;

					if(up_wall)
						c_model.a_up_face(
							vec3d<int>{x, chunk_size-1, z}.cast<float>()*c_model.block_size,
							c_block.texture().up);
					else
						c_model.a_down_face(
							vec3d<int>{x, 0, z}.cast<float>()*c_model.block_size,
							c_block.texture().down);
				}
			}
		}
	}

	if(up_wall)
		_walls_empty.up = false;
	else
		_walls_empty.down = false;
}

template<bool forward_wall>
void model_chunk::update_wall_z(const world_chunk& check_chunk) noexcept
{
	const world_chunk::chunk_blocks& blocks = _own_chunk->blocks;
	const world_chunk::chunk_blocks& check_blocks = check_chunk.blocks;

	int block_index = 0;

	for(int x = 0; x < chunk_size; ++x)
	{
		for(int y = 0; y < chunk_size; ++y, block_index+=chunk_size)
		{
			const world_block c_block = forward_wall?
			blocks[block_index+chunk_size-1]
			: blocks[block_index];

			if(c_block.block_type!=block::air)
			{
				if(!check_chunk.empty())
				{
					const world_block check_block = forward_wall?
					check_blocks[block_index]
					: check_blocks[block_index+chunk_size-1];

					if(!draw_side(c_block, check_block))
						continue;
				}

				model_holder& c_model = c_block.transparent() ? _transparent_model : _opaque_model;

				if(forward_wall)
					c_model.a_forward_face(
						vec3d<int>{x, y, chunk_size-1}.cast<float>()*c_model.block_size,
						c_block.texture().forward);
				else
					c_model.a_back_face(
						vec3d<int>{x, y, 0}.cast<float>()*c_model.block_size,
						c_block.texture().back);
			}
		}
	}
	
	if(forward_wall)
		_walls_empty.forward = false;
	else
		_walls_empty.back = false;
}

bool model_chunk::draw_side(const world_block& block, const world_block& check) noexcept
{
	return check.transparent() && (block.block_type != check.block_type);
}

full_chunk::full_chunk()
{
}

full_chunk::full_chunk(const world_chunk chunk,
	const graphics_state& graphics)
: chunk(chunk), model(&this->chunk, graphics)
{
	model.update_mesh();
}

full_chunk::full_chunk(const full_chunk& other)
: chunk(other.chunk), model(other.model)
{
	model.set_owner(&chunk);
}

full_chunk::full_chunk(full_chunk&& other) noexcept
: chunk(std::move(other.chunk)), model(std::move(other.model))
{
	model.set_owner(&chunk);
}

full_chunk& full_chunk::operator=(const full_chunk& other)
{
	if(this!=&other)
	{
		chunk = other.chunk;
		model = other.model;
		model.set_owner(&chunk);
	}
	return *this;
}

full_chunk& full_chunk::operator=(full_chunk&& other) noexcept
{
	if(this!=&other)
	{
		chunk = std::move(other.chunk);
		model = std::move(other.model);
		model.set_owner(&chunk);
	}
	return *this;
}