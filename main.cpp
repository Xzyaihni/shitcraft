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

#include <glcyan.h>
#include <GLFW/glfw3.h>

#include "world.h"
#include "character.h"
#include "types.h"
#include "physics.h"

constexpr int renderRadius = 10;
constexpr int renderDist = 4;

std::mutex globalMutex;
std::mutex globalMutexI;

class WorldController
{
public:
	enum YKey {forward = 0, back, right, left,  jump, crouch, LAST};

	WorldController();
	void graphics_init();
	
	void draw_update();
	void update_func();
	
	void slow_update();
	
	void mouse_update(int button, int state, int x, int y);
	void keyboard_func(int key, int scancode, int action, int mods);
	void mouse_func(int button, int action, int mods);
	
	void mousepos_update(int x, int y);
	
	void resize_func(int width, int height);
	
	void window_terminate();
	
	std::vector<int> controlKeys;
	
	GLFWwindow* _mainWindow;
	
	YanColor skyColor;
	
	int screenWidth;
	int screenHeight;
	float mouseSens;
	
	std::map<Vec3d<int>, WorldChunk> loadedChunks;
	std::vector<Vec3d<int>> loadedChunkPos;
	
	std::queue<Vec3d<int>> initializeChunks;
	
private:
	void updateStatusTexts();
	
	void chunkUpdateThread();

	double _lastFrameTime;
	double _timeDelta = 0;
	
	int _lastMouseX = 0;
	int _lastMouseY = 0;

	bool _mouseLocked = false;
	
	float _yaw = 0;
	float _pitch = 0;

	std::condition_variable _condVar;
	bool _endThreads = false;
	bool _fillChunks = true;
	int _chunksAmount;
	int _currIndex;
	std::queue<Vec3d<int>> _threadsQueue;
	std::vector<std::thread> _threadsVec;

	std::map<Vec3d<int>, YandereObject> _drawMeshes;
	std::map<Vec3d<int>, bool> _meshVisible;
	
	WorldGenerator _worldGen;

	Character _mainCharacter;

	YandereInitializer _mainInit;
	YandereCamera _mainCamera;
	YandereCamera _guiCamera;
	PhysicsController _mainPhysCtl;
	
	std::map<std::string, YandereText> _textsMap;
	
	float _guiWidth;
	float _guiHeight;
};

std::unique_ptr<WorldController> _mainWorld;

WorldController::WorldController()
{
	controlKeys = std::vector(YKey::LAST, 0);

	controlKeys[YKey::forward] = GLFW_KEY_W;
	controlKeys[YKey::back] = GLFW_KEY_S;
	controlKeys[YKey::right] = GLFW_KEY_D;
	controlKeys[YKey::left] = GLFW_KEY_A;
	controlKeys[YKey::jump] = GLFW_KEY_SPACE;
	controlKeys[YKey::crouch] = GLFW_KEY_LEFT_CONTROL;
	
	
	skyColor = {0.1f, 0.5f, 0.7f};

	screenWidth = 640;
	screenHeight = 640;
	mouseSens = 250.0f;

	_mainInit = YandereInitializer();
	_mainInit.loadShadersFrom("./shaders");
	_mainInit.loadTexturesFrom("./textures");
	
	_mainInit.loadFont("./fonts/FreeSans.ttf");
	
	_mainCamera = YandereCamera({0, 0, 0}, {0, 0, 1});
	resize_func(screenWidth, screenHeight);
	
	_guiCamera = YandereCamera({0, 0, 1}, {0, 0, 0});
	
	_guiWidth = 50.0f;
	_guiHeight = 50.0f;
	_guiCamera.createProjection({
	-_guiWidth,
	_guiWidth,
	-_guiHeight,
	_guiHeight}, {0, 2});
	
	
	_mainPhysCtl = PhysicsController(&loadedChunks);
	
	_mainCharacter = Character(&_mainPhysCtl);
	_mainCharacter.moveSpeed = 1.5f;
	_mainCharacter.floating = true;
	_mainCharacter.position = {0, 0, 0};
	_mainPhysCtl.physObjs.push_back(_mainCharacter);
	
	_worldGen = WorldGenerator(&_mainInit, "block_textures");
	_worldGen.noiseParts = 1;
	_worldGen.genHeight = 32;
	_worldGen.seed = rand();
	std::cout << "world seed: " << _worldGen.seed << std::endl;
	
	mousepos_update(_lastMouseX, _lastMouseY);
}

void WorldController::graphics_init()
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
	_mainInit.doGlewInit();
	
	_mainInit.createShaderProgram(0, {"defaultflat.fragment", "defaultflat.vertex"});
	_mainInit.createShaderProgram(1, {"text.fragment", "defaultflat.vertex"});
	
	const float textPadding = 0.5f;
	const float textDistance = 0.75f;
	
	_textsMap["xPos"] = _mainInit.createText("undefined", "FreeSans", 30, 0, 0);
	float xPosHeight = _textsMap["xPos"].textHeight();
	
	float textXPos = -_guiWidth+textPadding;
	
	_textsMap["xPos"].setPosition(textXPos, _guiHeight-xPosHeight*2-textPadding);
	_textsMap["yPos"] = _mainInit.createText("undefined", "FreeSans", 30, textXPos, _textsMap["xPos"].getPosition()[1]-xPosHeight*2-textDistance);
	_textsMap["zPos"] = _mainInit.createText("undefined", "FreeSans", 30, textXPos, _textsMap["yPos"].getPosition()[1]-xPosHeight*2-textDistance);
	
	updateStatusTexts();
	
	_mainCharacter.activeChunkPos = {0, 0, 0};
	
	_lastFrameTime = glfwGetTime();
}

void WorldController::window_terminate()
{
	_endThreads = true;
	_condVar.notify_all();
	for(auto& thread : _threadsVec)
	{
		thread.join();
	}

	glfwDestroyWindow(_mainWindow);
}

void WorldController::updateStatusTexts()
{
	_textsMap["xPos"].changeText("x: "+std::to_string(_mainCharacter.position.x));
	_textsMap["yPos"].changeText("y: "+std::to_string(_mainCharacter.position.y));
	_textsMap["zPos"].changeText("z: "+std::to_string(_mainCharacter.position.z));
}

void WorldController::draw_update()
{
	_mainInit.switchShaderProgram(0);

	glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	while(!initializeChunks.empty())
	{
		Vec3d<int> chunk = initializeChunks.front();
	
		std::string chunkName = WorldChunk::getModelName(loadedChunks[chunk].position());
	
		//casting before multiplying takes more time but allows me to have positions higher than 2 billion (not enough)
		YanPosition chunkPos{static_cast<float>(loadedChunks[chunk].position().x)*chunkSize, 
		static_cast<float>(loadedChunks[chunk].position().y)*chunkSize, 
		static_cast<float>(loadedChunks[chunk].position().z)*chunkSize};
	
		YanTransforms chunkT{chunkPos, chunkSize, chunkSize, chunkSize};
		
		_drawMeshes[chunk] = YandereObject(&_mainInit, chunkName, _worldGen.atlasName, chunkT);
		
		initializeChunks.pop();
	}
	
	_mainInit.setDrawCamera(&_mainCamera);
	for(auto& [key, mesh] : _drawMeshes)
	{
		if(_meshVisible[key])
			mesh.drawUpdate();
	}
	
	
	_mainInit.switchShaderProgram(1);
	_mainInit.setDrawCamera(&_guiCamera);
	for(auto& [name, text] : _textsMap)
	{
		text.drawUpdate();
	}
	
	glfwSwapBuffers(_mainWindow);
}

void WorldController::update_func()
{
	_timeDelta = (glfwGetTime()-_lastFrameTime)*10;
	_lastFrameTime = glfwGetTime();

	if(glfwGetKey(_mainWindow, controlKeys[YKey::forward]))
	{
		_mainCharacter.velocity.z += std::sin(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
		_mainCharacter.velocity.x += std::cos(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::back]))
	{
		_mainCharacter.velocity.z -= std::sin(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
		_mainCharacter.velocity.x -= std::cos(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::right]))
	{
		_mainCharacter.velocity.z += std::cos(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
		_mainCharacter.velocity.x += -std::sin(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::left]))
	{
		_mainCharacter.velocity.z -= std::cos(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
		_mainCharacter.velocity.x -= -std::sin(_yaw) * _mainCharacter.moveSpeed*_timeDelta;
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::jump]))
	{
		if(_mainCharacter.onGround && !_mainCharacter.floating)
		{
			_mainCharacter.midJump = true;
			_mainCharacter.velocity.y += _mainCharacter.jumpStrength;
			
			_mainCharacter.onGround = false;
		} else if(_mainCharacter.floating)
		{
			_mainCharacter.velocity.y += _mainCharacter.moveSpeed*_timeDelta;
		}
	}
	if(glfwGetKey(_mainWindow, controlKeys[YKey::crouch]))
	{
		if(!_mainCharacter.floating)
		{
			_mainCharacter.sneaking = true;
		} else
		{
			_mainCharacter.velocity.y -= _mainCharacter.moveSpeed*_timeDelta;
		}
	} else
	{
		_mainCharacter.sneaking = false;
	}
	
	_mainPhysCtl.physicsUpdate(_timeDelta);
	
	updateStatusTexts();
	
	_mainCamera.setPosition({_mainCharacter.position.x, _mainCharacter.position.y, _mainCharacter.position.z});
	
	for(auto& [chunk, mesh] : _drawMeshes)
	{
		Vec3d<int> diffPos = chunk-_mainCharacter.activeChunkPos;
		
		int maxDiff = std::max({std::abs(diffPos.x), std::abs(diffPos.y), std::abs(diffPos.z)});
		
		if(loadedChunks[chunk].empty())
		{
			_meshVisible[chunk] = false;
			continue;
		}
		
		if(maxDiff>renderDist)
		{
			_meshVisible[chunk] = false;
		} else
		{
			Vec3d<float> checkPosF = Vec3dCVT<float>(chunk)*chunkSize;
		
			if(true)
			{
				_meshVisible[chunk] = true;
			} else
			{
				_meshVisible[chunk] = false;
			}
		}
	}
}

void WorldController::chunkUpdateThread()
{
	while(true)
	{
		std::unique_lock<std::mutex> lock(globalMutex);
		
		_condVar.wait(lock, [this]{return  !_threadsQueue.empty() || _endThreads;});
		
		if(_endThreads)
		{
			return;
		}
			
		Vec3d<int>chunkPos = _threadsQueue.front();
		_threadsQueue.pop();
		
		lock.unlock();
		
		
		WorldChunk chunk = WorldChunk(&_worldGen, chunkPos);
		chunk.chunk_gen();
		
		loadedChunks[chunkPos] = std::move(chunk);
		
		{
			std::unique_lock<std::mutex> lock(globalMutexI);
		
			initializeChunks.push(chunkPos);
			loadedChunkPos.push_back(chunkPos);
			
			++_currIndex;
		}
	}
}

void WorldController::slow_update()
{
	constexpr int rVal = 16;

	if(_fillChunks)
	{
		_fillChunks = false;
		
		for(int i = 0; i < std::thread::hardware_concurrency(); ++i)
		{
			_threadsVec.push_back(std::thread(&WorldController::chunkUpdateThread, this));
		}
		
		_chunksAmount = 0;
		_currIndex = 0;
	}

	if(_currIndex != _chunksAmount)
	{
		_condVar.notify_all();
		return;
	}		
	_chunksAmount = 0;
	_currIndex = 0;

	std::map<Vec3d<int>, bool> updateLoaded;

	for(int r = 0; r <= renderRadius; ++r)
	{
		for(int a0 = 0; a0 < rVal; ++a0)
		{
			float angle_0 = a0*(M_PI/static_cast<float>(rVal));
			
			for(int a1 = 0; a1 < rVal; ++a1)
			{
				float angle_1 = a1*(M_PI/static_cast<float>(rVal));
			
				int xLoad = static_cast<int>(r*std::cos(angle_0)*std::sin(angle_1));
				int yLoad = static_cast<int>(r*std::sin(angle_0)*std::sin(angle_1));
				int zLoad = static_cast<int>(r*std::cos(angle_1));
				
				Vec3d<int> checkChunk = _mainCharacter.activeChunkPos + Vec3d<int>{xLoad, yLoad, zLoad};
			
				if(loadedChunks.count(checkChunk)==0 && !updateLoaded[checkChunk])
				{
					++_chunksAmount;
					updateLoaded[checkChunk] = true;
					
					_threadsQueue.push(checkChunk);
					_condVar.notify_one();
				}
			}
		}
	}
}

void WorldController::mousepos_update(int x, int y)
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
	
	_mainCamera.setRotation(_yaw, _pitch);
	
	_mainCharacter.directionVec = PhysicsController::calcDir(_yaw, _pitch);
	
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

void WorldController::keyboard_func(int key, int scancode, int action, int mods)
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
}

void WorldController::mouse_func(int button, int action, int mods)
{
	if(button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS)
	{
		
	}
}

void WorldController::resize_func(int width, int height)
{
	screenWidth = width;
	screenHeight = height;
	
	_mainCamera.createProjection(45, width/static_cast<float>(height), {0.001f, 500});
	
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
	_mainWorld = std::make_unique<WorldController>();
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
