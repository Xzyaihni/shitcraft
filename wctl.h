#ifndef WCTL_H
#define WCTL_H

#include <map>
#include <set>
#include <vector>

#include <glcyan.h>
#include <ythreads.h>

#include <GLFW/glfw3.h>

#include "chunk.h"
#include "character.h"
#include "wgen.h"

class world_controller
{
public:
	world_controller() {};
	world_controller(yanderegl::yandere_controller* yan_control,
	GLFWwindow* main_window, character* main_character, yanderegl::yandere_camera* main_camera,
	yanderegl::yandere_shader_program block_shader);
	
	void create_world(unsigned block_textures_id, unsigned seed);
	
	void destroy_block(const vec3d<int> chunk, const vec3d<int> block);
	void place_block(const vec3d<int> chunk, const vec3d<int> block_pos, const world_block block, const ytype::direction side);
	
	
	void chunk_update(full_chunk& f_chunk);
	void chunk_update_full(full_chunk& f_chunk, vec3d<int> block_pos);
	void chunk_update_full(full_chunk& f_chunk, world_types::wall_states walls);
	
	
	void wait_threads();
	
	void range_remove();
	
	void add_chunks();
	
	void full_update();
	
	void draw_update() const;
	
	void update_queued(std::map<vec3d<int>, world_types::wall_states>& update_chunks);
	void queue_updater(const std::map<vec3d<int>, world_types::wall_states>& queued_chunks);
	
	
	int render_dist();
	int chunk_radius();
	
	std::map<vec3d<int>, full_chunk> world_chunks;

private:
	void chunk_loader(const vec3d<int> pos);
	void wall_updater();

	float chunk_outside(const vec3d<int> pos) const;

	void init_chunks();
	
	GLFWwindow* _main_window;
	
	yanderegl::yandere_controller* _yan_control = nullptr;
	character* _main_character = nullptr;
	yanderegl::yandere_camera* _main_camera = nullptr;
	yanderegl::yandere_shader_program _block_shader;
	
	texture_atlas _atlas;
	
	std::unique_ptr<world_generator> _world_gen;
	
	std::vector<world_chunk> _processing_chunks;
	
	std::set<vec3d<int>> _been_loaded_chunks;
	
	std::map<vec3d<int>, world_types::wall_states> _queued_blocks;
	
	int _chunk_radius = 6;
	int _render_dist = 5;
	
	bool _world_created = false;
	bool _empty = true;
	
	typedef yanderethreads::yandere_pool<void (world_controller::*)(vec3d<int>), world_controller*, vec3d<int>> cgen_pool_type;
	std::unique_ptr<cgen_pool_type> _chunk_gen_pool;
	
	typedef yanderethreads::yandere_pool<void (world_controller::*)(), world_controller*> wall_pool_type;
	std::unique_ptr<wall_pool_type> _wall_update_pool;
	
	inline static std::mutex _process_chunks_mtx;
};

#endif
