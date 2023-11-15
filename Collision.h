#pragma once
#ifndef hCollision
#define hCollision

#include <map>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "Mesh.h"
#include "Utilities.h"

float linear(float t)
{//Todo
	//	Find out how to define the shape of surfaces with a function
	return t;
}

//Interview - Data Structures (ML Image Recognition)
struct geometricHash {
	geometricHash(std::vector<vk::Mesh>& gameObjects) {
		glm::vec3 origin = gameObjects[0].matrix[3];
		glm::vec3 boundary = glm::vec3(0.f);
		
		float bounds = 0.f;
		for (int i = 1; i < gameObjects.size(); i++)
		{//TODO: multithread with std::for_each
			// Loop through objects to find the boundaries of the geometric hash
			float distance = glm::distance(glm::vec3(gameObjects[i].matrix[3]), origin);
			if (bounds < distance)
			{//TODO: use atomics for multithreading
				// Determine if hash boundary needs to be increased to accommodate object
				bounds = distance;
				boundary = glm::vec3(gameObjects[i].matrix[3]);
			}
		}
		createHash(origin, boundary, gameObjects);
	}
public:
	void checkNearby(vk::Mesh& gameObject)//opt param: bool(*condition)(vk::Mesh& gameObject, vk::Mesh* localObject)
	{// Checks bin for objects meeting certain criteria
		glm::vec3 key = index(gameObject.matrix[3]);

		std::vector<vk::Mesh*> collisions;
		for (auto localObject : locationMap[key]) {
			if (localObject != &gameObject) {
				if (collision(gameObject, localObject)) {
					collisions.push_back(localObject);
					continue;
				}
			}
		}

		if (!collisions.empty()) {
			fixCollisions(gameObject, collisions);
			glm::vec3 newKey = index(gameObject.matrix[3]);
			if (key != newKey)
			{//TODO
				// Possibly move into the fixCollisions function
				updateHash(gameObject, key, newKey);
			}
		}
	}
protected:
	int hashResolution = 0;
	glm::mat4 cmat = glm::mat4(1.f);
	std::unordered_map <glm::vec3, std::vector<vk::Mesh*>> locationMap;

	void createHash(glm::vec3& origin, glm::vec3& boundary, std::vector<vk::Mesh>& gameObjects) {
		//1. Calculate and store optimal hash resolution glm::vector
		//2. Find object indices and organize into std::vectors
		//		this is a vector of objects with so-and-so index, etc.
		//3. Generate hash-table using indices and std::vectors of objects

		cmat = genMatrix(origin, boundary);
		hashResolution = calcHashResolution(gameObjects.size(), 3);

		for (auto& gameObject : gameObjects) {
			glm::vec3 key = index(gameObject.matrix[3]);
			if (locationMap.contains(key)) {
				locationMap[key].push_back(&gameObject);
				continue;
			}
			locationMap.insert({ key, std::vector<vk::Mesh*>{&gameObject} });
		}
	}
	
	void updateHash(vk::Mesh& gameObject, glm::vec3& oldKey, glm::vec3& newKey)
	{// Update object's geometric hash position
		if (locationMap.contains(oldKey) && locationMap.contains(newKey)) {
			auto it = std::find(locationMap[oldKey].begin(), locationMap[oldKey].end(), &gameObject);
			//auto index = std::distance(locationMap[oldKey].begin(), it);
			locationMap[oldKey].erase(it);
			locationMap[newKey].push_back(&gameObject);
			return;
		}
		locationMap.insert({ newKey, std::vector<vk::Mesh*>{&gameObject} });
	};

	void rehash();//TODO
	// Remove/Consolidate unused bins
	// If neighboring bins do not have any objects, remove immediately

	glm::vec3 index(glm::vec4& objectPos) // (2)
	{//Generates key using optimal hash resolution
		//	and object's hash-space position
		glm::vec4 hashPos = cmat * objectPos;
		//TODO
		//	determine hash resolutions for each dimension
		int hashX = int((hashPos.x) * hashResolution);
		int hashY = int((hashPos.y) * hashResolution);
		int hashZ = int((hashPos.z) * hashResolution);
		return glm::vec3(hashX, hashY, hashZ);
	}

	static float calcScale(glm::vec3& coord0, glm::vec3& coord1) {
		return 1 / glm::distance(coord0, coord1);
	}
	static glm::vec2 calcRotation(glm::vec3& coord0, glm::vec3& coord1) {
		float rotX = glm::orientedAngle(glm::normalize(coord0), glm::normalize(coord1), glm::vec3(1, 0, 0));
		float rotY = glm::orientedAngle(glm::normalize(coord0), glm::normalize(coord1), glm::vec3(0, 1, 0));
		// join threads, if chose to multithread
		return glm::vec2(rotX, rotY);
	}
	static glm::mat4 genMatrix(glm::vec3& coord0, glm::vec3& coord1)
	{//TODO: multithread
	 // Create hash-space matrix to index object positions within the geometric hash-table
		// time the order of rotation, scale, and translate functions
		// to find the order which reduces thread idling
		float scale = calcScale(coord0, coord1);
		glm::vec2 rotation = glm::radians(calcRotation(coord0, coord1));

		glm::mat4 coordMat = glm::translate(glm::mat4(1.f), coord0); //relocate origin
		coordMat = glm::rotate(coordMat, glm::radians(rotation.x), glm::vec3(1, 0, 0)); //x-axis rotation
		coordMat = glm::rotate(coordMat, glm::radians(rotation.y), glm::vec3(0, 1, 0)); //y-axis rotation
		return glm::scale(coordMat, glm::vec3(scale));
	};

	static int calcHashResolution(int numObjects, int numPerBin) // (1)
	{// Generates the optimal geometric hash to minimize empty bins,
		//	and maximize bin usage
		return (numObjects - (numObjects % numPerBin)) / numPerBin;
	}
private:
	bool collision(vk::Mesh& gameObject0, vk::Mesh* gameObject1) {
		float theta0 = AoI(&gameObject0, gameObject1); //angle of incidence
		float rad0i = 1.f, rad0f = 2.f;   //min rad to max rad of object0
		float rad0 = lerp(rad0i, rad0f, linear(theta0)); //rad of object0 at collision point

		float theta1 = AoI(gameObject1, &gameObject0); //angle of incidence
		float rad1i = 0.5f, rad1f = 2.5f; //min rad to max rad of object1
		float rad1 = lerp(rad1i, rad1f, linear(theta1)); //rad of object1 collision point

		if (glm::distance(gameObject0.matrix[3], gameObject1->matrix[3]) <= rad0 + rad1) {
			return true;
		}
		return false;
	}

	void fixCollisions(vk::Mesh& gameObject, std::vector<vk::Mesh*>& collisions)
	{//TODO

	}

	float AoI(vk::Mesh* gameObject0, vk::Mesh* gameObject1)
	{//TODO
		
	}
};



#endif