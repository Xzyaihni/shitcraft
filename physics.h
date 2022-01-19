#ifndef PHYSICS_H
#define PHYSICS_H

#include <array>
#include <vector>
#include <map>

#include "types.h"

class WorldChunk;
class PhysicsController;

struct RaycastResult
{
	Direction direction;
	Vec3d<int> chunk;
	Vec3d<int> block;
};

class PhysicsObject
{
public:
	PhysicsObject();
	PhysicsObject(PhysicsController* physCtl);
	
	Vec3d<float> position = {0, 0, 0};
	Vec3d<float> velocity = {0, 0, 0};
	
	Vec3d<float> rotationAxis = {1, 0, 0};
	float rotation = 0;
	
	Vec3d<float> directionVec = {0, 0, 0};
	
	Vec3d<int> activeChunkPos;
	
	bool floating = false;
	bool isStatic = false;
	
	float mass = 1;
	
	void update(double timeDelta);
	
private:
	void calculate_active();

	PhysicsController* _physCtl = nullptr;
};

class PhysicsController
{
public:
	PhysicsController();
	PhysicsController(std::map<Vec3d<int>, WorldChunk>* worldChunks);
	
	void physics_update(double timeDelta);
	
	RaycastResult raycast(Vec3d<float> startPos, Vec3d<float> endPos);
	
	std::vector<std::reference_wrapper<PhysicsObject>> physObjs;
	
	float friction = 4;
	Vec3d<float> gravity = {0, -1, 0};
	
	static Vec3d<float> calc_dir(float yaw, float pitch);
	
private:
	std::map<Vec3d<int>, WorldChunk>* _worldChunks;
};

#endif
