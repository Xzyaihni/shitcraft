#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

#include "physics.h"
#include "chunk.h"


using namespace world_types;

physics_object::physics_object()
{
}

physics_object::physics_object(physics_controller* phys_ctl) : _phys_ctl(phys_ctl)
{
}

void physics_object::update(double time_d)
{
	assert(_phys_ctl!=nullptr);

	active_chunk_pos = world_chunk::active_chunk(position);

	if(!is_static)
	{
		if(!floating)
		{
			acceleration += _phys_ctl->gravity;
		}

		vec3d<float> new_position;
		vec3d<float> new_velocity;

		float cross_section_area = M_PI*(size*size/4);
		
		vec3d<float> air_resistance = 0.5f*drag_coefficient*_phys_ctl->air_density*cross_section_area*(velocity*velocity);
		air_resistance.x = velocity.x>0 ? -air_resistance.x : air_resistance.x;
		air_resistance.y = velocity.y>0 ? -air_resistance.y : air_resistance.y;
		air_resistance.z = velocity.z>0 ? -air_resistance.z : air_resistance.z;
		
		vec3d<float> net_force_accel = (force+air_resistance)/mass;
		
		new_velocity = velocity + (acceleration+net_force_accel)*time_d;
		new_position = position + velocity*time_d;
		
		raycast_result x_raycast = _phys_ctl->raycast(position, {new_position.x, position.y, position.z});
		raycast_result y_raycast = _phys_ctl->raycast(position, {position.x, new_position.y, position.z});
		raycast_result z_raycast = _phys_ctl->raycast(position, {position.x, position.y, new_position.z});
		
		if(x_raycast.direction!=ytype::direction::none)
		{
			new_position.x = position.x;
			new_velocity.x = 0;
		}
		
		if(y_raycast.direction!=ytype::direction::none)
		{
			new_position.y = position.y;
			new_velocity.y = 0;
		}

		if(z_raycast.direction!=ytype::direction::none)
		{
			new_position.z = position.z;
			new_velocity.z = 0;
		}
		
		
		position = new_position;
		velocity = new_velocity;
	}
}

vec3d<float> physics_object::apply_friction(vec3d<float> velocity, float friction)
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


physics_controller::physics_controller(std::map<vec3d<int>, full_chunk>& world_chunks)
: _world_chunks(world_chunks)
{
}

void physics_controller::physics_update(double time_d)
{
	std::for_each(phys_objs.begin(), phys_objs.end(), [time_d](auto& obj){obj.get().update(time_d);});
}


raycast_result physics_controller::raycast(vec3d<float> start_pos, vec3d<float> end_pos)
{
	vec3d<int> vec_difference = world_chunk::closest_block(end_pos) - world_chunk::closest_block(start_pos);
	
	int length = std::abs(vec_difference.x)+std::abs(vec_difference.y)+std::abs(vec_difference.z);
	
	if(length==0)
		return raycast_result{ytype::direction::none, {0, 0, 0}, {0, 0, 0}};
	
	return raycast(start_pos, end_pos-start_pos, length);
}

raycast_result physics_controller::raycast(vec3d<float> start_pos, vec3d<float> direction, int length)
{
	raycast_result ray_result;
	
	vec3d<int> c_chunk_pos = world_chunk::active_chunk(start_pos);
	
	vec3d<float> start_block_pos = vec3d<float>{static_cast<float>(std::fmod(start_pos.x, chunk_size)), static_cast<float>(std::fmod(start_pos.y, chunk_size)), static_cast<float>(std::fmod(start_pos.z, chunk_size))};
	start_block_pos.x = start_pos.x<0 ? chunk_size+start_block_pos.x : start_block_pos.x;
	start_block_pos.y = start_pos.y<0 ? chunk_size+start_block_pos.y : start_block_pos.y;
	start_block_pos.z = start_pos.z<0 ? chunk_size+start_block_pos.z : start_block_pos.z;
	
	direction /= vec3d<float>::magnitude(direction); //normalize it
	vec3d<float> direction_abs{std::abs(direction.x), std::abs(direction.y), std::abs(direction.z)};
	
	vec3d<int> c_check_pos = world_chunk::closest_block(start_block_pos);
	vec3d<bool> move_side{false, false, false};
	
	
	float temp;
	
	vec3d<float> fractional_pos{std::abs(std::modf(start_pos.x, &temp)), std::abs(std::modf(start_pos.y, &temp)), std::abs(std::modf(start_pos.z, &temp))};
	
	vec3d<float> position_change;
	if(start_pos.x<0)
	{
		position_change.x = direction.x<0 ? fractional_pos.x : 1-fractional_pos.x;
	} else
	{
		position_change.x = direction.x<0 ? 1-fractional_pos.x : fractional_pos.x;
	}
	
	if(start_pos.y<0)
	{
		position_change.y = direction.y<0 ? fractional_pos.y : 1-fractional_pos.y;
	} else
	{
		position_change.y = direction.y<0 ? 1-fractional_pos.y : fractional_pos.y;
	}
	
	if(start_pos.z<0)
	{
		position_change.z = direction.z<0 ? fractional_pos.z : 1-fractional_pos.z;
	} else
	{
		position_change.z = direction.z<0 ? 1-fractional_pos.z : fractional_pos.z;
	}
	
	while(true)
	{
		const auto c_iter = _world_chunks.find(c_chunk_pos);
		if(c_iter==_world_chunks.end())
			return raycast_result{ytype::direction::none, {0, 0, 0}, {0, 0, 0}};
	
		const world_chunk& c_chunk = c_iter->second.chunk;
		while(true)
		{
			if(c_chunk.block(c_check_pos).block_type!=block::air)
				return raycast_result{(move_side.x?(direction.x<0? ytype::direction::left : ytype::direction::right):(move_side.y?(direction.y<0? ytype::direction::down : ytype::direction::up):(direction.z<0? ytype::direction::back : ytype::direction::forward))), c_chunk_pos, c_check_pos};
			
			if(length==0)
				return raycast_result{ytype::direction::none, {0, 0, 0}, {0, 0, 0}};
			
			vec3d<float> wall_dist;
			
			wall_dist.x = direction.x==0 ? INFINITY : (1-position_change.x)/direction_abs.x;
			wall_dist.y = direction.y==0 ? INFINITY : (1-position_change.y)/direction_abs.y;
			wall_dist.z = direction.z==0 ? INFINITY : (1-position_change.z)/direction_abs.z;
			
						
			--length;
			if(wall_dist.x <= wall_dist.y && wall_dist.x <= wall_dist.z)
			{
				//x wall is closest
				position_change.x = 0;
				
				position_change.y += std::abs(wall_dist.x*direction.y);
				position_change.y = position_change.y>1 ? 1 : position_change.y;
				
				position_change.z += std::abs(wall_dist.x*direction.z);
				position_change.z = position_change.z>1 ? 1 : position_change.z;
				
				move_side = {true, false, false};
				if(direction.x<0)
				{
					if(c_check_pos.x==0)
					{
						c_check_pos.x = chunk_size-1;
						--c_chunk_pos.x;
						break;
					} else
					{
						--c_check_pos.x;
					}
				} else 
				{
					if(c_check_pos.x==chunk_size-1)
					{
						c_check_pos.x = 0;
						++c_chunk_pos.x;
						break;
					} else
					{
						++c_check_pos.x;
					}
				}
			} else if(wall_dist.y <= wall_dist.x && wall_dist.y <= wall_dist.z)
			{
				//y wall is closest
				position_change.x += std::abs(wall_dist.y*direction.x);
				position_change.x = position_change.x>1 ? 1 : position_change.x;
				
				position_change.y = 0;
				
				position_change.z += std::abs(wall_dist.y*direction.z);
				position_change.z = position_change.z>1 ? 1 : position_change.z;
				
				move_side = {false, true, false};
				if(direction.y<0)
				{
					if(c_check_pos.y==0)
					{
						c_check_pos.y = chunk_size-1;
						--c_chunk_pos.y;
						break;
					} else
					{
						--c_check_pos.y;
					}
				} else 
				{
					if(c_check_pos.y==chunk_size-1)
					{
						c_check_pos.y = 0;
						++c_chunk_pos.y;
						break;
					} else
					{
						++c_check_pos.y;
					}
				}
			} else
			{
				//z wall is closest
				position_change.x += std::abs(wall_dist.z*direction.x);
				position_change.x = position_change.x>1 ? 1 : position_change.x;
				
				position_change.y += std::abs(wall_dist.z*direction.y);
				position_change.y = position_change.y>1 ? 1 : position_change.y;
				
				position_change.z = 0;
				
				move_side = {false, false, true};
				if(direction.z<0)
				{
					if(c_check_pos.z==0)
					{
						c_check_pos.z = chunk_size-1;
						--c_chunk_pos.z;
						break;
					} else
					{
						--c_check_pos.z;
					}
				} else 
				{
					if(c_check_pos.z==chunk_size-1)
					{
						c_check_pos.z = 0;
						++c_chunk_pos.z;
						break;
					} else
					{
						++c_check_pos.z;
					}
				}
			}
		}
	}
}

int physics_controller::raycast_distance(vec3d<float> ray_start, raycast_result raycast)
{
	vec3d<float> chunk_offset = vec3d_cvt<float>((raycast.chunk-world_chunk::active_chunk(ray_start))*chunk_size);
	
	vec3d<float> in_chunk_pos{
	static_cast<float>(std::fmod(ray_start.x, chunk_size)),
	static_cast<float>(std::fmod(ray_start.y, chunk_size)),
	static_cast<float>(std::fmod(ray_start.z, chunk_size))};
	
	in_chunk_pos.x = in_chunk_pos.x<0 ? chunk_size+in_chunk_pos.x : in_chunk_pos.x;
	in_chunk_pos.y = in_chunk_pos.y<0 ? chunk_size+in_chunk_pos.y : in_chunk_pos.y;
	in_chunk_pos.z = in_chunk_pos.z<0 ? chunk_size+in_chunk_pos.z : in_chunk_pos.z;
	
	chunk_offset = chunk_offset+vec3d_cvt<float>(raycast.block)-in_chunk_pos;

	return vec3d<float>::magnitude(chunk_offset);
}

vec3d<float> physics_controller::calc_dir(float yaw, float pitch)
{
	return {std::cos(yaw)*std::cos(pitch), std::sin(pitch), std::cos(pitch)*std::sin(yaw)};
}
