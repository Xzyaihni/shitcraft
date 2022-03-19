#ifndef Y_CHUNK_H
#define Y_CHUNK_H

#include <vector>
#include <memory>
#include <mutex>

#include <glcyan.h>

#include "wblock.h"
#include "textures.h"

class world_chunk
{
public:
	typedef std::array<world_block, chunk_size*chunk_size*chunk_size> chunk_blocks;

	world_chunk(const vec3d<int> pos);
	
	void update_states();
	
	static vec3d<int> active_chunk(const vec3d<int> pos) noexcept;
	static vec3d<int> active_chunk(const vec3d<float> pos) noexcept;
	static vec3d<int> closest_block(const vec3d<float> pos) noexcept;
	
	world_block& block(const vec3d<int> pos) noexcept;
	const world_block& block(const vec3d<int> pos) const noexcept;
	
	void set_empty(const bool state) noexcept;
	bool empty() const noexcept;
	bool has_transparent() const noexcept;
	bool check_empty() const noexcept;
	
	int plants_amount() const noexcept;
	void set_plants_amount(const int amount) noexcept;
	
	const vec3d<int> position() const noexcept;

	chunk_blocks blocks;

private:
	
	vec3d<int> _position;
	
	bool _empty = true;
	
	int _plants_amount = 0;
};

class model_chunk
{
private:
	struct mutex_storage
	{
		mutable std::mutex mtx;
	
		mutex_storage() {};
		mutex_storage(const mutex_storage&) {};
		mutex_storage(mutex_storage&&) noexcept {};
		mutex_storage& operator=(const mutex_storage&) {return *this;};
		mutex_storage& operator=(mutex_storage&&) noexcept {return *this;};
	};

public:
	model_chunk(world_chunk& owner, yanderegl::yandere_shader_program shader, const texture_atlas& atlas);

	void update_mesh() noexcept;
	
	void update_wall(const ytype::direction wall, const world_chunk& check_chunk) noexcept;

	void draw() const noexcept;
	
	world_types::wall_states walls() const noexcept;
	bool walls_full() const noexcept;

private:
	void update_block_walls(const vec3d<int> pos) noexcept;
	void update_block_walls(const vec3d<int> pos, const int index) noexcept;

	void update_right_wall(const world_chunk& check_chunk) noexcept;
	void update_left_wall(const world_chunk& check_chunk) noexcept;
	void update_up_wall(const world_chunk& check_chunk) noexcept;
	void update_down_wall(const world_chunk& check_chunk) noexcept;
	void update_forward_wall(const world_chunk& check_chunk) noexcept;
	void update_back_wall(const world_chunk& check_chunk) noexcept;

	void a_forward_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_back_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_left_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_right_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_up_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_down_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	
	world_chunk& _own_chunk;
	
	unsigned _index_offset;

	yanderegl::yandere_object _draw_object;
	yanderegl::core::model _model;
	
	world_types::wall_states _walls_empty;
	bool _walls_full = false;
	
	float _texture_distance_x;
	float _texture_distance_y;
	
	mutex_storage _model_mutex;
};

struct full_chunk
{
	full_chunk(world_chunk chunk, yanderegl::yandere_shader_program shader, const texture_atlas& atlas);

	world_chunk chunk;
	model_chunk model;
};

#endif
