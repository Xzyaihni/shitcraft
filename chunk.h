#ifndef Y_CHUNK_H
#define Y_CHUNK_H

#include <vector>
#include <memory>
#include <mutex>

#include "wblock.h"

class chunk_observer
{
public:
	virtual ~chunk_observer() = default;

	virtual void block_notify(const vec3d<int> chunk, const vec3d<int> pos) = 0;
};

class world_chunk
{
public:
	typedef std::array<world_block, world_types::chunk_size*world_types::chunk_size*world_types::chunk_size> chunk_blocks;

	world_chunk();
	world_chunk(const vec3d<int> pos);
	
	void connect_observer(chunk_observer* observer) noexcept;
	void remove_observer(chunk_observer* observer) noexcept;

	void update_states();
	
	static vec3d<int> active_chunk(const vec3d<int> pos) noexcept;
	static vec3d<int> active_chunk(const vec3d<float> pos) noexcept;
	static vec3d<int> round_block(const vec3d<float> pos) noexcept;
	static vec3d<int> closest_bound_block(const vec3d<int> pos) noexcept;
	static vec3d<int> closest_bound_block(const vec3d<float> pos) noexcept;

	static world_types::wall_states block_sides(const vec3d<int> pos) noexcept;
	
	void set_block(const world_block block, const vec3d<int> pos) noexcept;

	world_block& block(const vec3d<int> pos) noexcept;
	const world_block& block(const vec3d<int> pos) const noexcept;
	
	void set_empty(const bool state) noexcept;
	bool empty() const noexcept;
	bool has_transparent() const noexcept;
	bool check_empty() const noexcept;
	
	const vec3d<int> position() const noexcept;

	static int index_block(const vec3d<int> pos) noexcept;

	chunk_blocks blocks;

private:
	void notify_observers(const vec3d<int> chunk, const vec3d<int> pos) noexcept;

	std::vector<chunk_observer*> _observers;

	vec3d<int> _position;
	
	bool _empty = true;
};

#endif
