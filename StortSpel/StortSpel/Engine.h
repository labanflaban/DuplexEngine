#pragma once
#include "Player.h"
#include "LightComponent.h"
#include "SpotLightComponent.h"
#include "MeshComponent.h"
#include "TestComponent.h"
#include "RotateAroundComponent.h"
#include "SweepingComponent.h"
#include "FlippingComponent.h"
#include "PhysicsComponent.h"
#include "AudioHandler.h"
#include "AudioComponent.h"
#include "CheckpointComponent.h"
#include "Camera.h"

struct Settings
{
	int width = 1920;
	int height = 1080;
};


class Engine : public PhysicsObserver
{

private:
	Engine();
	Settings m_settings;
	static const int m_startWidth = 1920;
	static const int m_startHeight = 1080;

	Vector4 m_skyLightDir = Vector4(0, 0.5, -0.5, 0);
	Vector4 m_skyLightColor = Vector4(1, 1, 1, 1);
	FLOAT m_skyLightBrightness = 1.5f;
	FLOAT m_ambientLightLevel = 0.3f;
	
	ID3D11Device* m_devicePtr = NULL;
	ID3D11DeviceContext* m_dContextPtr = NULL;

	float nightVolume;
	float nightSlide;

	// Entities
	std::unordered_map<std::string, Entity*> m_entities;
	
	Player* m_player = nullptr;
	Camera m_camera;
	std::map<unsigned int long, MeshComponent*> m_meshComponentMap;
	std::map<std::string, LightComponent*> m_lightComponentMap;

	unsigned int long m_MeshCount = 0;
	unsigned int long m_lightCount = 0;

	bool DeviceAndContextPtrsAreSet; //This bool just ensures that no one calls Engine::initialize before Renderer::initialize has been called

	void updateLightData();
public:
	static Engine& get();

	void sendPhysicsMessage(PhysicsData& physicsData, bool& removed);

	void initialize();
	void setDeviceAndContextPtrs(ID3D11Device* devicePtr, ID3D11DeviceContext* dContextPtr);

	Entity* getEntity(std::string key);
	Entity* addEntity(std::string identifier);
	void removeEntity(std::string identifier);
	~Engine();

	void update(const float &dt);

	Settings getSettings() const;
	Camera* getCameraPtr();

	bool addComponent(Entity* entity, std::string componentIdentifier, Component* component);

	void addMeshComponent(MeshComponent* component);
	void createNewPhysicsComponent(Entity* entity, bool dynamic = false, std::string meshName = "", PxGeometryType::Enum geometryType = PxGeometryType::eBOX, std::string materialName = "default", bool isUnique = false);
	void addLightComponent(LightComponent* component); 
	void removeLightComponent(LightComponent* component);

	std::map<unsigned int long, MeshComponent*>* getMeshComponentMap();

	void buildTestStage();
};
