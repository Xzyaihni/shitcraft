#ifndef PHYSICS_H
#define PHYSICS_H

#include <array>
#include <vector>
#include <map>
#include <mutex>

#include "types.h"

struct full_chunk;
class physics_controller;


struct raycast_result
{
	ytype::direction direction;
	vec3d<int> chunk;
	vec3d<int> block;
};

class physics_object
{
public:
	physics_object();
	physics_object(physics_controller* phys_ctl);
	
	vec3d<float> position = {0, 0, 0};
	vec3d<float> velocity = {0, 0, 0};
	vec3d<float> acceleration = {0, 0, 0};
	vec3d<float> force = {0, 0, 0};
	
	vec3d<float> rotation_axis = {1, 0, 0};
	float rotation = 0;
	
	vec3d<float> direction = {0, 0, 0};
	
	vec3d<int> active_chunk_pos;
	
	bool floating = false;
	bool is_static = false;
	
	float size = 1;
	float mass = 1;
	
	void update(double time_d);
	
private:
	static vec3d<float> apply_friction(vec3d<float> velocity, float friction);

	physics_controller* _phys_ctl = nullptr;
	
	//sphere's drag coefficient
	float drag_coefficient = 0.47f;
};

class physics_controller
{
public:
	physics_controller(std::map<vec3d<int>, full_chunk>& world_chunks);
	
	void physics_update(double time_d);
	
	raycast_result raycast(vec3d<float> start_pos, vec3d<float> direction, int length);
	raycast_result raycast(vec3d<float> start_pos, vec3d<float> end_pos);
	
	int raycast_distance(vec3d<float> ray_start, raycast_result raycast);
	
	std::vector<std::reference_wrapper<physics_object>> phys_objs;
	
	float air_density = 1.225f;
	vec3d<float> gravity = {0, -1, 0};
	
	static vec3d<float> calc_dir(float yaw, float pitch);
	
private:
	std::map<vec3d<int>, full_chunk>& _world_chunks;
};

#endif
