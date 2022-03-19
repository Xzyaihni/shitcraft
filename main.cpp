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


using namespace yanderegl;
using namespace world_types;

class game_controller
{
private:
	enum ykey {forward = 0, back, right, left, jump, crouch, LAST};

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
	
	yan_color sky_color;
	
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
	
	yandere_object _look_plane;
	

	int _chunks_amount;
	
	yandere_shader_program _default_shader;
	yandere_shader_program _game_object_shader;
	yandere_shader_program _gui_shader;
	
	unsigned _shader_player_pos_id = -1;
	

	character _main_character;

	std::unique_ptr<yandere_controller> _yan_control;
	std::unique_ptr<physics_controller> _main_phys_ctl;
	
	yandere_camera _main_camera;
	yandere_camera _gui_camera;
	
	yandere_gui _main_gui;
	
	std::vector<yandere_object> _gui_elements;
	
	std::map<std::string, unsigned> _textures_map;
	std::map<std::string, unsigned> _shaders_map;
	std::map<std::string, yandere_text> _texts_map;
	
	float _gui_width;
	float _gui_height;
};


game_controller main_world;


game_controller::game_controller()
{
	control_keys = std::vector(ykey::LAST, 0);

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
	
	_main_camera = yandere_camera({0, 0, 0}, {0, 0, 1});
	resize_func(screen_width, screen_height);
	
	_gui_camera = yandere_camera({0, 0, 1}, {0, 0, 0});
	
	_gui_width = 1000;
	_gui_height = 1000;
	_gui_camera.create_projection({
	-_gui_width,
	_gui_width,
	-_gui_height,
	_gui_height}, {0, 2});
}

void game_controller::graphics_init()
{
	if(!glfwInit())
	{
		throw std::runtime_error("glfw init failed!");
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	main_window = glfwCreateWindow(screen_width, screen_height, "shitcraft", NULL, NULL);
	if(!main_window)
	{
		throw std::runtime_error("main window creation failed!");
	}
	
	glfwMakeContextCurrent(main_window);
	glViewport(0, 0, screen_width, screen_height);

	_yan_control = std::make_unique<yandere_controller>(true, true, true);

	yandere_resources& yan_resources = _yan_control->resources();
	_shaders_map = yan_resources.load_shaders_from("./shaders");
	_textures_map = yan_resources.load_textures_from("./textures");
	
	_yan_control->load_font("./fonts/FreeSans.ttf");
	
	
	_default_shader = _yan_control->create_shader_program({_shaders_map["defaultflat.fragment"], _shaders_map["defaultflat.vertex"]});
	_game_object_shader = _yan_control->create_shader_program({_shaders_map["gameobject.fragment"], _shaders_map["gameobject.vertex"]});
	_gui_shader = _yan_control->create_shader_program({_shaders_map["text.fragment"], _shaders_map["defaultflat.vertex"]});
	
	
	world_ctl = world_controller(_yan_control.get(), main_window, &_main_character, &_main_camera, _game_object_shader);
	
	_shader_player_pos_id = _game_object_shader.add_vec3("player_pos");
	_game_object_shader.set_prop(_game_object_shader.add_vec3("fog_color"), yvec3{sky_color.r, sky_color.g, sky_color.b});
	_game_object_shader.set_prop(_game_object_shader.add_num("render_distance"), world_ctl.render_dist());
	
	unsigned c_seed = time(NULL);
	world_ctl.create_world(_textures_map["block_textures"], c_seed);	
	std::cout << "world seed: " << c_seed << std::endl;
	
	_main_phys_ctl = std::make_unique<physics_controller>(world_ctl.world_chunks);
	
	_main_character = character(_main_phys_ctl.get());
	_main_character.move_speed = 10;
	_main_character.mass = 50;
	_main_character.floating = true;
	_main_character.position = {500, 30, 1200};
	_main_phys_ctl->phys_objs.push_back(_main_character);
	
	
	mousepos_update(_last_mouse_x, _last_mouse_y);
	
	
	_main_gui = yandere_gui();
	const float text_padding = 10.0f;
	const float text_distance = 15.0f;
	
	_texts_map.emplace(std::piecewise_construct, 
	std::forward_as_tuple("x_pos"), 
	std::forward_as_tuple(_gui_shader, _yan_control->font("FreeSans"), "undefined", 30, 0, 0));
	
	float x_pos_height = _texts_map.at("x_pos").text_height();
	
	float text_pos_x = -_gui_width+text_padding;
	
	_texts_map.at("x_pos").set_position(text_pos_x, _gui_height-x_pos_height-text_padding);
	_texts_map.emplace(std::piecewise_construct,
	std::forward_as_tuple("y_pos"),
	std::forward_as_tuple(_gui_shader, _yan_control->font("FreeSans"), "undefined",
	30, text_pos_x, _texts_map.at("x_pos").position()[1]-x_pos_height-text_distance));
	
	_texts_map.emplace(std::piecewise_construct,
	std::forward_as_tuple("z_pos"),
	std::forward_as_tuple(_gui_shader, _yan_control->font("FreeSans"), "undefined",
	30, text_pos_x, _texts_map.at("y_pos").position()[1]-x_pos_height-text_distance));
	
	_texts_map.emplace(std::piecewise_construct,
	std::forward_as_tuple("fps"),
	std::forward_as_tuple(_gui_shader, _yan_control->font("FreeSans"), "undefined",
	30, text_pos_x, _texts_map.at("z_pos").position()[1]-x_pos_height*2-text_distance));
	
	update_status_texts();
	
	_gui_elements.push_back(yandere_object(_gui_shader,
	yan_resources.create_model(default_model::square), yan_resources.create_texture(_textures_map["crosshair"]),
	{{0, 0, 0}, {25, 25, 1}}));
	
	_main_character.active_chunk_pos = {0, 0, 0};
	
	_look_plane = yandere_object(_game_object_shader,
	yan_resources.create_model(default_model::square), yan_resources.create_texture(default_texture::solid),
	{{}, {0.5f, 0.5f, 0.5f}}, {1, 1, 1, 0.2f});
	
	
	_last_frame_time = glfwGetTime();
}

void game_controller::window_terminate()
{
	glfwDestroyWindow(main_window);
	
	world_ctl.wait_threads();
}

void game_controller::update_status_texts()
{
	_texts_map.at("x_pos").set_text("x: "+std::to_string(_main_character.position.x));
	_texts_map.at("y_pos").set_text("y: "+std::to_string(_main_character.position.y));
	_texts_map.at("z_pos").set_text("z: "+std::to_string(_main_character.position.z));
	
	_texts_map.at("fps").set_text("fps: "+std::to_string(_display_fps));
}

void game_controller::draw_update()
{
	glClearColor(sky_color.r, sky_color.g, sky_color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	
	_yan_control->set_draw_camera(&_main_camera);
	
	world_ctl.draw_update();
	
	if(_look_direction!=ytype::direction::none)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	
		_look_plane.draw_update();
	}
	
	glClear(GL_DEPTH_BUFFER_BIT);
	_yan_control->set_draw_camera(&_gui_camera);
	
	for(auto& [name, text] : _texts_map)
	{
		text.draw_update();
	}
	
	for(auto& element : _gui_elements)
	{
		element.draw_update();
	}
	
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
		c_accel.z += std::sin(_yaw) * _main_character.move_speed;
		c_accel.x += std::cos(_yaw) * _main_character.move_speed;
	}
	if(glfwGetKey(main_window, control_keys[ykey::back]))
	{
		c_accel.z -= std::sin(_yaw) * _main_character.move_speed;
		c_accel.x -= std::cos(_yaw) * _main_character.move_speed;
	}
	if(glfwGetKey(main_window, control_keys[ykey::right]))
	{
		c_accel.z += std::cos(_yaw) * _main_character.move_speed;
		c_accel.x += -std::sin(_yaw) * _main_character.move_speed;
	}
	if(glfwGetKey(main_window, control_keys[ykey::left]))
	{
		c_accel.z -= std::cos(_yaw) * _main_character.move_speed;
		c_accel.x -= -std::sin(_yaw) * _main_character.move_speed;
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
			c_accel.y += _main_character.move_speed/2;
		}
	}
	
	if(glfwGetKey(main_window, control_keys[ykey::crouch]))
	{
		if(!_main_character.floating)
		{
			_main_character.sneaking = true;
		} else
		{
			c_accel.y -= _main_character.move_speed/2;
		}
	} else
	{
		_main_character.sneaking = false;
	}
	
	//the magical force controlling the character
	_main_character.velocity = c_accel;
	
	
	_main_phys_ctl->physics_update(_time_delta);
	
	raycast_result c_raycast = _main_phys_ctl->raycast(_main_character.position, _main_character.direction, _look_distance*3);
	if(c_raycast.direction!=ytype::direction::none && _main_phys_ctl->raycast_distance(_main_character.position, c_raycast)<_look_distance)
	{
		_look_direction = c_raycast.direction;
		_look_chunk = c_raycast.chunk;
		_look_block = c_raycast.block;
		
		yan_position look_pos = {static_cast<float>(_look_chunk.x)*chunk_size+_look_block.x+0.5f, 
				static_cast<float>(_look_chunk.y)*chunk_size+_look_block.y+0.5f, 
				static_cast<float>(_look_chunk.z)*chunk_size+_look_block.z+0.5f};
				
		switch(_look_direction)
		{
			case ytype::direction::right:
				look_pos = {look_pos.x-0.5f, look_pos.y, look_pos.z};
			
				_look_plane.set_rotation_axis(yan_position{0, 1, 0});
				_look_plane.set_rotation(-M_PI/2);
			break;
		
			case ytype::direction::left:
				look_pos = {look_pos.x+0.5f, look_pos.y, look_pos.z};
			
				_look_plane.set_rotation_axis(yan_position{0, 1, 0});
				_look_plane.set_rotation(M_PI/2);
			break;
			
			case ytype::direction::up:
				look_pos = {look_pos.x, look_pos.y-0.5f, look_pos.z};
			
				_look_plane.set_rotation_axis(yan_position{1, 0, 0});
				_look_plane.set_rotation(M_PI/2);
			break;
			
			case ytype::direction::down:
				look_pos = {look_pos.x, look_pos.y+0.5f, look_pos.z};
			
				_look_plane.set_rotation_axis(yan_position{1, 0, 0});
				_look_plane.set_rotation(-M_PI/2);
			break;
			
			case ytype::direction::forward:
				look_pos = {look_pos.x, look_pos.y, look_pos.z-0.5f};
			
				_look_plane.set_rotation_axis(yan_position{0, 1, 0});
				_look_plane.set_rotation(M_PI);
			break;
			
			case ytype::direction::back:
				look_pos = {look_pos.x, look_pos.y, look_pos.z+0.5f};
			
				_look_plane.set_rotation_axis(yan_position{0, 1, 0});
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
	
	_main_character.direction = physics_controller::calc_dir(_yaw, _pitch);
	
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
	
	if(mouse && key==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS)
	{	
		if(_look_direction!=ytype::direction::none)
			world_ctl.destroy_block(_look_chunk, _look_block);
	}
	
	if(mouse && key==GLFW_MOUSE_BUTTON_RIGHT && action==GLFW_PRESS)
	{
		world_ctl.place_block(_look_chunk, _look_block, world_block{block::stone}, _look_direction);
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
	double slowupdate_counter = 0.5;

	while(!glfwWindowShouldClose(main_world.main_window))
	{
		main_world.update_func();
		main_world.draw_update();
		
		slowupdate_counter += glfwGetTime()-last_timer;
		last_timer = glfwGetTime();
		
		if(slowupdate_counter>0.5)
		{
			slowupdate_counter -= 0.5;
			update_slow();
		}
		
		glfwPollEvents();
	}
	
	main_world.window_terminate();
	
	glfwTerminate();
	return 0;
}
