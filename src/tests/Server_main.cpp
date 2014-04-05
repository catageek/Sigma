#include <iostream>
#include <unistd.h>
#include "systems/BulletPhysics.h"
#include "systems/FactorySystem.h"
#include "entities/BulletMover.h"
#include "SCParser.h"
#include "systems/network/NetworkSystem.h"
#include "OS.h"

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argCount, char **argValues) {
	Log::Print::Init(); // Initializing the Logger must be done first.

	Sigma::OS glfwos;

	Sigma::BulletPhysics bphys;

	Sigma::NetworkSystem netsys;

	Sigma::FactorySystem& factory = Sigma::FactorySystem::getInstance();

	// EntitySystem can add components
	Sigma::EntitySystem entitySystem(&factory);

	factory.register_Factory(bphys);
	factory.register_ECSFactory(bphys);

	///////////////////
	// Setup physics //
	///////////////////

	bphys.Start();

	/////////////////////////////
	// Start to listen network //
	/////////////////////////////

	Sigma::ThreadPool::Initialize(5);
	netsys.SetTCPHandler<false>();
	netsys.Server_Start("127.0.0.1", 7777);


	// Create hard coded entity ID #1
	// position is hardcoded
	std::vector<Property> properties;
	properties.emplace_back(Property("x", 0.0f));
	properties.emplace_back(Property("y", 0.0f));
	properties.emplace_back(Property("z", 0.0f));
	properties.emplace_back(Property("rx", 0.0f));
	properties.emplace_back(Property("ry", 0.0f));
	properties.emplace_back(Property("rz", 0.0f));
	Property v("shape", std::string("capsule"));
	properties.push_back(v);
	properties.emplace_back(Property("radius", 0.3f));
	properties.emplace_back(Property("height", 1.3f));
	Sigma::BulletMover mover(1, properties);
	mover.InitializeRigidBody(properties);

	////////////////
	// Load scene //
	////////////////

	// Parse the scene file to retrieve entities
	Sigma::parser::SCParser parser;

	LOG << "Parsing test.sc scene file.";
	if (!parser.Parse("test.sc")) {
		LOG_ERROR << "Failed to load entities from file.";
		exit (-1);
	}

	LOG << "Generating Entities.";

	// Create each entity's components
	for (unsigned int i = 0; i < parser.EntityCount(); ++i) {
		Sigma::parser::Entity* e = parser.GetEntity(i);
		for (auto itr = e->components.begin(); itr != e->components.end(); ++itr) {

			if (! factory.create(itr->type,e->id, const_cast<std::vector<Property>&>(itr->properties))) {
				factory.createECS(itr->type,e->id, const_cast<std::vector<Property>&>(itr->properties));
			};
		}
	}

	// Call now to clear the delta after startup.
	glfwos.GetDeltaTime();

	LOG << "Main loop begins ";
	while (1) {
		// Get time in ms, store it in seconds too
		double deltaSec = glfwos.GetDeltaTime();
		sleep(10000);

		///////////////////////
		// Update subsystems //
		///////////////////////

		// Pass in delta time in seconds
//		bphys.Update(deltaSec);
	}

	// do a proper clean up
	glfwos.Terminate();

	return 0;
}
