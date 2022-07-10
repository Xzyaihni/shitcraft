#include <iostream>
#include <algorithm>
#include <chrono>
#include <execution>
#include <cassert>

#include "chunk.h"
#include "inventory.h"
#include "types.h"


using namespace world_types;

world_chunk::world_chunk()
{
}

world_chunk::world_chunk(const vec3d<int> pos)
: _position(pos)
{
}

void world_chunk::connect_observer(chunk_observer* observer) noexcept
{
	_observers.push_back(observer);
}

void world_chunk::remove_observer(chunk_observer* observer) noexcept
{
	_observers.erase(std::find(_observers.begin(), _observers.end(), observer));
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

	std::for_each(blocks.begin(), blocks.end(), [](world_block& block)
	{
			block.update();
	});
}

vec3d<int> world_chunk::active_chunk(const vec3d<int> pos) noexcept
{
	return vec3d<int>{
		pos.x<0?((pos.x-chunk_size)/chunk_size):(pos.x/chunk_size),
		pos.y<0?((pos.y-chunk_size)/chunk_size):(pos.y/chunk_size),
		pos.z<0?((pos.z-chunk_size)/chunk_size):(pos.z/chunk_size)};
}

vec3d<int> world_chunk::active_chunk(const vec3d<float> pos) noexcept
{
	return active_chunk(vec3d<int>{
		static_cast<int>(pos.x),
		static_cast<int>(pos.y),
		static_cast<int>(pos.z)});
}

vec3d<int> world_chunk::round_block(const vec3d<float> pos) noexcept
{
	return {static_cast<int>(std::floor(pos.x)),
		static_cast<int>(std::floor(pos.y)),
		static_cast<int>(std::floor(pos.z))};
}

vec3d<int> world_chunk::closest_bound_block(const vec3d<int> pos) noexcept
{
	const vec3d<int> clipped_pos{pos.x%chunk_size,
		pos.y%chunk_size,
		pos.z%chunk_size};

	return {clipped_pos.x<0?chunk_size+clipped_pos.x:clipped_pos.x,
		clipped_pos.y<0?chunk_size+clipped_pos.y:clipped_pos.y,
		clipped_pos.z<0?chunk_size+clipped_pos.z:clipped_pos.z};
}

vec3d<int> world_chunk::closest_bound_block(const vec3d<float> pos) noexcept
{
	const vec3d<int> rounded_pos{
		pos.x<0?static_cast<int>(pos.x-1):static_cast<int>(pos.x),
		pos.y<0?static_cast<int>(pos.y-1):static_cast<int>(pos.y),
		pos.z<0?static_cast<int>(pos.z-1):static_cast<int>(pos.z)};

	return closest_bound_block(rounded_pos);
}

wall_states world_chunk::block_sides(const vec3d<int> pos) noexcept
{
	return wall_states{pos.x==chunk_size-1, pos.x==0,
		pos.y==chunk_size-1, pos.y==0,
		pos.z==chunk_size-1, pos.z==0};
}

void world_chunk::set_block(const world_block block, const vec3d<int> pos) noexcept
{
	blocks[index_block(pos)] = block;

	notify_observers(_position, pos);
}

world_block& world_chunk::block(const vec3d<int> pos) noexcept
{
	return blocks[index_block(pos)];
}

const world_block& world_chunk::block(const vec3d<int> pos) const noexcept
{
	return blocks[index_block(pos)];
}

void world_chunk::set_empty(const bool state) noexcept
{
	_empty = state;
}

bool world_chunk::empty() const noexcept
{
	return _empty;
}

const vec3d<int> world_chunk::position() const noexcept
{
	return _position;
}

int world_chunk::index_block(const vec3d<int> pos) noexcept
{
	assert((pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z)<(chunk_size*chunk_size*chunk_size));
	return pos.x*chunk_size*chunk_size+pos.y*chunk_size+pos.z;
}

void world_chunk::notify_observers(const vec3d<int> chunk, const vec3d<int> pos) noexcept
{
	std::for_each(_observers.begin(), _observers.end(), [&chunk, &pos](chunk_observer* observer)
	{
		observer->block_notify(chunk, pos);
	});
}