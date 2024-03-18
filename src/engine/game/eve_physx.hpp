#pragma once

#include <iostream>

#include "PxPhysicsAPI.h"

namespace eve {

	using namespace physx;

	static PxDefaultErrorCallback gDefaultErrorCallback;
	static PxDefaultAllocator gDefaultAllocatorCallback;

	class EvePhysx {
		public:
			EvePhysx();
			~EvePhysx();

			void createPhysxSimulation();

			PxFoundation *mFoundation;
			PxPvd *mPvd;
			PxPhysics *mPhysics;
		private:

	};
}