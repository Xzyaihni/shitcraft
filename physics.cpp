#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

#include "physics.h"
#include "chunk.h"


using namespace world_types;
using namespace physics;

box_collider::box_collider()
{
}

box_collider::box_collider(const vec3d<float> size)
: size(size)
{
}

world::world(const cmap::controller& world_chunks)
: _world_chunks(world_chunks)
{
}

bool world::collision_point(const vec3d<float> pos) const noexcept
{
	return _world_chunks.at(world_chunk::active_chunk(pos)).chunk.block
		(world_chunk::closest_bound_block(pos)).block_type!=block::air;
}

raycast_result raycaster::raycast(const vec3d<float> start_pos, const vec3d<float> end_pos) const
{
	const vec3d<int> vec_difference = world_chunk::closest_bound_block(end_pos)
		- world_chunk::closest_bound_block(start_pos);

	const int length = std::abs(vec_difference.x)+std::abs(vec_difference.y)+std::abs(vec_difference.z);

	if(length==0)
		return raycast_result{ytype::direction::none, {0, 0, 0}, {0, 0, 0}};

	return raycast(start_pos, end_pos-start_pos, length);
}

raycast_result raycaster::raycast(const vec3d<float> start_pos, const vec3d<float> direction, int length) const
{
	vec3d<int> c_chunk_pos = world_chunk::active_chunk(start_pos);
	vec3d<int> c_block_pos = world_chunk::closest_bound_block(start_pos);

	vec3d<bool> move_side{false, false, false};


	const vec3d<float> fractional_pos{fraction(start_pos)};

	vec3d<float> position_change = distance_change(start_pos, direction, fractional_pos);

	while(true)
	{
		const auto c_iter = _world_chunks.find(c_chunk_pos);
		if(c_iter==_world_chunks.end())
			return raycast_result{ytype::direction::none, {0, 0, 0}, {0, 0, 0}};

		const world_chunk& c_chunk = c_iter->chunk;
		while(true)
		{
			if(!c_chunk.empty() && c_chunk.block(c_block_pos).block_type!=block::air)
				return raycast_result{(move_side.x?
					(direction.x<0? ytype::direction::left : ytype::direction::right)
					:(move_side.y?(direction.y<0? ytype::direction::down : ytype::direction::up)
					:(direction.z<0? ytype::direction::back : ytype::direction::forward))),
					c_chunk_pos, c_block_pos};

			if(length==0)
				return raycast_result{ytype::direction::none, {0, 0, 0}, {0, 0, 0}};

			--length;
			if(next_block({position_change, move_side, c_chunk_pos, c_block_pos}, direction))
				break;
		}
	}
}

int raycaster::raycast_distance(const vec3d<float> ray_start, const raycast_result raycast) const
{
	vec3d<float> in_chunk_pos{
		static_cast<float>(std::fmod(ray_start.x, chunk_size)),
		static_cast<float>(std::fmod(ray_start.y, chunk_size)),
		static_cast<float>(std::fmod(ray_start.z, chunk_size))};

	in_chunk_pos.x = in_chunk_pos.x<0 ? chunk_size+in_chunk_pos.x : in_chunk_pos.x;
	in_chunk_pos.y = in_chunk_pos.y<0 ? chunk_size+in_chunk_pos.y : in_chunk_pos.y;
	in_chunk_pos.z = in_chunk_pos.z<0 ? chunk_size+in_chunk_pos.z : in_chunk_pos.z;

	const vec3d<float> chunk_offset =
		((raycast.chunk-world_chunk::active_chunk(ray_start))*chunk_size).cast<float>()
		+ raycast.block.cast<float>() - in_chunk_pos;

	return chunk_offset.magnitude();
}

bool raycaster::next_block(ray_info ray, const vec3d<float> direction) noexcept
{
	const vec3d<float> normalized_direction = direction.normalize();
	const vec3d<float> direction_abs = normalized_direction.abs();

	const vec3d<float> wall_dist{
		(direction.x==0 ? INFINITY : (1-ray.change.x)/direction_abs.x),
		(direction.y==0 ? INFINITY : (1-ray.change.y)/direction_abs.y),
		(direction.z==0 ? INFINITY : (1-ray.change.z)/direction_abs.z)};


	if(wall_dist.x <= wall_dist.y && wall_dist.x <= wall_dist.z)
	{
		return next_block_calcs<true, false, false>(ray, direction, normalized_direction, wall_dist);
	} else if(wall_dist.y <= wall_dist.x && wall_dist.y <= wall_dist.z)
	{
		return next_block_calcs<false, true, false>(ray, direction, normalized_direction, wall_dist);
	} else
	{
		return next_block_calcs<false, false, true>(ray, direction, normalized_direction, wall_dist);
	}
}

template<bool x_b, bool y_b, bool z_b>
bool raycaster::next_block_calcs(ray_info ray, const vec3d<float> direction, const vec3d<float> normalized_direction, const vec3d<float> wall_dist) noexcept
{
	ray.side = {x_b, y_b, z_b};

	const float c_dist = x_b ? wall_dist.x : y_b ? wall_dist.y : wall_dist.z;

	vec3d<float>& change = ray.change;

	if(x_b)
		change.x = 0;
	else
	{
		change.x += std::abs(c_dist*normalized_direction.x);
		change.x = change.x>1 ? 1 : change.x;
	}

	if(y_b)
		change.y = 0;
	else
	{
		change.y += std::abs(c_dist*normalized_direction.y);
		change.y = change.y>1 ? 1 : change.y;
	}

	if(z_b)
		change.z = 0;
	else
	{
		change.z += std::abs(c_dist*normalized_direction.z);
		change.z = change.z>1 ? 1 : change.z;
	}

	int& c_block = x_b ? ray.block.x : y_b ? ray.block.y : ray.block.z;
	int& c_chunk = x_b ? ray.chunk.x : y_b ? ray.chunk.y : ray.chunk.z;

	const bool c_direction = x_b ? direction.x<0 : y_b ? direction.y<0 : direction.z<0;

	return calculate_next(c_direction, c_chunk, c_block);
}

bool raycaster::calculate_next(const bool positive, int& chunk, int& block) noexcept
{
	if(positive)
	{
		--block;
		if(block==-1)
		{
			block = chunk_size-1;
			--chunk;
			return true;
		}
	} else
	{
		++block;
		if(block==chunk_size)
		{
			block = 0;
			++chunk;
			return true;
		}
	}

	return false;
}

vec3d<float> raycaster::distance_change(const vec3d<float> start_pos, const vec3d<float> direction, const vec3d<float> fraction) noexcept
{
	vec3d<float> change;

	if(start_pos.x<0)
	{
		change.x = direction.x<0 ? fraction.x : 1-fraction.x;
	} else
	{
		change.x = direction.x<0 ? 1-fraction.x : fraction.x;
	}

	if(start_pos.y<0)
	{
		change.y = direction.y<0 ? fraction.y : 1-fraction.y;
	} else
	{
		change.y = direction.y<0 ? 1-fraction.y : fraction.y;
	}

	if(start_pos.z<0)
	{
		change.z = direction.z<0 ? fraction.z : 1-fraction.z;
	} else
	{
		change.z = direction.z<0 ? 1-fraction.z : fraction.z;
	}

	return change;
}

vec3d<float> raycaster::fraction(const vec3d<float> val) noexcept
{
	return vec3d<float>{fraction(val.x), fraction(val.y), fraction(val.z)};
}

float raycaster::fraction(const float val) noexcept
{
	float temp;
	return std::abs(std::modf(val, &temp));
}

object::object()
: box_collider({1, 1, 1})
{
}

object::object(const raycaster* rayctl)
: _raycaster(rayctl), box_collider({1, 1, 1})
{
}

void object::set_raycaster(const raycaster* rayctl) noexcept
{
	_raycaster = rayctl;
}

void object::set_environment(const vec3d<float> gravity, const float air_density)
{
	_gravity = gravity;
	_air_density = air_density;
}

void object::update(const double delta) noexcept
{
	assert(_raycaster!=nullptr);

	if(!is_static)
	{
		if(!floating)
		{
			acceleration += _gravity;
		}

		const float cross_section_area = M_PI*(size.x*size.y/4);

		vec3d<float> air_resistance = 0.5f*_drag_coefficient*_air_density*cross_section_area*(velocity*velocity);
		air_resistance.x = velocity.x>0 ? -air_resistance.x : air_resistance.x;
		air_resistance.y = velocity.y>0 ? -air_resistance.y : air_resistance.y;
		air_resistance.z = velocity.z>0 ? -air_resistance.z : air_resistance.z;

		const vec3d<float> net_force_accel = (force+air_resistance)/mass;

		vec3d<float> new_velocity = velocity + (acceleration+net_force_accel)*delta;
		vec3d<float> new_position = position + velocity*delta;

		const vec3d<int> closest_new_pos = world_chunk::round_block(new_position);
		const vec3d<int> closest_pos = world_chunk::round_block(position);

		const vec3d<int> diff_pos = closest_new_pos-closest_pos;


		/*if(diff_pos.x!=0)
		{
			const raycast_result x_raycast = _raycaster->raycast_x(position, diff_pos.x);
			if(x_raycast.direction!=ytype::direction::none)
			{
				new_position.x = position.x;
				new_velocity.x = 0;
			}
		}

		if(diff_pos.y!=0)
		{
			const raycast_result y_raycast = _raycaster->raycast_y(position, diff_pos.y);
			if(y_raycast.direction!=ytype::direction::none)
			{
				new_position.y = position.y;
				new_velocity.y = 0;
			}
		}

		if(diff_pos.z!=0)
		{
			const raycast_result z_raycast = _raycaster->raycast_z(position, diff_pos.z);
			if(z_raycast.direction!=ytype::direction::none)
			{
				new_position.z = position.z;
				new_velocity.z = 0;
			}
		}*/


		position = new_position;
		velocity = new_velocity;
	}
}

vec3d<int> object::active_chunk() const noexcept
{
	return world_chunk::active_chunk(position);
}

vec3d<float> object::apply_friction(vec3d<float> velocity, const float friction)
{
	if(velocity.x>0)
	{
		velocity.x = velocity.x>=friction ? velocity.x - friction : 0;
	} else
	{
		velocity.x = velocity.x<=-friction ? velocity.x + friction : 0;
	}
	
	if(velocity.y>0)
	{
		velocity.y = velocity.y>=friction ? velocity.y - friction : 0;
	} else
	{
		velocity.y = velocity.y<=-friction ? velocity.y + friction : 0;
	}
	
	if(velocity.z>0)
	{
		velocity.z = velocity.z>=friction ? velocity.z - friction : 0;
	} else
	{
		velocity.z = velocity.z<=-friction ? velocity.z + friction : 0;
	}
	
	return velocity;
}


controller::controller()
{
}

void controller::connect_object(world_observer* observer)
{
	_physics_objs.push_back(observer);
}

void controller::remove_object(world_observer* observer)
{
	_physics_objs.erase(std::find(_physics_objs.begin(), _physics_objs.end(), observer));
}

void controller::physics_update(const double delta)
{
	std::for_each(_physics_objs.begin(), _physics_objs.end(), [delta](world_observer* observer)
	{
		observer->update(delta);
	});
}

vec3d<float> controller::calc_dir(const float yaw, const float pitch)
{
	return {std::cos(yaw)*std::cos(pitch), std::sin(pitch), std::cos(pitch)*std::sin(yaw)};
}
