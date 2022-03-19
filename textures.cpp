#include "textures.h"

texture_atlas::texture_atlas(yanderegl::yandere_texture texture, int width, int height, int block_width, int block_height)
: texture(texture),
width(width), height(height), block_width(block_width), block_height(block_height),
blocks_horizontal(width/block_width), blocks_vertical(height/block_height),
tex_offset_x(static_cast<float>(width*block_width)/(width*width)), tex_offset_y(static_cast<float>(height*block_height)/(height*height))
{
}
