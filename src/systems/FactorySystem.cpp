#include "systems/FactorySystem.h"
#include "systems/CompositeSystem.h"
#include "Sigma.h"

namespace Sigma{
	std::shared_ptr<FactorySystem> FactorySystem::_instance;

	FactorySystem::FactorySystem() {
		// nothing to construct
	}

	FactorySystem::~FactorySystem(){
		// destruction of private members is handled by stl
	}

	/**
	 * \brief return or create the singleton instance
	 */
	FactorySystem& FactorySystem::getInstance(){
		if(!_instance){
			FactorySystem::_instance.reset( new FactorySystem() );
		}
		return *FactorySystem::_instance;
	}

	IComponent* FactorySystem::create(const std::string& type,
							   const id_t entityID,
							   const std::vector<Property> &properties){
		if(registeredFactoryFunctions.find(type) != registeredFactoryFunctions.end()){
			LOG << "Creating component of type: " << type;
			return registeredFactoryFunctions[type](entityID, properties);
		}
		else {
			return nullptr;
		}
	}

	void FactorySystem::createECS(const CompositeID& type,
							const id_t entityID,
							const std::vector<Property> &properties){
		if(registeredECSFactoryFunctions.find(type) != registeredECSFactoryFunctions.end()){
			std::cerr << "Creating ECS component of type: " << type << " with id #" << entityID << std::endl;
			registeredECSFactoryFunctions[type](entityID, properties);
		}
		else {
			std::cerr << "Error: Couldn't find component: " << type << std::endl;
		}
	}

	void FactorySystem::register_Factory(IFactory& Factory){
		const auto& factoryfunctions = Factory.getFactoryFunctions();
		for(auto FactoryFunc = factoryfunctions.begin(); FactoryFunc != factoryfunctions.end(); ++FactoryFunc){
			LOG << "Registering component factory of type: " << FactoryFunc->first;
			registeredFactoryFunctions[FactoryFunc->first]=FactoryFunc->second;
		}
	}

	void FactorySystem::register_ECSFactory(IECSFactory& Factory){
		const auto& factoryfunctions = Factory.getECSFactoryFunctions();
		for(auto FactoryFunc = factoryfunctions.begin(); FactoryFunc != factoryfunctions.end(); ++FactoryFunc){
			std::cerr << "Registering ECS composite factory of type: " << FactoryFunc->first << std::endl;
			registeredECSFactoryFunctions[FactoryFunc->first]=FactoryFunc->second;
		}
	}
} // namespace Sigma
