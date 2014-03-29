#ifndef USERSYSTEM_H_INCLUDED
#define USERSYSTEM_H_INCLUDED

namespace Sigma {
	class UserSystem : public Sigma::IFactory {
	public:
		DLL_EXPORT UserSystem() {};
		DLL_EXPORT virtual ~UserSystem() {};

		/**
		 * \brief Starts the User system.
		 *
		 * \return bool Returns false on startup failure.
		 */
		DLL_EXPORT bool Start() {};

		std::map<std::string,IECSFactory::FactoryFunction> getECSFactoryFunctions() {
			using namespace std::placeholders;
			std::map<std::string,Sigma::IECSFactory::FactoryFunction> retval;
			retval[NetworkNode::getComponentTypeName()] = std::bind(&NetworkNode::AddEntity,_1);
			return retval;
		}
	};
}


#endif // USERSYSTEM_H_INCLUDED
