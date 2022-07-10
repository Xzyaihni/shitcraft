#include <iostream>

#include "cmap.h"
#include "wgen.h"


using namespace cmap;

storage::storage()
{
}

storage::storage(controller* owner, world_generator* generator, const int size)
: chunks(size), _owner(owner), _generator(generator), _chunks_amount(size)
{
	_open_spots.reserve(_chunks_amount);
	for(int i = 0; i < _chunks_amount; ++i)
	{
		_open_spots.emplace_back(i);
	}
}

storage::storage(const storage& other)
{
	std::lock_guard lock(other.chunk_gen_mtx);

	copy_members(other);
}

storage::storage(storage&& other) noexcept
{
	std::lock_guard lock(other.chunk_gen_mtx);

	move_members(std::move(other));
}

storage& storage::operator=(const storage& other)
{
	if(this != &other)
	{
		std::unique_lock<std::mutex> lock_self(chunk_gen_mtx, std::defer_lock);
		std::unique_lock<std::mutex> lock(other.chunk_gen_mtx, std::defer_lock);
		std::lock(lock_self, lock);

		copy_members(other);
	}
	return *this;
}

storage& storage::operator=(storage&& other) noexcept
{
	if(this != &other)
	{
		std::unique_lock<std::mutex> lock_self(chunk_gen_mtx, std::defer_lock);
		std::unique_lock<std::mutex> lock(other.chunk_gen_mtx, std::defer_lock);
		std::lock(lock_self, lock);

		move_members(std::move(other));
	}
	return *this;
}

void storage::generate_chunk(const vec3d<int> pos)
{
	assert(_generator!=nullptr);

	full_chunk f_chunk = _generator->full_chunk_gen(pos);


	std::lock_guard lock(chunk_gen_mtx);

	if(_open_spots.empty())
		throw std::runtime_error("_open_spots is empty");
	const int open_index = _open_spots.back();
	full_chunk& c_chunk = chunks[open_index];

	c_chunk = std::move(f_chunk);
	processed_chunks.push_back(&c_chunk);

	_open_spots.pop_back();
}

void storage::remove_chunk(container_type::iterator chunk)
{
	std::lock_guard lock(chunk_gen_mtx);

	remove_chunk(*chunk, std::distance(chunks.begin(), chunk));
}

void storage::remove_chunk(full_chunk& chunk)
{
	std::lock_guard lock(chunk_gen_mtx);

	for(int i = 0; i < chunks.size(); ++i)
	{
		if((chunks.data()+i)==&chunk)
		{
			remove_chunk(chunk, i);
			return;
		}
	}

	//if chunk not found then ignore
}

void storage::remove_chunk(full_chunk& chunk, const int index)
{
	_open_spots.push_back(index);
	chunk.chunk.set_empty(true);
}

void storage::clear() noexcept
{
	std::lock_guard lock(chunk_gen_mtx);

	processed_chunks.clear();

	_open_spots.clear();
	_open_spots.reserve(_chunks_amount);
	for(int i = 0; i < _chunks_amount; ++i)
	{
		_open_spots.emplace_back(i);
	}
}

void storage::copy_members(const storage& other)
{
	chunks = other.chunks;
	processed_chunks = ref_container_type();

	_chunks_amount = other._chunks_amount;

	_open_spots = other._open_spots;
	_owner = other._owner;
	_generator = other._generator;
}

void storage::move_members(storage&& other) noexcept
{
	chunks = std::move(other.chunks);
	processed_chunks = ref_container_type();

	_chunks_amount = other._chunks_amount;

	_open_spots = std::move(other._open_spots);
	_owner = other._owner;
	_generator = other._generator;
}

controller::iterator::iterator(const value_type* end, pointer p)
: _end_ptr(end), _ptr(p)
{
}

controller::iterator::reference controller::iterator::operator*() const
{
	return **_ptr;
}

controller::iterator::value_type controller::iterator::operator->()
{
	return *_ptr;
}

controller::iterator& controller::iterator::operator++()
{
	while(++_ptr!=_end_ptr && *_ptr==nullptr);
	return *this;
}

controller::iterator controller::iterator::operator++(int)
{
	iterator temp{*this};
	++(*this);
	return temp;
}

namespace cmap
{
	bool operator==(const controller::iterator& a, const controller::iterator& b)
	{
		return a._ptr==b._ptr;
	}

	bool operator!=(const controller::iterator& a, const controller::iterator& b)
	{
		return a._ptr!=b._ptr;
	}
};

controller::const_iterator::const_iterator(pointer end, pointer p)
: _end_ptr(end), _ptr(p)
{
}

controller::const_iterator::reference controller::const_iterator::operator*() const
{
	return **_ptr;
}

controller::const_iterator::value_type controller::const_iterator::operator->() const
{
	return *_ptr;
}

controller::const_iterator& controller::const_iterator::operator++()
{
	while(++_ptr!=_end_ptr && *_ptr==nullptr);
	return *this;
}

controller::const_iterator controller::const_iterator::operator++(int)
{
	const_iterator temp{*this};
	++(*this);
	return temp;
}

namespace cmap
{
	bool operator==(const controller::const_iterator& a, const controller::const_iterator& b)
	{
		return a._ptr==b._ptr;
	}

	bool operator!=(const controller::const_iterator& a, const controller::const_iterator& b)
	{
		return a._ptr!=b._ptr;
	}
};

controller::controller()
{
}

controller::controller(world_generator* generator,
	const int render_size, const vec3d<int> center_pos)
: _generator(generator),
_render_size(render_size), _row_size(1+render_size*2),
_chunks_amount(_row_size*_row_size*_row_size),
_center_pos(center_pos),
_chunks(this, generator, _chunks_amount),
_chunks_map(_chunks_amount, nullptr),
_status_flags(_chunks_amount, false)
{
	generate_all();
}

void controller::generate_all() noexcept
{
	generate_pool();

	generate_missing();
}

void controller::generate_pool() noexcept
{
	const int max_threads = std::thread::hardware_concurrency();
	const int chunk_load_threads = std::max(1, max_threads-2);

	_chunk_gen_pool = std::make_unique<cgen_pool_type>(chunk_load_threads,
		&storage::generate_chunk, &_chunks, vec3d<int>{});
}

controller::controller(const controller& other)
: _center_pos(other._center_pos),
_render_size(other._render_size), _row_size(other._row_size),
_chunks_amount(other._chunks_amount),
_generator(other._generator),
_chunks(this, _generator, _chunks_amount),
_chunks_map(_chunks_amount, nullptr),
_status_flags(_chunks_amount, false)
{
	generate_all();
}

controller& controller::operator=(const controller&& other)
{
	if(this!=&other)
	{
		_center_pos = other._center_pos;

		_render_size = other._render_size;
		_row_size = other._row_size;
		_chunks_amount = other._chunks_amount;

		_generator = other._generator;

		_chunks = storage(this, _generator, _chunks_amount);

		_chunks_map = std::vector<full_chunk*>(_chunks_amount, nullptr);
		_status_flags = std::vector<bool>(_chunks_amount, false);

		generate_all();
	}
	return *this;
}

void controller::update() noexcept
{
	connect_processed();
}

void controller::update_center(const vec3d<int> pos)
{
	const bool missing = reassign_chunks(pos);

	_center_pos = pos;

	if(missing)
		generate_missing();
}

void controller::block_notify(const vec3d<int> chunk, const vec3d<int> pos)
{
	update_chunk(chunk);

	update_chunks(chunk, world_chunk::block_sides(pos));
}

full_chunk& controller::at(const vec3d<int> pos)
{
	return *(_chunks_map[index_chunk(pos)]);
}

const full_chunk& controller::at(const vec3d<int> pos) const
{
	return *(_chunks_map[index_chunk(pos)]);
}

vec3d<int> controller::difference(const vec3d<int> pos) const noexcept
{
	return {(pos.x+_render_size)-(_center_pos.x+_render_size),
		(pos.y+_render_size)-(_center_pos.y+_render_size),
		(pos.z+_render_size)-(_center_pos.z+_render_size)};
}

bool controller::in_bounds(const vec3d<int> pos) const noexcept
{
	return in_local_bounds(position_local(pos));
}

bool controller::in_local_bounds(const vec3d<int> rel_pos) const noexcept
{
	return rel_pos.x < _row_size && rel_pos.x >= 0
		&& rel_pos.y < _row_size && rel_pos.y >= 0
		&& rel_pos.z < _row_size && rel_pos.z >= 0;
}

bool controller::contains(const vec3d<int> pos) const noexcept
{
	return in_bounds(pos) && exists(pos);
}

bool controller::contains_local(const vec3d<int> rel_pos) const noexcept
{
	return in_local_bounds(rel_pos) && exists_local(rel_pos);
}

controller::iterator controller::find(const vec3d<int> pos) noexcept
{
	if(contains(pos))
		return iterator(_chunks_map.data()+_chunks_map.size(), _chunks_map.data()+index_chunk(pos));
	else
		return end();
}

controller::const_iterator controller::find(const vec3d<int> pos) const noexcept
{
	if(contains(pos))
		return const_iterator(_chunks_map.data()+_chunks_map.size(), _chunks_map.data()+index_chunk(pos));
	else
		return cend();
}

controller::iterator controller::begin() noexcept
{
	iterator c_iter{_chunks_map.data()+_chunks_map.size(), _chunks_map.data()};
	if(exists(0))
		return c_iter;
	else
		return ++c_iter;
}

controller::const_iterator controller::cbegin() const noexcept
{
	const_iterator c_iter{_chunks_map.data()+_chunks_map.size(), _chunks_map.data()};
	if(exists(0))
		return c_iter;
	else
		return ++c_iter;
}

controller::const_iterator controller::begin() const noexcept
{
	return cbegin();
}

controller::iterator controller::end() noexcept
{
	return iterator(_chunks_map.data()+_chunks_map.size(), _chunks_map.data()+_chunks_map.size());
}

controller::const_iterator controller::cend() const noexcept
{
	return const_iterator(_chunks_map.data()+_chunks_map.size(), _chunks_map.data()+_chunks_map.size());
}

controller::const_iterator controller::end() const noexcept
{
	return cend();
}

void controller::clear() noexcept
{
	_chunks_map = std::vector<full_chunk*>(_chunks_amount, nullptr);
	_status_flags = std::vector<bool>(_chunks_amount, false);
	_chunks.clear();
}

void controller::connect_processed() noexcept
{
	std::lock_guard lock(_chunks.chunk_gen_mtx);

	for(const auto chunk : _chunks.processed_chunks)
	{
		const vec3d<int>& c_pos = chunk->chunk.position();
		_chunks_map[index_chunk(c_pos)] = chunk;
		chunk->chunk.connect_observer(this);
		update_walls(c_pos, world_types::wall_states{});
	}

	_chunks.processed_chunks.clear();
}

void controller::generate_missing()
{
	for(int i = 0; i < _chunks_amount; ++i)
	{
		if(!_status_flags[i] && !exists(i))
		{
			_status_flags[i] = true;
			_chunk_gen_pool->run(index_position(i));
		}
	}
}

bool controller::reassign_chunks(const vec3d<int> pos) noexcept
{
	if(_center_pos==pos)
		return false;

	_chunk_gen_pool->exit_threads();

	const bool overlap = squares_overlap(pos);

	if(overlap)
	{
		//move all chunks in the opposite direction
		const vec3d<int> offset_pos = difference(pos) * vec3d<int>{-1, -1, -1};

		for(int x_i = 0; x_i < _row_size; ++x_i)
		{
			const int x = offset_pos.x<0 ? x_i : _row_size-1-x_i;
			for(int y_i = 0; y_i < _row_size; ++y_i)
			{
				const int y = offset_pos.y<0 ? y_i : _row_size-1-y_i;
				for(int z_i = 0; z_i < _row_size; ++z_i)
				{
					const int z = offset_pos.z<0 ? z_i : _row_size-1-z_i;
					move_chunk({x, y, z}, offset_pos);
				}
			}
		}
	} else
	{
		clear();
	}

	generate_pool();

	return true;
}

bool controller::squares_overlap(const vec3d<int> p2) const noexcept
{
	const vec3d<int>& p1 = _center_pos;
	const int& length = _row_size;

	const vec3d<int> p1_l{p1.x+length, p1.y+length, p1.z+length};
	const vec3d<int> p2_l{p2.x+length, p2.y+length, p2.z+length};

	if((p1.x>p2.x && p1_l.x<p2_l.x)
		|| (p1.y>p2.y && p1_l.y<p2_l.y)
		|| (p1.z>p2.z && p1_l.z<p2_l.z))
		return false;
	return true;
}

void controller::move_chunk(const vec3d<int> rel_pos, const vec3d<int> offset) noexcept
{
	const int c_index = index_local_chunk(rel_pos);
	const vec3d<int> move_pos = rel_pos+offset;

	if(in_local_bounds(move_pos))
	{
		const int move_index = index_local_chunk(move_pos);
		_chunks_map[move_index] = _chunks_map[c_index];
		_status_flags[move_index] = true;
	} else
	{
		_chunks.remove_chunk(*_chunks_map[c_index]);
	}

	_chunks_map[c_index] = nullptr;
	_status_flags[c_index] = false;
}

void controller::update_chunks(const vec3d<int> pos, const world_types::wall_states chunks) noexcept
{
	if(chunks.right)
		update_chunk({pos.x+1, pos.y, pos.z});

	if(chunks.left)
		update_chunk({pos.x-1, pos.y, pos.z});

	if(chunks.up)
		update_chunk({pos.x, pos.y+1, pos.z});

	if(chunks.down)
		update_chunk({pos.x, pos.y-1, pos.z});

	if(chunks.forward)
		update_chunk({pos.x, pos.y, pos.z+1});

	if(chunks.back)
		update_chunk({pos.x, pos.y, pos.z-1});
}

void controller::update_chunk(const vec3d<int> pos) noexcept
{
	if(contains(pos))
	{
		at(pos).model.update_mesh();
		update_walls(pos, world_types::wall_states{});
	}
}

void controller::update_walls(const vec3d<int> pos, const world_types::wall_states walls) noexcept
{
	if(walls.right)
		update_wall(pos, ytype::direction::right);

	if(walls.left)
		update_wall(pos, ytype::direction::left);

	if(walls.up)
		update_wall(pos, ytype::direction::up);

	if(walls.down)
		update_wall(pos, ytype::direction::down);

	if(walls.forward)
		update_wall(pos, ytype::direction::forward);

	if(walls.back)
		update_wall(pos, ytype::direction::back);
}

void controller::update_wall(const vec3d<int> pos, const ytype::direction wall) noexcept
{
	const int index = index_chunk(pos);
	if(in_bounds(pos) && exists(index))
	{
		const vec3d<int> side = pos+direction_offset(wall);
		const int side_index = index_chunk(side);
		if(in_bounds(side) && exists(side_index))
		{
			update_side(index, side_index, wall);
			update_side(side_index, index, direction_opposite(wall));
		}
	}
}

void controller::update_side(const int index, const int side_index, const ytype::direction wall) noexcept
{
	_chunks_map[index]->model.update_wall(_chunks_map[side_index]->chunk, wall);
}

bool controller::exists(const vec3d<int> pos) const noexcept
{
	return exists(index_chunk(pos));
}

bool controller::exists_local(const vec3d<int> rel_pos) const noexcept
{
	return exists(index_local_chunk(rel_pos));
}

bool controller::exists(const int index) const noexcept
{
	return _chunks_map[index]!=nullptr;
}

int controller::index_chunk(const vec3d<int> pos) const noexcept
{
	return index_local_chunk(position_local(pos));
}

int controller::index_local_chunk(const vec3d<int> rel_pos) const noexcept
{
	return rel_pos.x + rel_pos.y*_row_size + rel_pos.z*_row_size*_row_size;
}

vec3d<int> controller::index_position(const int index) const noexcept
{
	return vec3d<int>{index%_row_size+_center_pos.x-_render_size,
		(index/_row_size)%_row_size+_center_pos.y-_render_size,
		index/(_row_size*_row_size)+_center_pos.z-_render_size};
}

vec3d<int> controller::position_global(const vec3d<int> rel_pos) const noexcept
{
	return {rel_pos.x-_render_size+_center_pos.x,
		rel_pos.y-_render_size+_center_pos.y,
		rel_pos.z-_render_size+_center_pos.z};
}

vec3d<int> controller::position_local(const vec3d<int> pos) const noexcept
{
	return {(_render_size+pos.x-_center_pos.x),
		(_render_size+pos.y-_center_pos.y),
		(_render_size+pos.z-_center_pos.z)};
}