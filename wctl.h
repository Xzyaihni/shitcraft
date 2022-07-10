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
#include "cmodel.h"
#include "cmap.h"

class world_controller
{
public:
	world_controller();
	world_controller(const GLFWwindow* main_window,
	const character* main_character, const graphics_state graphics);
	
	void full_update();
	
	void draw_update();
	
	
	int render_dist();
	int chunk_radius();

	cmap::controller world_chunks;

private:
	float chunk_outside(const vec3d<int> pos) const;

	void init_chunks();
	
	const GLFWwindow* _main_window = nullptr;
	const yangl::camera* _main_camera = nullptr;
	const character* _main_character = nullptr;

	std::unique_ptr<world_generator> _world_gen;
	
	std::vector<world_chunk> _processing_chunks;
	
	std::map<vec3d<int>, world_types::wall_states> _queued_blocks;
	
	int _chunk_radius = 6;
	int _render_dist = 5;
	
	bool _empty = true;
};

#endif
