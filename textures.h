#ifndef Y_TEXTURES_H
#define Y_TEXTURES_H

#include "glcyan.h"

struct texture_atlas
{
	texture_atlas();
	texture_atlas(const yangl::generic_texture* texture,
		const int block_width, const int block_height);
	
	const yangl::generic_texture* texture;
	
	int width;
	int height;
	
	int block_width;
	int block_height;
	
	int blocks_horizontal;
	int blocks_vertical;
	
	float tex_offset_x;
	float tex_offset_y;
};

struct graphics_state
{
	graphics_state();
	graphics_state(const yangl::camera* camera, const yangl::generic_shader* shader,
				   const yangl::core::texture* opaque_texture, const yangl::core::texture* transparent_texture);

	const yangl::camera* camera = nullptr;
	const yangl::generic_shader* shader = nullptr;
	texture_atlas opaque_atlas;
	texture_atlas transparent_atlas;
};

#endif
