#ifndef Y_TEXTURES_H
#define Y_TEXTURES_H

#include "glcyan.h"

struct texture_atlas
{
	texture_atlas() {};
	texture_atlas(yanderegl::yandere_texture texture, int width, int height, int block_width, int block_height);
	
	yanderegl::yandere_texture texture;
	
	int width;
	int height;
	
	int block_width;
	int block_height;
	
	int blocks_horizontal;
	int blocks_vertical;
	
	float tex_offset_x;
	float tex_offset_y;
};

#endif
