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
#include <ythreads.h>
#include <GLFW/glfw3.h>

#include "world.h"
#include "character.h"
#include "types.h"
#include "physics.h"

constexpr int chunkRadius = 3;
constexpr int renderDist = 3;

std::mutex mtxLoadedChunks;
std::mutex mtxQueueChunks;

class WorldController
{
private:
	enum YKey {forward = 0, back, right, left,  jump, crouch, LAST};
	
	struct UpdateChunk
	{
		Vec3d<int> pos;
		bool right = true;
		bool left = true;
		bool up = true;
		bool down = true;
		bool forward = true;
		bool back = true;
		
		bool walls_or() {return right || left || up || down || forward || back;};
	};

public:
	WorldController();
	void graphics_init();
	
	void draw_update();
	void update_func();
	
	void set_visibles();
	void range_remove();
	
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
	std::queue<UpdateChunk> loadedChunkPos;
	
	std::queue<Vec3d<int>> initializeChunks;
	
private:
	void updateStatusTexts();
	
	void update_walls(UpdateChunk currChunk);
	void chunk_loader(Vec3d<int> chunkPos);

	double _lastFrameTime;
	double _timeDelta = 0;
	
	int _lastMouseX = 0;
	int _lastMouseY = 0;

	bool _mouseLocked = false;
	
	float _yaw = 0;
	float _pitch = 0;

	int _chunksAmount;
	
	YanderePool _cGenPool;
	YanderePool _cWallPool;

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
	
	_guiWidth = 1000;
	_guiHeight = 1000;
	_guiCamera.createProjection({
	-_guiWidth,
	_guiWidth,
	-_guiHeight,
	_guiHeight}, {0, 2});
	
	
	_mainPhysCtl = PhysicsController(&loadedChunks);
	
	_mainCharacter = Character(&_mainPhysCtl);
	_mainCharacter.moveSpeed = 1.5f;
	_mainCharacter.floating = true;
	_mainCharacter.position = {0, 30, 0};
	_mainPhysCtl.physObjs.push_back(_mainCharacter);
	
	_worldGen = WorldGenerator(&_mainInit, "block_textures");
	_worldGen.terrainSmallScale = 2.1f;
	_worldGen.terrainMidScale = 0.44f;
	_worldGen.terrainLargeScale = 0.023f;
	_worldGen.temperatureScale = 0.0111f;
	_worldGen.humidityScale = 0.0276f;
	
	unsigned currSeed = time(NULL);
	_worldGen.changeSeed(currSeed);
	std::cout << "world seed: " << currSeed << std::endl;
	
	
	const int wallUpdateThreads = 1;
	const int chunkLoadThreads = std::thread::hardware_concurrency()-wallUpdateThreads-1;
	
	_cGenPool = YanderePool(chunkLoadThreads, &WorldController::chunk_loader, this, Vec3d<int>{0, 0, 0});
	_cWallPool = YanderePool(wallUpdateThreads, &WorldController::update_walls, this, UpdateChunk{Vec3d<int>{0, 0, 0}});
	
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
	
	const float textPadding = 10.0f;
	const float textDistance = 15.0f;
	
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
	glfwDestroyWindow(_mainWindow);
	
	_cGenPool.exit_threads();
	_cWallPool.exit_threads();
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
		if(loadedChunks.count(chunk)==0)
			continue;
	
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
	
	set_visibles();
}

void WorldController::update_walls(UpdateChunk currChunk)
{
	Vec3d<int> currPos = currChunk.pos;
		
	std::unique_lock<std::mutex> lockL(mtxLoadedChunks);	
		
	WorldChunk* currChunkPtr = &loadedChunks[currPos];
		
	if(currChunkPtr->empty())
	{
		std::unique_lock<std::mutex> lockQ(mtxQueueChunks);
		loadedChunkPos.push(currChunk);
		
		return;
	}
	
	
	if(loadedChunks.count(currPos)!=0)
	{
		if(loadedChunks.count({currPos.x+1, currPos.y, currPos.z})!=0 && currChunk.right)
		{
			WorldChunk* checkChunkPtr = &loadedChunks.at({currPos.x+1, currPos.y, currPos.z});
		
			if(!checkChunkPtr->empty())
			{
				currChunk.right = false;
				currChunkPtr->update_wall(Direction::right, checkChunkPtr);
			}
		}
			
		if(loadedChunks.count({currPos.x-1, currPos.y, currPos.z})!=0 && currChunk.left)
		{
			WorldChunk* checkChunkPtr = &loadedChunks.at({currPos.x-1, currPos.y, currPos.z});
		
			if(!checkChunkPtr->empty())
			{
				currChunk.left = false;
				currChunkPtr->update_wall(Direction::left, checkChunkPtr);
			}
		}
			
		if(loadedChunks.count({currPos.x, currPos.y+1, currPos.z})!=0 && currChunk.up)
		{
			WorldChunk* checkChunkPtr = &loadedChunks.at({currPos.x, currPos.y+1, currPos.z});
		
			if(!checkChunkPtr->empty())
			{
				currChunk.up = false;
				currChunkPtr->update_wall(Direction::up, checkChunkPtr);
			}
		}
			
		if(loadedChunks.count({currPos.x, currPos.y-1, currPos.z})!=0 && currChunk.down)
		{
			WorldChunk* checkChunkPtr = &loadedChunks.at({currPos.x, currPos.y-1, currPos.z});
		
			if(!checkChunkPtr->empty())
			{
				currChunk.down = false;
				currChunkPtr->update_wall(Direction::down, checkChunkPtr);
			}
		}
			
		if(loadedChunks.count({currPos.x, currPos.y, currPos.z+1})!=0 && currChunk.forward)
		{
			WorldChunk* checkChunkPtr = &loadedChunks.at({currPos.x, currPos.y, currPos.z+1});
			
			if(!checkChunkPtr->empty())
			{
				currChunk.forward = false;
				currChunkPtr->update_wall(Direction::forward, checkChunkPtr);
			}
		}
			
		if(loadedChunks.count({currPos.x, currPos.y, currPos.z-1})!=0 && currChunk.back)
		{
			WorldChunk* checkChunkPtr = &loadedChunks.at({currPos.x, currPos.y, currPos.z-1});
		
			if(!checkChunkPtr->empty())
			{
				currChunk.back = false;
				currChunkPtr->update_wall(Direction::back, checkChunkPtr);
			}
		}
		
		currChunkPtr->apply_model();
			
		if(currChunk.walls_or())
		{
			std::unique_lock<std::mutex> lockQ(mtxQueueChunks);
			loadedChunkPos.push(currChunk);
		}
	}
}

void WorldController::chunk_loader(Vec3d<int> chunkPos)
{
	WorldChunk genChunk = WorldChunk(&_worldGen, chunkPos);
	genChunk.chunk_gen();
	
	{
		std::unique_lock<std::mutex> lockL(mtxLoadedChunks);	
		loadedChunks[chunkPos] = std::move(genChunk);
	}
		
	initializeChunks.push(chunkPos);
	
	std::unique_lock<std::mutex> lockQ(mtxQueueChunks);
	loadedChunkPos.push({chunkPos});
}

void WorldController::set_visibles()
{
	for(auto& [chunk, mesh] : _drawMeshes)
	{
		Vec3d<int> diffPos = chunk-_mainCharacter.activeChunkPos;
		
		if(loadedChunks[chunk].empty() || Vec3d<int>::magnitude(diffPos) > renderDist)
		{
			_meshVisible[chunk] = false;
		} else
		{
			Vec3d<float> checkPosF = Vec3dCVT<float>(chunk)*chunkSize;
		
			if(_mainCamera.cubeInFrustum({checkPosF.x, checkPosF.y, checkPosF.z}, chunkSize))
			{
				_meshVisible[chunk] = true;
			} else
			{
				_meshVisible[chunk] = false;
			}
		}
	}
}

void WorldController::range_remove()
{
	//unloads chunks which are outside of a certain range
	for(auto& [chunkPos, chunk] : loadedChunks)
	{
		Vec3d<int> offsetChunk = Vec3d<int>{std::abs(chunkPos.x), std::abs(chunkPos.y), std::abs(chunkPos.z)}
		- Vec3d<int>{std::abs(_mainCharacter.activeChunkPos.x), std::abs(_mainCharacter.activeChunkPos.y), std::abs(_mainCharacter.activeChunkPos.z)};
		
		int maxDist = std::max({std::abs(offsetChunk.x), std::abs(offsetChunk.y), std::abs(offsetChunk.z)});
		if(maxDist>chunkRadius)
		{
			//unload the chunk
			if(!chunk.empty())
				chunk.remove_model();
			
			_drawMeshes.erase(chunkPos);
			_meshVisible.erase(chunkPos);
			
			std::unique_lock<std::mutex> lockL(mtxLoadedChunks);	
			loadedChunks.erase(chunkPos);
		}
	}
}

void WorldController::slow_update()
{
	std::map<Vec3d<int>, bool> updateLoaded;

	const int renderDiameter = chunkRadius*2;

	for(int x = 0; x < renderDiameter; ++x)
	{
		for(int y = 0; y < renderDiameter; ++y)
		{
			for(int z = 0; z < renderDiameter; ++z)
			{
				Vec3d<int> checkChunk = _mainCharacter.activeChunkPos + Vec3d<int>{x-chunkRadius, y-chunkRadius, z-chunkRadius};
				
				if(loadedChunks.count(checkChunk)==0 && !updateLoaded[checkChunk])
				{
					updateLoaded[checkChunk] = true;
					loadedChunks[checkChunk] = WorldChunk();

					_cGenPool.run(checkChunk);
				}
			}
		}
	}

	//update chunk meshes to make the block walls continious across chunks
	
	{
		std::unique_lock<std::mutex> lockQ(mtxQueueChunks);
		int queueSize = loadedChunkPos.size();
		for(int i = 0; i<queueSize; ++i)
		{
			//check if not unloaded, then update the walls
			UpdateChunk topChunk = loadedChunkPos.front();
			if(loadedChunks.count(topChunk.pos)!=0)
				_cWallPool.run(std::move(topChunk));

			loadedChunkPos.pop();
		}
	}
	
	range_remove();
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
