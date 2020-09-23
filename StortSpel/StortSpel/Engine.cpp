#include "3DPCH.h"
#include "Engine.h"

Engine& Engine::get()
{
	static Engine instance;
	return instance;
}

Entity* Engine::getEntity(std::string key)
{
	if (m_entities.find(key) != m_entities.end())
		return m_entities[key];
	else
		return nullptr;
}

bool Engine::addEntity(std::string identifier)
{
	if (m_entities.find(identifier) != m_entities.end())// If one with that name is already found
	{
		return false;
	}

	m_entities[identifier] = new Entity(identifier);

	return true;
}

Engine::~Engine()
{
	if (m_player)
		delete m_player;
}



void Engine::update(Mouse* mousePtr, Keyboard* keyboardPtr, const float& dt)
{
	m_player->updatePlayer(keyboardPtr, dt);
	
}

void Engine::Init()
{
	m_player = new Player();
	if (addEntity("meshPlayer"))
	{
		m_entities["meshPlayer"]->addComponent("mesh", new MeshComponent("../res/models/testTania_tania_geo.lrm"));
		m_entities["meshPlayer"]->move({ 1, -1, 0 });
		m_entities["meshPlayer"]->scaleUniform(0.02f);
		m_player->setPlayerEntity(m_entities["meshPlayer"]);
	}
	else
	{
		ErrorLogger::get().logError("No player model added or already exists when adding");
	}


	// Temp entity init
	addEntity("first");
	m_entities["first"]->addComponent("test", new TestComponent());


	if(addEntity("meshTest"))
		m_entities["meshTest"]->addComponent("mesh", new MeshComponent("../res/models/testCube_pCube1.lrm"));

	if (addEntity("meshTest1"))
	{
		m_entities["meshTest1"]->addComponent("mesh", new MeshComponent("../res/models/testCube_pCube1.lrm"));
		m_entities["meshTest1"]->move({ 2, 1, 1});
		
	}
		
}
