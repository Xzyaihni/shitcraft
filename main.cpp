#include <iostream>
#include <functional>
#include <array>
#include <vector>
#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <tuple>
#include <map>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <future>

#include "chunk.h"
#include "character.h"
#include "types.h"
#include "physics.h"
#include "wctl.h"

#include "ygui.h"

using namespace yangl;
using namespace ytype;
using namespace world_types;

class game_controller
{
private:
	enum ykey {forward = 0, back, right, left, jump, crouch, yLAST};
	enum text_id {xpos = 0, ypos, zpos, fps, tLAST};

public:
	game_controller();
	void graphics_init();
	
	void draw_update();
	void update_func();
	
	void slow_update();
	
	void mouse_update(int button, int state, int x, int y);
	
	void control_func(bool mouse, int key, int action, int mods, int scancode);
	void keyboard_func(int key, int scancode, int action, int mods);
	void mouse_func(int button, int action, int mods);
	
	void mousepos_update(int x, int y);
	
	void resize_func(int width, int height);
	
	void window_terminate();
	
	std::vector<int> control_keys;
	
	GLFWwindow* main_window;
	world_controller world_ctl;
	
	ycolor sky_color;
	
	int screen_width;
	int screen_height;
	float mouse_sens;
	
private:
	void update_status_texts();

	double _last_frame_time;
	double _time_delta = 0;
	
	float _second_counter = 1;
	int _fps = 0;
	int _display_fps = 0;
	
	int _last_mouse_x = 0;
	int _last_mouse_y = 0;

	bool _mouse_locked = false;
	
	float _yaw = 0;
	float _pitch = 0;

	int _look_distance = 9;

	ytype::direction _look_direction = ytype::direction::none;
	vec3d<int> _look_chunk;
	vec3d<int> _look_block;
	
	generic_object _look_plane;
	

	int _chunks_amount;
	
	core::shader_program _game_object_shader;

	const font_data* _default_font;
	
	unsigned _shader_player_pos_id = -1;
	

	character _main_character;

	std::unique_ptr<controller> _yan_control;
	std::unique_ptr<physics::raycaster> _main_raycaster;
	physics::controller _main_physics;
	
	camera _main_camera;
	
	gui::controller _main_gui;
	gui::panel* _debug_panel;

	std::array<gui::store_object<text_object>*, text_id::tLAST> _texts_arr;
	
	std::map<std::string, core::texture> _textures_map;
	std::map<std::string, core::shader> _shaders_map;
	
	float _gui_width;
	float _gui_height;
};

void APIENTRY debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	const std::vector<unsigned> ignore_ids({131185});

	for(const auto& num : ignore_ids)
	{
		if(id==num)
			return;
	}

	std::cout << "opengl error " << id << ": " << message << std::endl;
}

static const int font_quality = 75;
game_controller main_world;


game_controller::game_controller()
: _main_camera({0, 0, 0}, {0, 0, 1})
{
	control_keys = std::vector(ykey::yLAST, 0);

	control_keys[ykey::forward] = GLFW_KEY_W;
	control_keys[ykey::back] = GLFW_KEY_S;
	control_keys[ykey::right] = GLFW_KEY_D;
	control_keys[ykey::left] = GLFW_KEY_A;
	control_keys[ykey::jump] = GLFW_KEY_SPACE;
	control_keys[ykey::crouch] = GLFW_KEY_LEFT_CONTROL;
	
	
	sky_color = {0.05f, 0.4f, 0.7f};

	screen_width = 640;
	screen_height = 640;
	mouse_sens = 250.0f;
}

void game_controller::graphics_init()
{
	if(!glfwInit())
		throw std::runtime_error("glfw init failed!");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	#endif

	main_window = glfwCreateWindow(screen_width, screen_height, "shitcraft", NULL, NULL);
	if(!main_window)
		throw std::runtime_error("main window creation failed!");

	glfwMakeContextCurrent(main_window);
	resize_func(screen_width, screen_height);

	_yan_control = std::make_unique<controller>(controller::options{});

	#ifdef DEBUG
	glDebugMessageCallback(debug_callback, 0);
	#endif

	_default_font = _yan_control->load_font("./fonts/FreeSans.ttf", font_quality);

	_shaders_map = controller::load_shaders_from("./shaders");
	_textures_map = controller::load_textures_from("./textures");

	_game_object_shader = core::shader_program(
		_shaders_map["gameobject.fragment"],
		_shaders_map["gameobject.vertex"],
		core::shader());


	_main_character = character();
	_main_character.move_speed = 500;
	_main_character.mass = 5;
	_main_character.floating = true;
	_main_character.position = {0, 30, 0};

	world_ctl = world_controller(main_window, &_main_character,
		graphics_state{&_main_camera, &_game_object_shader,
			&_textures_map.at("block_textures"), &_textures_map.at("transparent_blocks")});

	_main_physics.connect_object(&_main_character);
	_main_raycaster = std::make_unique<physics::raycaster>(world_ctl.world_chunks);
	_main_character.set_raycaster(_main_raycaster.get());

	_shader_player_pos_id = _game_object_shader.add_vec3("player_pos");
	_game_object_shader.set_prop(_game_object_shader.add_vec3("fog_color"), yvec3{sky_color.r, sky_color.g, sky_color.b});
	_game_object_shader.set_prop(_game_object_shader.add_num("render_distance"),
								 world_ctl.render_dist()*chunk_size-chunk_size*2);


	mousepos_update(_last_mouse_x, _last_mouse_y);

	_main_gui = gui::controller(screen_width, screen_height);

	_debug_panel = &_main_gui.add_panel(yvec2{0, 0}, yvec2{1, 1});

	_texts_arr[text_id::xpos] = &_debug_panel->add_text(gui::object_info{
		yvec3{0, 1, 0},
		yvec2{0.2, 0.2},
		yvec2{0, 1}},
		_default_font, "undefined");

	_texts_arr[text_id::ypos] = &_debug_panel->add_text(gui::object_info{
		yvec3{0, 0.9, 0},
		yvec2{0.2, 0.2},
		yvec2{0, 0}},
		_default_font, "undefined");

	_texts_arr[text_id::zpos] = &_debug_panel->add_text(gui::object_info{
		yvec3{0, 0.8, 0},
		yvec2{0.2, 0.2},
		yvec2{0, 0}},
		_default_font, "undefined");

	_texts_arr[text_id::fps] = &_debug_panel->add_text(gui::object_info{
		yvec3{0, 0.6, 0},
		yvec2{0.2, 0.2},
		yvec2{0, 0}},
		_default_font, "undefined");

	update_status_texts();

	/*
	 generic_object c_obj = generic_ob*ject(_gui_shader,
	 yan_resources.create_model(default_model::plane), yan_resources.create_texture(_textures_map["crosshair"]));
	 c_obj.set_position({0, 0, 0});
	 c_obj.set_scale({25, 25, 1});
	 _gui_elements.push_back(std::move(c_obj));*/

	_look_plane = generic_object(&_main_camera,
					&_game_object_shader,
					default_assets.model(default_model::plane),
					default_assets.texture(default_texture::half_transparent));

	_look_plane.set_position({0, 0, 0});
	_look_plane.set_scale({0.5f, 0.5f, 0.5f});

	_last_frame_time = glfwGetTime();
}

void game_controller::window_terminate()
{
	glfwDestroyWindow(main_window);
}

void game_controller::update_status_texts()
{
	_texts_arr[text_id::xpos]->object.set_text("x: "+std::to_string(_main_character.position.x));
	_texts_arr[text_id::ypos]->object.set_text("y: "+std::to_string(_main_character.position.y));
	_texts_arr[text_id::zpos]->object.set_text("z: "+std::to_string(_main_character.position.z));

	_texts_arr[text_id::fps]->object.set_text("fps: "+std::to_string(_display_fps));

	_debug_panel->update();
}

void game_controller::draw_update()
{
	glClearColor(sky_color.r, sky_color.g, sky_color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	
	world_ctl.draw_update();
	
	if(_look_direction!=ytype::direction::none)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	
		_look_plane.draw();
	}
	
	glClear(GL_DEPTH_BUFFER_BIT);

	_main_gui.draw();
	
	glfwSwapBuffers(main_window);
}

void game_controller::update_func()
{
	_time_delta = (glfwGetTime()-_last_frame_time);
	_last_frame_time = glfwGetTime();
	
	_second_counter -= _time_delta;
	++_fps;
	
	if(_second_counter<=0)
	{
		_display_fps = _fps;
		_second_counter = 1;
		_fps = 0;
	}

	vec3d<float> c_accel = {0, 0, 0};
	if(glfwGetKey(main_window, control_keys[ykey::forward]))
	{
		c_accel.z += std::sin(_yaw) * _main_character.move_speed * _time_delta;
		c_accel.x += std::cos(_yaw) * _main_character.move_speed * _time_delta;
	}
	if(glfwGetKey(main_window, control_keys[ykey::back]))
	{
		c_accel.z -= std::sin(_yaw) * _main_character.move_speed * _time_delta;
		c_accel.x -= std::cos(_yaw) * _main_character.move_speed * _time_delta;
	}
	if(glfwGetKey(main_window, control_keys[ykey::right]))
	{
		c_accel.z += std::cos(_yaw) * _main_character.move_speed * _time_delta;
		c_accel.x += -std::sin(_yaw) * _main_character.move_speed * _time_delta;
	}
	if(glfwGetKey(main_window, control_keys[ykey::left]))
	{
		c_accel.z -= std::cos(_yaw) * _main_character.move_speed * _time_delta;
		c_accel.x -= -std::sin(_yaw) * _main_character.move_speed * _time_delta;
	}
	
	if(glfwGetKey(main_window, control_keys[ykey::jump]))
	{
		if(_main_character.on_ground && !_main_character.floating)
		{
			_main_character.mid_jump = true;
			c_accel.y += _main_character.jump_strength;
			
			_main_character.on_ground = false;
		} else if(_main_character.floating)
		{
			c_accel.y += _main_character.move_speed/2 * _time_delta;
		}
	}
	
	if(glfwGetKey(main_window, control_keys[ykey::crouch]))
	{
		if(!_main_character.floating)
		{
			_main_character.sneaking = true;
		} else
		{
			c_accel.y -= _main_character.move_speed/2 * _time_delta;
		}
	} else
	{
		_main_character.sneaking = false;
	}
	
	//the magical force controlling the character
	_main_character.velocity = c_accel;
	
	
	_main_physics.physics_update(_time_delta);
	
	const auto c_raycast = _main_raycaster->raycast(_main_character.position, _main_character.direction, _look_distance*3);
	if(c_raycast.direction!=ytype::direction::none
		&& _main_raycaster->raycast_distance(_main_character.position, c_raycast)<_look_distance)
	{
		_look_direction = c_raycast.direction;
		_look_chunk = c_raycast.chunk;
		_look_block = c_raycast.block;
		
		yvec3 look_pos = {static_cast<float>(_look_chunk.x)*chunk_size+_look_block.x+0.5f, 
				static_cast<float>(_look_chunk.y)*chunk_size+_look_block.y+0.5f, 
				static_cast<float>(_look_chunk.z)*chunk_size+_look_block.z+0.5f};
				
		switch(_look_direction)
		{
			case ytype::direction::right:
				look_pos = {look_pos.x-0.5f, look_pos.y, look_pos.z};
			
				_look_plane.set_rotation_axis(yvec3{0, 1, 0});
				_look_plane.set_rotation(-M_PI/2);
			break;
		
			case ytype::direction::left:
				look_pos = {look_pos.x+0.5f, look_pos.y, look_pos.z};
			
				_look_plane.set_rotation_axis(yvec3{0, 1, 0});
				_look_plane.set_rotation(M_PI/2);
			break;
			
			case ytype::direction::up:
				look_pos = {look_pos.x, look_pos.y-0.5f, look_pos.z};
			
				_look_plane.set_rotation_axis(yvec3{1, 0, 0});
				_look_plane.set_rotation(M_PI/2);
			break;
			
			case ytype::direction::down:
				look_pos = {look_pos.x, look_pos.y+0.5f, look_pos.z};
			
				_look_plane.set_rotation_axis(yvec3{1, 0, 0});
				_look_plane.set_rotation(-M_PI/2);
			break;
			
			case ytype::direction::forward:
				look_pos = {look_pos.x, look_pos.y, look_pos.z-0.5f};
			
				_look_plane.set_rotation_axis(yvec3{0, 1, 0});
				_look_plane.set_rotation(M_PI);
			break;
			
			case ytype::direction::back:
				look_pos = {look_pos.x, look_pos.y, look_pos.z+0.5f};
			
				_look_plane.set_rotation_axis(yvec3{0, 1, 0});
				_look_plane.set_rotation(0);
			break;
			
			default:
			break;
		}
		
		_look_plane.set_position(look_pos);
	} else
	{
		_look_direction = ytype::direction::none;
	}
	
	_game_object_shader.set_prop(_shader_player_pos_id, yvec3{_main_character.position.x, _main_character.position.y, _main_character.position.z});
	
	
	update_status_texts();
	
	_main_camera.set_position({_main_character.position.x, _main_character.position.y, _main_character.position.z});
}


void game_controller::slow_update()
{
	world_ctl.full_update();
}

void game_controller::mousepos_update(int x, int y)
{
	_yaw += (x-_last_mouse_x)/mouse_sens;
	_pitch -= (y-_last_mouse_y)/mouse_sens;
	
	if(_yaw>M_PI*2)
	{
		_yaw -= 2*M_PI;
	} else if(_yaw<0)
	{
		_yaw += 2*M_PI;
	}
	
	//pitch is in radians in a circle
	_pitch = std::clamp(_pitch, -1.56f, 1.56f);
	
	_main_camera.set_rotation(_yaw, _pitch);
	
	_main_character.direction = physics::controller::calc_dir(_yaw, _pitch);
	
	if(_mouse_locked)
	{
		glfwSetCursorPos(main_window, 0, 0);
		_last_mouse_x = 0;
		_last_mouse_y = 0;
	} else
	{
		_last_mouse_x = x;
		_last_mouse_y = y;
	}
}

void game_controller::control_func(bool mouse, int key, int action, int mods, int scancode)
{
	if(!mouse && key==GLFW_KEY_ESCAPE && action==GLFW_PRESS)
	{
		_mouse_locked = !_mouse_locked;
		if(_mouse_locked)
		{
			glfwSetInputMode(main_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		} else
		{
			glfwSetInputMode(main_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			
			double x_pos, y_pos;
			glfwGetCursorPos(main_window, &x_pos, &y_pos);
			_last_mouse_x = static_cast<int>(x_pos);
			_last_mouse_y = static_cast<int>(y_pos);
		}
	}
	
	if(_look_direction!=ytype::direction::none)
	{
		if(mouse && key==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS)
		{
			world_ctl.world_chunks.at(_look_chunk).chunk.set_block(world_block{block::air},
				_look_block);
		}

		if(mouse && key==GLFW_MOUSE_BUTTON_RIGHT && action==GLFW_PRESS)
		{
			const vec3d<int> side = direction_offset(direction_opposite(_look_direction));
			const vec3d<int> place_block = _look_block+side;
			const vec3d<int> place_chunk = _look_chunk+world_chunk::active_chunk(place_block);

			world_ctl.world_chunks.at(place_chunk).chunk.set_block(world_block{block::stone},
				world_chunk::closest_bound_block(place_block));
		}
	}
}


void game_controller::keyboard_func(int key, int scancode, int action, int mods)
{
	control_func(false, key, action, mods, scancode);
}

void game_controller::mouse_func(int button, int action, int mods)
{
	control_func(true, button, action, mods, 0);
}

void game_controller::resize_func(int width, int height)
{
	screen_width = width;
	screen_height = height;
	
	_main_camera.create_projection(45, width/static_cast<float>(height), {0.001f, 500});
	
	glViewport(0, 0, width, height);

	_main_gui.resize(width, height);
}

void update_slow()
{
	main_world.slow_update();
}

void mouse_callback(GLFWwindow* window, double x, double y)
{
	main_world.mousepos_update(static_cast<int>(x), static_cast<int>(y));
}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	main_world.keyboard_func(key, scancode, action, mods);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	main_world.mouse_func(button, action, mods);
}

void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	main_world.resize_func(width, height);
}

int main(int argc, char* argv[])
{	
	main_world.graphics_init();

	glfwSetInputMode(main_world.main_window, GLFW_STICKY_KEYS, GLFW_TRUE);
	
	if(glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(main_world.main_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
	
	glfwSetCursorPosCallback(main_world.main_window, &mouse_callback);
	glfwSetKeyCallback(main_world.main_window, &keyboard_callback);
	glfwSetMouseButtonCallback(main_world.main_window, &mouse_button_callback);
	
	glfwSetFramebufferSizeCallback(main_world.main_window, &framebuffer_resize_callback);

	double last_timer = glfwGetTime();

	const double slowupdate_max = 0.5;
	double slowupdate_counter = slowupdate_max;

	while(!glfwWindowShouldClose(main_world.main_window))
	{
		main_world.update_func();
		main_world.draw_update();
		
		slowupdate_counter += glfwGetTime()-last_timer;
		last_timer = glfwGetTime();
		
		if(slowupdate_counter>slowupdate_max)
		{
			slowupdate_counter -= slowupdate_max;
			update_slow();
		}
		
		glfwPollEvents();
	}
	
	main_world.window_terminate();
	
	glfwTerminate();
	return 0;
}
