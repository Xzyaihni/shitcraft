#ifndef Y_CMODEL_H
#define Y_CMODEL_H

#include <glcyan.h>

#include "textures.h"
#include "chunk.h"

class model_holder
{
public:
	static constexpr float block_size = 1.0f/static_cast<float>(world_types::chunk_size);

	model_holder();
	model_holder(const yangl::camera* cam, const yangl::generic_shader* shader, const texture_atlas& atlas);

	model_holder(const model_holder&);
	model_holder(model_holder&&) noexcept;
	model_holder& operator=(const model_holder&);
	model_holder& operator=(model_holder&&) noexcept;

	void set_position(const vec3d<float> pos) noexcept;
	void set_scale(const float scale) noexcept;

	void draw() noexcept;

	void clear() noexcept;

	void a_forward_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_back_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_left_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_right_face( const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_up_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;
	void a_down_face(const vec3d<float> pos_f, const world_types::tex_pos texture_pos) noexcept;

private:
	yangl::core::model_manual _model;
	yangl::generic_object _draw_object;

	int _index;

	float _distance_x;
	float _distance_y;
};

class model_chunk
{
public:
	model_chunk();
	model_chunk(const world_chunk* owner,
		const graphics_state& graphics);

	void update_mesh() noexcept;
	
	void update_wall(const world_chunk& check_chunk, const ytype::direction wall) noexcept;

	void draw_opaque() noexcept;
	void draw_transparent() noexcept;
	
	world_types::wall_states walls() const noexcept;
	bool walls_full() const noexcept;

	void set_owner(const world_chunk* owner) noexcept;

private:
	void update_block_walls(const vec3d<int> pos) noexcept;
	void update_block_walls(const vec3d<int> pos, const int index) noexcept;

	template<bool right_wall>
	void update_wall_x(const world_chunk& check_chunk) noexcept;
	template<bool up_wall>
	void update_wall_y(const world_chunk& check_chunk) noexcept;
	template<bool forward_wall>
	void update_wall_z(const world_chunk& check_chunk) noexcept;

	static bool draw_side(const world_block& block, const world_block& check) noexcept;
	
	const world_chunk* _own_chunk = nullptr;

	model_holder _opaque_model;
	model_holder _transparent_model;
	
	world_types::wall_states _walls_empty;
	bool _walls_full = false;
};

struct full_chunk
{
	full_chunk();
	full_chunk(const world_chunk chunk,
		const graphics_state& graphics);

	full_chunk(const full_chunk&);
	full_chunk(full_chunk&&) noexcept;
	full_chunk& operator=(const full_chunk&);
	full_chunk& operator=(full_chunk&&) noexcept;

	world_chunk chunk;
	model_chunk model;
};

#endif
