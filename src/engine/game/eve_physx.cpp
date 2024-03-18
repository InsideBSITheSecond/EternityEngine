#include "eve_physx.hpp"

namespace eve {
	using namespace physx;

	EvePhysx::EvePhysx() {
		mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
		if(!mFoundation)
			std::cerr << "PxCreateFoundation failed!" << std::endl;

		bool recordMemoryAllocations = true;

		mPvd = PxCreatePvd(*mFoundation);
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
		mPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);


		mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation,
			PxTolerancesScale(), recordMemoryAllocations, mPvd);
		if(!mPhysics)
			std::cerr << "PxCreatePhysics failed!" << std::endl;

		// Extension Library
		/*if (!PxInitExtensions(*mPhysics, mPvd))
    		std::cerr << "PxInitExtensions failed!" << std::endl;*/
	}

	EvePhysx::~EvePhysx() {
		mPhysics->release();
		mFoundation->release();
	}

	void EvePhysx::createPhysxSimulation() {
		
	}
}