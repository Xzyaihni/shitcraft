#include <iostream>
#include <functional>
#include <array>
#include <vector>
#include <chrono>
#include <cmath>
#include <memory>
#include <thread>
#include <map>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <future>

#include <glcyan.h>
#include <GLFW/glfw3.h>

#include "world.h"
#include "character.h"
#include "types.h"
#include "physics.h"
#include "wctl.h"


using namespace WorldTypes;

class GameController
{
private:
	enum YKey {forward = 0, back, right, left,  jump, crouch, LAST};

public:
	GameController();
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
	
	std::vector<int> controlKeys;
	
	GLFWwindow* _mainWindow;
	WorldController worldCtl;
	
	YanColor skyColor;
	
	int screenWidth;
	int screenHeight;
	float mouseSens;
	
private:
	void update_status_texts();

	double _lastFrameTime;
	double _timeDelta = 0;
	
	float _secondCounter = 1;
	int _fps = 0;
	int _displayFps = 0;
	
	int _lastMouseX = 0;
	int _lastMouseY = 0;

	bool _mouseLocked = false;
	
	float _yaw = 0;
	float _pitch = 0;

	int _lookDistance = 9;

	bool _lookLooking = false;
	Vec3d<int> _lookChunk;
	Vec3d<int> _lookBlock;
	
	YandereObject _lookOutline;
	

	int _chunksAmount;
	
	unsigned _defaultShaderID = -1;
	unsigned _gameObjectShaderID = -1;
	unsigned _guiShaderID = -1;
	
	YandereShaderProgram* _shaderGameObjPtr = nullptr;
	unsigned _shaderPlayerPosLoc = -1;
	

	Character _mainCharacter;

	YandereInitializer _mainInit;
	YandereCamera _mainCamera;
	YandereCamera _guiCamera;
	PhysicsController _mainPhysCtl;
	
	std::vector<YandereObject> _guiElements;
	
	std::map<std::string, unsigned> _texturesMap;
	std::map<std::string, unsigned> _shadersMap;
	std::map<std::string, YandereText> _textsMap;
	
	float _guiWidth;
	float _guiHeight;
};

std::unique_ptr<GameController> _mainWorld;

GameController::GameController()
{
	controlKeys = std::vector(YKey::LAST, 0);

	controlKeys[YKey::forward] = GLFW_KEY_W;
	controlKeys[YKey::back] = GLFW_KEY_S;
	controlKeys[YKey::right] = GLFW_KEY_D;
	controlKeys[YKey::left] = GLFW_KEY_A;
	controlKeys[YKey::jump] = GLFW_KEY_SPACE;
	controlKeys[YKey::crouch] = GLFW_KEY_LEFT_CONTROL;
	
	
	skyColor = {0.05f, 0.4f, 0.7f};

	screenWidth = 640;
	screenHeight = 640;
	mouseSens = 250.0f;

	_mainInit = YandereInitializer();
	_shadersMap = _mainInit.load_shaders_from("./shaders");
	_texturesMap = _mainInit.load_textures_from("./textures");
	
	_mainInit.load_font("./fonts/FreeSans.ttf");
	
	_mainCamera = YandereCamera({0, 0, 0}, {0, 0, 1});
	resize_func(screenWidth, screenHeight);
	
	_guiCamera = YandereCamera({0, 0, 1}, {0, 0, 0});
	
	_guiWidth = 1000;
	_guiHeight = 1000;
	_guiCamera.create_projection({
	-_guiWidth,
	_guiWidth,
	-_guiHeight,
	_guiHeight}, {0, 2});
	
	
	worldCtl = WorldController(&_mainInit, &_mainCharacter, &_mainCamera);
	
	_mainPhysCtl = PhysicsController(&worldCtl.worldChunks);
	
	_mainCharacter = Character(&_mainPhysCtl);
	_mainCharacter.moveSpeed = 10;
	_mainCharacter.mass = 50;
	_mainCharacter.floating = true;
	_mainCharacter.position = {500, 30, 1200};
	_mainPhysCtl.physObjs.push_back(_mainCharacter);
	
	
	mousepos_update(_lastMouseX, _lastMouseY);
}

void GameController::graphics_init()
{
	if(!glfwInit())
	{
		std::cout << "glfw init failed!" << std::endl;
		return;
	}
	
	_mainWindow = glfwCreateWindow(screenWidth, screenHeight, "shitcraft", NULL, NULL);
	if(!_mainWindow)
	{
		std::cout << "main window creation failed!" << std::endl;
		return;
	}
	
	glfwMakeContextCurrent(_mainWindow);
	
	glViewport(0, 0, screenWidth, screenHeight);
	_mainInit.do_glew_init();
	
	
	unsigned currSeed = time(NULL);
	worldCtl.create_world(_texturesMap["block_textures"], currSeed);	
	std::cout << "world seed: " << currSeed << std::endl;
	
	
	_defaultShaderID = _mainInit.create_shader_program({_shadersMap["defaultflat.fragment"], _shadersMap["defaultflat.vertex"]});
	_gameObjectShaderID = _mainInit.create_shader_program({_shadersMap["gameobject.fragment"], _shadersMap["gameobject.vertex"]});
	_guiShaderID = _mainInit.create_shader_program({_shadersMap["text.fragment"], _shadersMap["defaultflat.vertex"]});
	
	_shaderGameObjPtr = _mainInit.shader_program_ptr(_gameObjectShaderID);
	
	_shaderPlayerPosLoc = _shaderGameObjPtr->add_vec3("playerPos");
	_shaderGameObjPtr->set_prop(_shaderGameObjPtr->add_vec3("fogColor"), YVec3{skyColor.r, skyColor.g, skyColor.b});
	_shaderGameObjPtr->set_prop(_shaderGameObjPtr->add_num("renderDistance"), worldCtl.render_dist());
	
	
	const float textPadding = 10.0f;
	const float textDistance = 15.0f;
	
	_textsMap["xPos"] = _mainInit.create_text("undefined", "FreeSans", 30, 0, 0);
	float xPosHeight = _textsMap["xPos"].text_height();
	
	float textXPos = -_guiWidth+textPadding;
	
	_textsMap["xPos"].set_position(textXPos, _guiHeight-xPosHeight-textPadding);
	_textsMap["yPos"] = _mainInit.create_text("undefined", "FreeSans", 30, textXPos, _textsMap["xPos"].getPosition()[1]-xPosHeight-textDistance);
	_textsMap["zPos"] = _mainInit.create_text("undefined", "FreeSans", 30, textXPos, _textsMap["yPos"].getPosition()[1]-xPosHeight-textDistance);
	
	_textsMap["fps"] = _mainInit.create_text("undefined", "FreeSans", 30, textXPos, _textsMap["zPos"].getPosition()[1]-xPosHeight*2-textDistance);
	
	update_status_texts();
	
	_guiElements.push_back(YandereObject(&_mainInit, DefaultModel::square, _texturesMap["crosshair"], {{0, 0, 0}, {25, 25, 1}}));
	
	_mainCharacter.activeChunkPos = {0, 0, 0};
	
	_lookOutline = YandereObject(&_mainInit, DefaultModel::cube, DefaultTexture::solid, {{}, {0.5f, 0.5f, 0.5f}}, {1, 1, 1, 0.2f});
	
	
	_lastFrameTime = glfwGetTime();
}

void GameController::window_terminate()
{
	glfwDestroyWindow(_mainWindow);
	
	worldCtl.wait_threads();
}

void GameController::update_status_texts()
{
	_textsMap["xPos"].change_text("x: "+std::to_string(_mainCharacter.position.x));
	_textsMap["yPos"].change_text("y: "+std::to_string(_mainCharacter.position.y));
	_textsMap["zPos"].change_text("z: "+std::to_string(_mainCharacter.position.z));
	
	_textsMap["fps"].change_text("fps: "+std::to_string(_displayFps));
}

void GameController::draw_update()
{
	_mainInit.set_shader_program(_gameObjectShaderID);

	glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	_mainInit.set_draw_camera(&_mainCamera);
	
	worldCtl.draw_update();
	
	if(_lookLooking)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		
		_lookOutline.draw_update();
	}
	
	_mainInit.set_shader_program(_guiShaderID);
	_mainInit.set_draw_camera(&_guiCamera);
	
	for(auto& [name, text] : _textsMap)
	{
		text.draw_update();
	}
	
	for(auto& element : _guiElements)
	{
		element.draw_update();
	}
	
	glfwSwapBuffers(_mainWindow);
}

void GameController::update_func()
{
	_timeDelta = (glfwGetTime()-_lastFrameTime);
	_lastFrameTime = glfwGetTime();
	
	_secondCounter -= _timeDelta;
	++_fps;
	
	if(_secondCounter<=0)
	{
		_displayFps = _fps;
		_secondCounter = 1;
		_fps = 0;
	}

	Vec3d<float> currAccel = {0, 0, 0};
	if(glfwGetKey(_mainWindow, controlKeys[YKey::forward]))
	{
		currAccel.z += std::sin(_yaw);
		currAccel.x += std::cos(_yaw);
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::back]))
	{
		currAccel.z -= std::sin(_yaw);
		currAccel.x -= std::cos(_yaw);
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::right]))
	{
		currAccel.z += std::cos(_yaw);
		currAccel.x += -std::sin(_yaw);
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::left]))
	{
		currAccel.z -= std::cos(_yaw);
		currAccel.x -= -std::sin(_yaw);
	}
	
	if(glfwGetKey(_mainWindow, controlKeys[YKey::jump]))
	{
		if(_mainCharacter.onGround && !_mainCharacter.floating)
		{
			_mainCharacter.midJump = true;
			currAccel.y += _mainCharacter.jumpStrength;
			
			_mainCharacter.onGround = false;
		} else if(_mainCharacter.floating)
		{
			currAccel.y += 1;
		}
	}
	
	if(glfwGetKey(_mainWindow, controlKeys[YKey::crouch]))
	{
		if(!_mainCharacter.floating)
		{
			_mainCharacter.sneaking = true;
		} else
		{
			currAccel.y -= 1;
		}
	} else
	{
		_mainCharacter.sneaking = false;
	}
	
	float vecLength = Vec3d<float>::magnitude(currAccel);
	currAccel = vecLength!=0 ? currAccel/Vec3d<float>::magnitude(currAccel) : Vec3d<float>{0, 0, 0};
	
	//the magical force controlling the character
	_mainCharacter.force = ((currAccel*_mainCharacter.moveSpeed) - _mainCharacter.velocity) * 500;
	
	
	_mainPhysCtl.physics_update(_timeDelta);
	
	RaycastResult currRaycast = _mainPhysCtl.raycast(_mainCharacter.position, _mainCharacter.direction, _lookDistance*3);
	if(currRaycast.direction!=Direction::none && _mainPhysCtl.raycast_distance(_mainCharacter.position, currRaycast)<_lookDistance)
	{
		_lookLooking = true;
		_lookChunk = currRaycast.chunk;
		_lookBlock = currRaycast.block;
			
		YanPosition lookPos = {static_cast<float>(_lookChunk.x)*chunkSize+_lookBlock.x+0.5f, 
		static_cast<float>(_lookChunk.y)*chunkSize+_lookBlock.y+0.5f, 
		static_cast<float>(_lookChunk.z)*chunkSize+_lookBlock.z+0.5f};
			
		_lookOutline.set_position(lookPos);
	} else
	{
		_lookLooking = false;
	}
	
	_shaderGameObjPtr->set_prop(_shaderPlayerPosLoc, YVec3{_mainCharacter.position.x, _mainCharacter.position.y, _mainCharacter.position.z});
	
	
	update_status_texts();
	
	_mainCamera.set_position({_mainCharacter.position.x, _mainCharacter.position.y, _mainCharacter.position.z});
	
	worldCtl.set_visibles();
}


void GameController::slow_update()
{
	worldCtl.full_update();
}

void GameController::mousepos_update(int x, int y)
{
	_yaw += (x-_lastMouseX)/mouseSens;
	_pitch -= (y-_lastMouseY)/mouseSens;
	
	if(_yaw>M_PI*2)
	{
		_yaw -= 2*M_PI;
	} else if(_yaw<0)
	{
		_yaw += 2*M_PI;
	}
	
	//pitch is in radians in a circle
	_pitch = std::clamp(_pitch, -1.56f, 1.56f);
	
	_mainCamera.set_rotation(_yaw, _pitch);
	
	_mainCharacter.direction = PhysicsController::calc_dir(_yaw, _pitch);
	
	if(_mouseLocked)
	{
		glfwSetCursorPos(_mainWindow, 0, 0);
		_lastMouseX = 0;
		_lastMouseY = 0;
	} else
	{
		_lastMouseX = x;
		_lastMouseY = y;
	}
}

void GameController::control_func(bool mouse, int key, int action, int mods, int scancode)
{
	if(key==GLFW_KEY_ESCAPE && action==GLFW_PRESS)
	{
		_mouseLocked = !_mouseLocked;
		if(_mouseLocked)
		{
			glfwSetInputMode(_mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		} else
		{
			glfwSetInputMode(_mainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			
			double xPos, yPos;
			glfwGetCursorPos(_mainWindow, &xPos, &yPos);
			_lastMouseX = static_cast<int>(xPos);
			_lastMouseY = static_cast<int>(yPos);
		}
	}
	
	if(mouse && key==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS)
	{
		//destroy a block
		auto iter = worldCtl.worldChunks.find(_lookChunk);
		if(_lookLooking && iter!=worldCtl.worldChunks.end())
		{
			WorldChunk& currChunk = iter->second;
		
			currChunk.block(_lookBlock).blockType = Block::air;
			worldCtl.chunk_update_full(currChunk, _lookBlock);
		}
	}
}


void GameController::keyboard_func(int key, int scancode, int action, int mods)
{
	control_func(false, key, action, mods, scancode);
}

void GameController::mouse_func(int button, int action, int mods)
{
	control_func(true, button, action, mods, 0);
}

void GameController::resize_func(int width, int height)
{
	screenWidth = width;
	screenHeight = height;
	
	_mainCamera.create_projection(45, width/static_cast<float>(height), {0.001f, 500});
	
	glViewport(0, 0, width, height);
}

void update_slow()
{
	_mainWorld->slow_update();
}

void mouse_callback(GLFWwindow* window, double x, double y)
{
	_mainWorld->mousepos_update(static_cast<int>(x), static_cast<int>(y));
}

void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	_mainWorld->keyboard_func(key, scancode, action, mods);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	_mainWorld->mouse_func(button, action, mods);
}

void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
	_mainWorld->resize_func(width, height);
}

int main(int argc, char* argv[])
{	
	_mainWorld = std::make_unique<GameController>();
	_mainWorld->graphics_init();
	
	glfwSetInputMode(_mainWorld->_mainWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
	
	if(glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(_mainWorld->_mainWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
	
	glfwSetCursorPosCallback(_mainWorld->_mainWindow, &mouse_callback);
	glfwSetKeyCallback(_mainWorld->_mainWindow, &keyboard_callback);
	glfwSetMouseButtonCallback(_mainWorld->_mainWindow, &mouse_button_callback);
	
	glfwSetFramebufferSizeCallback(_mainWorld->_mainWindow, &framebuffer_resize_callback);

	double lastTimer = glfwGetTime();
	double slowupdateCounter = 0.5;

	while(!glfwWindowShouldClose(_mainWorld->_mainWindow))
	{
		_mainWorld->update_func();
		_mainWorld->draw_update();
		
		slowupdateCounter += glfwGetTime()-lastTimer;
		lastTimer = glfwGetTime();
		
		if(slowupdateCounter>0.5)
		{
			slowupdateCounter -= 0.5;
			update_slow();
		}
		
		glfwPollEvents();
	}
	
	_mainWorld->window_terminate();
	
	glfwTerminate();
	return 0;
}
