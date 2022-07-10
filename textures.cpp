#include "textures.h"

texture_atlas::texture_atlas()
{
}

texture_atlas::texture_atlas(const yangl::generic_texture* texture,
	const int block_width, const int block_height)
: texture(texture),
width(texture->width()),
height(texture->height()),
block_width(block_width),
block_height(block_height),
blocks_horizontal(width/block_width),
blocks_vertical(height/block_height),
tex_offset_x(static_cast<float>(width*block_width)/(width*width)),
tex_offset_y(static_cast<float>(height*block_height)/(height*height))
{
}


graphics_state::graphics_state()
{
}

graphics_state::graphics_state(const yangl::camera* camera, const yangl::generic_shader* shader,
							   const yangl::core::texture* opaque_texture, const yangl::core::texture* transparent_texture)
: camera(camera), shader(shader),
opaque_atlas(opaque_texture, 16, 16),
transparent_atlas(transparent_texture, 16, 16)
{

}