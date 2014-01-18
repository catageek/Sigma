#ifndef BENCHMARK_H_INCLUDED
#define BENCHMARK_H_INCLUDED

#include <random>
#include <chrono>

#include "systems/FactorySystem.h"
#include "components/PhysicalWorldLocation.h"
#include "components/ControllableMove.h"
#include "components/RigidBody.h"
#include <bullet/btBulletCollisionCommon.h>
#include <bullet/btBulletDynamicsCommon.h>
#include "glm/glm.hpp"

namespace Sigma {
	class Benchmark {
	public:
		Benchmark(unsigned int n) : n(n) {
			seed = (unsigned long int) std::chrono::system_clock::now().time_since_epoch().count();
			random = std::default_random_engine(seed);
		};

		virtual ~Benchmark() {};

		void CreateEntities(FactorySystem& f) {
			for (auto i = 10000; i < 10000 + n; i++) {
				std::vector<Property> prop;
				Property p("shape", std::string("sphere"));
				prop.push_back(p);
				prop.emplace_back(Property("radius", 100.0f));
				ControllableMove::AddEntity(i);
				PhysicalWorldLocation::AddEntityPosition(i, next(), next() + 600,  next()\
					, next()/100000, next()/100000, next()/100000);
				RigidBody::AddEntity(i, prop);
				forces.emplace_back(glm::vec3(next() / 100, next() / 100, next() /100));
			}
			for (auto i = 10000; i < 10000 + n; i++) {
				std::vector<Property> prop;
				Property p("texture", std::string("assets/mars"));
//					prop.push_back(p);
				Property q("subdivisions", int(1));
				prop.push_back(q);
				Property r("shader", std::string("shaders/cubesphere"));
//					prop.push_back(r);
				Property s("cull_face", std::string("none"));
				prop.push_back(s);
				prop.emplace_back(Property("x", float(PhysicalWorldLocation::getPosition(i)->x)));
				prop.emplace_back(Property("y", float(PhysicalWorldLocation::getPosition(i)->y)));
				prop.emplace_back(Property("z", float(PhysicalWorldLocation::getPosition(i)->z)));
				prop.emplace_back(Property("scale", 10.0f));
				Property t("lightEnabled", bool(false));
				prop.push_back(t);
				Property u("id", int(1));
				prop.push_back(u);
				f.create("GLIcoSphere", i, prop);
			}
		}

		void ApplyForces() {
			auto itr = forces.cbegin();
			for (auto i = 10000; i < 10000 + n; i++, itr++) {
				ControllableMove::AddForce(i, *itr);
			}
		}

        float next() {
            return ((float) (1000 * random() / random.max()) - 500);
        }


	private:
        unsigned long int seed;
        unsigned int n;
        std::default_random_engine random;
		std::vector<glm::vec3> forces;
	};
}

#endif // BENCHMARK_H_INCLUDED
