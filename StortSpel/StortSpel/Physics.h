#pragma once
#include"3DPCH.h"
#include"PhysicsMaterial.h"

static physx::PxDefaultErrorCallback gDefaultErrorCallback;
static physx::PxDefaultAllocator gDefaultAllocatorCallback;
using namespace physx;

class Physics
{
private:
	const char* m_host = "localhost";
	PxFoundation* m_foundationPtr;
	PxPhysics* m_physicsPtr;
	PxPvd* m_PvdPtr;
	PxCpuDispatcher* m_dispatcherPtr;
	PxScene* m_scenePtr;
	PxControllerManager* m_controllManager;

	std::map<std::string, PxMaterial*> m_defaultMaterials;
	std::map<std::string, PxGeometry*> m_sharedGeometry;
	std::map<std::string, PxShape*> m_sharedShapes;

	bool m_recordMemoryAllocations = true;

	Physics()
	{
		m_foundationPtr = nullptr;
		m_physicsPtr = nullptr;
		m_PvdPtr = nullptr;
		m_dispatcherPtr = nullptr;
		m_scenePtr = nullptr;
		m_controllManager = nullptr;
	}

	void loadDefaultMaterials()
	{
		std::vector<PhysicsMaterial> physicMaterialVector;
		PhysicsMaterial::createDefaultMaterials(physicMaterialVector);
		for (int i = 0; i < physicMaterialVector.size(); i++)
		{
			this->addPhysicsMaterial(physicMaterialVector[i]);
		}
	}

	PxRigidActor* createDynamicActor(PxVec3 pos, PxQuat rot)
	{
		return m_physicsPtr->createRigidDynamic(PxTransform(pos, rot));
	}

	PxRigidActor* createStaticActor(PxVec3 pos, PxQuat rot)
	{
		return m_physicsPtr->createRigidStatic(PxTransform(pos, rot));
	}

	PxGeometryHolder scaleGeometry(PxGeometry* geometry, XMFLOAT3 scale)
	{
		PxGeometryHolder geometryHolder = *geometry;
		PxVec3 pxScale(scale.x, scale.y, scale.z);
		switch (geometryHolder.getType())
		{
		case physx::PxGeometryType::ePLANE:
			break;
		case physx::PxGeometryType::eCAPSULE:
			break;
		case physx::PxGeometryType::eSPHERE: //Jump into same since we can use box data
			geometryHolder.sphere().radius *= pxScale.maxElement();
			break;
		case physx::PxGeometryType::eBOX:
			 geometryHolder.box().halfExtents = geometryHolder.box().halfExtents.multiply(pxScale);
			break;
		}

		return geometryHolder;
	}

public:
	Physics(const Physics&) = delete;
	void operator=(Physics const&) = delete;
	static Physics& get()
	{
		static Physics instance;
		return instance;
	}

	~Physics()
	{

	}

	void release()
	{
		m_controllManager->release();
		m_scenePtr->release();
		m_scenePtr = nullptr;
		m_physicsPtr->release();
		m_PvdPtr->release();
		delete m_dispatcherPtr;
		PxCloseExtensions();
		m_foundationPtr->release();
	}

	void init(const XMFLOAT3 &gravity = {0.0f, -9.81f, 0.0f}, const int &nrOfThreads = 1)
	{
		m_foundationPtr = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);

		m_PvdPtr = PxCreatePvd(*m_foundationPtr);
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate(m_host, 5425, 10);
		m_PvdPtr->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);


		m_physicsPtr = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundationPtr,
			physx::PxTolerancesScale(), m_recordMemoryAllocations, m_PvdPtr);
		if (!m_physicsPtr)
			assert("PxCreatePhysics failed!");

		if (!PxInitExtensions(*m_physicsPtr, m_PvdPtr))
			assert("PxInitExtensions failed!");

		physx::PxSceneDesc sceneDesc(m_physicsPtr->getTolerancesScale());
		sceneDesc.gravity = PxVec3(gravity.x, gravity.y, gravity.z);
		m_dispatcherPtr = PxDefaultCpuDispatcherCreate(nrOfThreads);
		sceneDesc.cpuDispatcher = m_dispatcherPtr;
		sceneDesc.filterShader = PxDefaultSimulationFilterShader;
		m_scenePtr = m_physicsPtr->createScene(sceneDesc);
		m_controllManager = PxCreateControllerManager(*m_scenePtr);

		PxPvdSceneClient* pvdClient = m_scenePtr->getScenePvdClient();
		if (pvdClient)
		{
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
		this->loadDefaultMaterials();
	}

	void addShapeForSharing(PxShape* shape, const std::string &name)
	{
		if (shape->isExclusive())
			ErrorLogger::get().logError("A shape being added for sharing is exclusive.");
		else
		{
			if (m_sharedShapes.find(name) == m_sharedShapes.end())
				m_sharedShapes[name] = shape;
		}
	}

	PxShape* getSharedShape(const std::string &name)
	{
		if (m_sharedShapes.find(name) != m_sharedShapes.end())
			return m_sharedShapes[name];

		return nullptr;
	}

	PxShape* createAndSetShapeForActor(PxRigidActor* actor, PxGeometry* geometry, const std::string &materialName, const bool &unique, const XMFLOAT3 &scale = { 1, 1, 1 })
	{
		physx::PxMaterial* physicsMaterial = getMaterialByName(materialName);
		PxGeometryHolder scaledGeometry = *geometry;
		if (physicsMaterial == nullptr)
		{
			physicsMaterial = m_defaultMaterials["default"];
			physicsMaterial ? ErrorLogger::get().logError(std::string("Physical material:" + materialName + " does not exist, default is used.").c_str()) : assert("Default material has been changed or cannot be accessed, a recent change in code did this.");
		}
		if (scale.x != 1 || scale.y != 1 || scale.z != 1)
			scaledGeometry = scaleGeometry(geometry, scale);


		PxShape* shape = m_physicsPtr->createShape(scaledGeometry.any(), *physicsMaterial, unique);
		actor->attachShape(*shape);
		return shape;
	}

	PxShape* createAndSetShapeForActor(PxRigidActor* actor, PxGeometryHolder geometry, const std::string &materialName, const bool &unique, const XMFLOAT3 &scale = { 1, 1, 1 })
	{
		physx::PxMaterial* physicsMaterial = getMaterialByName(materialName);
		if (physicsMaterial == nullptr)
		{
			physicsMaterial = m_defaultMaterials["default"];
			physicsMaterial ? ErrorLogger::get().logError("Physical material entered does not exist, default is used.") : assert("Default material has been changed or cannot be accessed, a recent change in code did this.");
		}

		PxShape* shape = m_physicsPtr->createShape(geometry.any(), *physicsMaterial, unique);
		actor->attachShape(*shape);
		return shape;
	}

	PxMaterial* getMaterialByName(const std::string &name)
	{
		PxMaterial* materialPtr = nullptr;
		if (m_defaultMaterials.find(name) != m_defaultMaterials.end())
		{
			materialPtr = m_defaultMaterials[name];
		}
		return materialPtr;
	}

	void addShapeToActor(PxRigidActor* actor, PxShape* shape)
	{
		actor->attachShape(*shape);
	}

	PxRigidActor* createRigidActor(const XMFLOAT3 &position, const XMFLOAT4& quaternion, const bool &dynamic)
	{
		PxVec3 pos(position.x, position.y, position.z);
		PxQuat quat(quaternion.x, quaternion.y, quaternion.z, quaternion.w);

		PxRigidActor* actor = dynamic ? createDynamicActor(pos, quat) : createStaticActor(pos, quat);
		m_scenePtr->addActor(*actor);
		return actor;
	}

	void setPosition(PxRigidActor* actor, const XMFLOAT3 &pos)
	{
		actor->setGlobalPose(PxTransform(PxVec3(pos.x, pos.y, pos.z)));
	}

	void setRotation(PxRigidActor* actor, const XMFLOAT4 &rotationQuat)
	{
		actor->setGlobalPose(PxTransform(PxQuat(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w)));
	}

	void setGlobalTransform(PxRigidActor* actor, const XMFLOAT3 &pos,  const XMFLOAT4 &rotQ)
	{
		actor->setGlobalPose(PxTransform(pos.x, pos.y, pos.z, PxQuat(rotQ.x, rotQ.y, rotQ.z, rotQ.w)));
	}



	void setMassOfActor(PxRigidActor* actor, const float &weight)
	{
		static_cast<PxRigidDynamic*>(actor)->setMass(weight);
	}

	void addPhysicsMaterial(const PhysicsMaterial &physicsMaterial)
	{
		if (m_defaultMaterials.find(physicsMaterial.name) == m_defaultMaterials.end())
		{
			PxMaterial* currentPhysXMaterial = m_defaultMaterials[physicsMaterial.name] = m_physicsPtr->createMaterial(physicsMaterial.staticFriction, physicsMaterial.dynamicFriction, physicsMaterial.restitution);
			currentPhysXMaterial->setFrictionCombineMode(physicsMaterial.frictionCombineMode);
			currentPhysXMaterial->setRestitutionCombineMode(physicsMaterial.restitutionCombineMode);
		}
		else
			ErrorLogger::get().logError(std::string("Trying to add a duplicate of material" + physicsMaterial.name).c_str());
	}

	void addPhysicsMaterial(const std::string &name, const float &staticFriction, const float &dynamicFriction, const float &restitution, const physx::PxCombineMode::Enum &frictionCombineMode, const physx::PxCombineMode::Enum &restitutionCombineMode)
	{
		this->addPhysicsMaterial(PhysicsMaterial(name, staticFriction, dynamicFriction, restitution, frictionCombineMode, restitutionCombineMode));
	}

	void setShapeMaterial(PxShape* shape, const std::string &materialName)
	{
		PxMaterial* materials[1];
		materials[0] = m_defaultMaterials[materialName];
		shape->isExclusive() ? shape->setMaterials(materials, 1) : ErrorLogger::get().logError("Trying to change material on shape that is not exclusive, this is not possible.");
	}

	void update(const float &dt)
	{
		m_scenePtr->simulate(dt);
		m_scenePtr->fetchResults(true);

	}

	PxGeometry* getGeometry(const std::string &geometryName)
	{
		if (m_sharedGeometry.find(geometryName) != m_sharedGeometry.end())
			return m_sharedGeometry[geometryName];
		else
			return nullptr;
	}

	void addGeometry(const std::string &geometryName, PxGeometry* geometry)
	{
		if (m_sharedGeometry.find(geometryName) == m_sharedGeometry.end())
			m_sharedGeometry[geometryName] = geometry;
		else
			ErrorLogger::get().logError("Trying to add already existing geometry to sharedGeometry map");
	}

	bool castRay(const SimpleMath::Vector3 &origin, const SimpleMath::Vector3 &unitDirection, const float &distance)
	{
		PxVec3 pOrigin(origin.x, origin.y, origin.z);
		PxVec3 pUnitDir(unitDirection.x, unitDirection.y, unitDirection.z);
		PxRaycastBuffer hit;                 // [out] Raycast results

		// Raycast against all static & dynamic objects (no filtering)
		// The main result from this call is the closest hit, stored in the 'hit.block' structure
		return m_scenePtr->raycast(pOrigin, pUnitDir, distance, hit);
	}

	//Manager
	PxController* addCapsuleController(const XMFLOAT3 &position, const float &height, const float &radius, const std::string &materialName, PxControllerBehaviorCallback* controlBehavior)
	{
		PxController* capsuleController = nullptr;
		PxCapsuleControllerDesc ccd;
		ccd.climbingMode = PxCapsuleClimbingMode::eCONSTRAINED;
		ccd.contactOffset = 0.1f;
		ccd.height = height;
		ccd.radius = radius;
		ccd.invisibleWallHeight = 0.f;
		ccd.maxJumpHeight = 0.f; //If invisibleWalLHeigt is used, this parameter is used.
		ccd.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
		ccd.position = PxExtendedVec3(position.x, position.y, position.z);
		ccd.registerDeletionListener = true;
		ccd.slopeLimit = 0.707f;
		ccd.stepOffset = 0.5f;
		ccd.upDirection = PxVec3(0.f, 1.f, 0.f);
		ccd.userData = NULL;
		ccd.volumeGrowth = 1.5f;

		//Callbacks
		ccd.reportCallback = NULL;
		ccd.behaviorCallback = controlBehavior;

		//Actor
		ccd.density = 10.0;
		ccd.material = getMaterialByName(materialName);
		ccd.scaleCoeff = 0.8f;

		capsuleController = m_controllManager->createController(ccd);

		return capsuleController;
	}

	void setCapsuleSize(PxController* controller, const float &height)
	{
		controller->resize(height);
	}

	void setCapsuleRadius(PxController* controller, const float &radius)
	{
		PxCapsuleController* capsule = static_cast<PxCapsuleController*>(controller);
		capsule->setRadius(radius);

	}
};
