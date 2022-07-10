#ifndef PHYSICS_H
#define PHYSICS_H

#include <array>
#include <vector>
#include <map>
#include <mutex>

#include "types.h"
#include "cmap.h"


namespace physics
{
	class box_collider
	{
	public:
		box_collider();
		box_collider(const vec3d<float> size);
		virtual ~box_collider() = default;

		vec3d<float> size;
	};

	class world
	{
	public:
		world(const cmap::controller& world_chunks);
		virtual ~world() = default;

		bool collision_point(const vec3d<float> pos) const noexcept;

	protected:
		const cmap::controller& _world_chunks;
	};

	struct raycast_result
	{
		ytype::direction direction;
		vec3d<int> chunk;
		vec3d<int> block;
	};

	class raycaster : public world
	{
	public:
		using world::world;

		raycast_result raycast(const vec3d<float> start_pos, const vec3d<float> end_pos) const;
		raycast_result raycast(const vec3d<float> start_pos, const vec3d<float> direction, int length) const;

		int raycast_distance(const vec3d<float> ray_start, const raycast_result raycast) const;

	private:
		struct ray_info
		{
			vec3d<float>& change;
			vec3d<bool>& side;
			vec3d<int>& chunk;
			vec3d<int>& block;
		};

		static bool next_block(ray_info ray, const vec3d<float> direction) noexcept;

		template<bool x_b, bool y_b, bool z_b>
		static bool next_block_calcs(ray_info ray, const vec3d<float> direction, const vec3d<float> normalized_direction, const vec3d<float> wall_dist) noexcept;

		static bool calculate_next(const bool positive, int& chunk, int& block) noexcept;

		static vec3d<float> distance_change(const vec3d<float> start_pos, const vec3d<float> direction, const vec3d<float> fraction) noexcept;
		static vec3d<float> fraction(const vec3d<float> val) noexcept;
		static float fraction(const float val) noexcept;
	};

	class world_observer
	{
	public:
		virtual ~world_observer() = default;

		virtual void set_environment(const vec3d<float> gravity, const float air_density) = 0;
		virtual void update(const double delta) = 0;
	};

	class object : public box_collider, public world_observer
	{
	public:
		object();
		object(const raycaster* rayctl);

		void set_raycaster(const raycaster* rayctl) noexcept;

		void set_environment(const vec3d<float> gravity, const float air_density);

		void update(const double delta) noexcept;

		vec3d<int> active_chunk() const noexcept;

		vec3d<float> position = {0, 0, 0};
		vec3d<float> velocity = {0, 0, 0};
		vec3d<float> acceleration = {0, 0, 0};
		vec3d<float> force = {0, 0, 0};

		vec3d<float> rotation_axis = {1, 0, 0};
		float rotation = 0;

		vec3d<float> direction = {0, 0, 0};

		bool floating = false;
		bool is_static = false;

		float mass = 1;

	private:
		static vec3d<float> apply_friction(vec3d<float> velocity, const float friction);

		const raycaster* _raycaster = nullptr;

		vec3d<int> _active_chunk_pos = {0, 0, 0};

		//sphere's drag coefficient
		float _drag_coefficient = 0.47f;

		vec3d<float> _gravity;
		float _air_density;
	};

	class controller
	{
	public:
		controller();

		void connect_object(world_observer* observer);
		void remove_object(world_observer* observer);

		void physics_update(const double delta);

		static vec3d<float> calc_dir(const float yaw, const float pitch);

		float air_density = 1.225f;
		vec3d<float> gravity = {0, -1, 0};

	private:
		std::vector<world_observer*> _physics_objs;
	};
};

#endif
