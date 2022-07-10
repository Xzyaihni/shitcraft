#include <execution>
#include <utility>
#include <tuple>
#include <iostream>

#include "wctl.h"


using namespace ytype;
using namespace world_types;

world_controller::world_controller()
{
}

world_controller::world_controller(const GLFWwindow* main_window,
	const character* main_character, const graphics_state graphics)
: _main_window(main_window),
_main_character(main_character), _main_camera(graphics.camera), _empty(false)
{
	_world_gen = std::make_unique<world_generator>(graphics);
	world_chunks = cmap::controller(_world_gen.get(), _chunk_radius, main_character->active_chunk());

	full_update();
}


void world_controller::full_update()
{
	world_chunks.update();
	world_chunks.update_center(_main_character->active_chunk());
}

void world_controller::draw_update()
{
	std::multimap<float, std::reference_wrapper<model_chunk>> distance_models;

	for(auto& f_chunk : world_chunks)
	{
		if(f_chunk.chunk.empty())
			continue;

		f_chunk.model.draw_opaque();
	
		const vec3d<int> chunk_pos = f_chunk.chunk.position();
		const vec3d<int> c_pos = _main_character->active_chunk();
		const vec3d<int> c_rel_pos = chunk_pos-c_pos;

		const float chunk_distance = std::pow(c_rel_pos.x, 2)
			+ std::pow(c_rel_pos.y, 2)
			+ std::pow(c_rel_pos.z, 2);
		
		const int render_dist_squared = _render_dist*_render_dist;

		//check if chunk is too far away to render
		if(chunk_distance > render_dist_squared)
			continue;

		//check if the chunk is in the camera frustum
		const vec3d<float> check_pos_f =
			(chunk_pos).cast<float>()*chunk_size
			+vec3d<float>{chunk_size/2, chunk_size/2, chunk_size/2};

		if(!_main_camera->cube_in_frustum({check_pos_f.x, check_pos_f.y, check_pos_f.z}, chunk_size))
			continue;
		
		const vec3d<float> c_fpos = c_rel_pos.cast<float>();
		const float direction_distance = _main_camera->distance({c_fpos.x, c_fpos.y, c_fpos.z});
		distance_models.insert({direction_distance, f_chunk.model});
	}

	const auto iter_end = distance_models.rend();
	for(auto iter = distance_models.rbegin(); iter!=iter_end; ++iter)
	{
		iter->second.get().draw_transparent();
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

float world_controller::chunk_outside(const vec3d<int> pos) const
{
	const vec3d<int> active_chunk = _main_character->active_chunk();
	return (vec3d<int>{std::abs(pos.x), std::abs(pos.y), std::abs(pos.z)}
		- vec3d<int>{std::abs(active_chunk.x), std::abs(active_chunk.y), std::abs(active_chunk.z)}).magnitude();
}
