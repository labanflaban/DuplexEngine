#pragma once
#include"3DPCH.h"
#include"Component.h"
#include"Physics.h"
#include"MeshComponent.h"

class PhysicsComponent : public Component
{
private:
	Physics* m_physicsPtr;
	PxRigidActor* m_actor;
	Transform* m_transform;
	Vector3 m_posOffset;
	PxShape* m_shape;
	std::vector<PxShape*> m_shapes;
	bool m_dynamic;
	bool m_controllRotation;


	physx::PxGeometry* createPrimitiveGeometry(physx::PxGeometryType::Enum geometryType, XMFLOAT3 min, XMFLOAT3 max, LRM_VERTEX vertexArray[], const int vertexCount)
	{
		PxGeometry* createdGeometry = nullptr;
		XMFLOAT3 vec = XMFLOAT3((max.x - min.x) / 2, (max.y - min.y) / 2, (max.z - min.z) / 2);
		switch (geometryType)
		{
		case physx::PxGeometryType::ePLANE:
			createdGeometry = new PxPlaneGeometry();
		case physx::PxGeometryType::eCAPSULE:
			createdGeometry = new PxCapsuleGeometry(vec.x, vec.y);
			break;
		case physx::PxGeometryType::eSPHERE:
			XMFLOAT3 center = { (max.x + min.x) * 0.5f, (max.y + min.y) * 0.5f, (max.z + min.z) * 0.5f };
			float radius;
			radius = 0;
			for (int i = 0; i < vertexCount; i++)
			{
				XMFLOAT3 position = vertexArray[i].pos;
				float tempDist = sqrt((position.x - center.x) * (position.x - center.x)
					+ (position.y - center.y) * (position.y - center.y)
					+ (position.z - center.z) * (position.z - center.z));
				if (tempDist > radius)
					radius = tempDist;
			}

			createdGeometry = new physx::PxSphereGeometry(radius);
			break;
		case physx::PxGeometryType::eBOX:
			createdGeometry = new physx::PxBoxGeometry((max.x - min.x) / 2, (max.y - min.y) / 2, (max.z - min.z) / 2);
			break;
		default:
			break;
		}

		return createdGeometry;
	}

	bool canAddGeometry(bool needsToBeStatic = false)
	{
		ErrorLogger* e = &ErrorLogger::get();
		if (!m_actor)
		{
			e->logError("No actor created before trying to add geometry!");
			return false;
		}

		if (m_shape)
		{
			e->logError("Shape already exists for actor.");
			return false;
		}

		if (needsToBeStatic)
		{
			if (m_dynamic)
			{
				e->logError("Actor is dynamic while geometry needs it to be static.");
				return false;
			}
		}

		return true;
	}
	
public:
	PhysicsComponent()
	{
		m_type = ComponentType::PHYSICS;
		m_physicsPtr = nullptr;
		m_dynamic = false;
		m_controllRotation = true;
	}
	PhysicsComponent(Physics* physics)
	{
		m_type = ComponentType::PHYSICS;
		m_physicsPtr = physics;
		m_dynamic = false;
		m_controllRotation = true;
	}
	~PhysicsComponent()
	{
		m_physicsPtr = nullptr;
		m_actor = nullptr;
		m_transform = nullptr;
		m_shape = nullptr;
	}

	void initActorAndShape(Entity* entity, MeshComponent* meshComponent, PxGeometryType::Enum geometryType, bool dynamic = false, std::string physicsMaterialName = "default", bool unique = false)
	{
		m_dynamic = dynamic;
		m_transform = entity->getTransform();
		XMFLOAT3 scale = entity->getScaling() * meshComponent->getScaling();
		std::string name = meshComponent->getFilePath() + std::to_string(geometryType);
		PxGeometry* geometry;
		m_actor = m_physicsPtr->createRigidActor(entity->getTranslation(), m_transform->getQuaternion(), dynamic);
		bool addGeom = true;

		if (this->canAddGeometry())
		{
			if (!unique)
			{
				if (scale.x == 1 && scale.y == 1 && scale.z == 1)
				{
					addGeom = false;
					m_shape = m_physicsPtr->getSharedShape(name);
					if (m_shape)
					{
						m_physicsPtr->addShapeToActor(m_actor, m_shape);
					}
					else //Create shape and add shape for sharing
					{
						geometry = addGeometryByModelData(geometryType, meshComponent, physicsMaterialName, unique);
						m_shape = m_physicsPtr->createAndSetShapeForActor(m_actor, geometry, physicsMaterialName, unique, entity->getScaling());
						m_physicsPtr->addShapeForSharing(m_shape, name);
					}
				}

			}
			if(addGeom)
			{
				geometry = addGeometryByModelData(geometryType, meshComponent, physicsMaterialName, unique);
				m_shape = m_physicsPtr->createAndSetShapeForActor(m_actor, geometry, physicsMaterialName, unique, entity->getScaling());
			}
		}
		

	}

	void initActor(Entity* entity, bool dynamic)
	{
		m_dynamic = dynamic;
		m_transform = entity->getTransform();
		if (!m_actor)
		{
			m_actor = m_physicsPtr->createRigidActor(entity->getTranslation(), entity->getQuaternion(), dynamic);
		}
		else
		{
			ErrorLogger::get().logError("Trying to initialize an already initalized actor. Nothing happened.");
		}
	}

	void addBoxShape(XMFLOAT3 halfExtends, std::string materialName = "default", bool unique = true)
	{
		PxGeometryHolder geometry;
		if (canAddGeometry())
		{
			geometry = PxBoxGeometry(PxVec3(halfExtends.x, halfExtends.y, halfExtends.z));
			m_shape = m_physicsPtr->createAndSetShapeForActor(m_actor, geometry, materialName, unique);
		}
	}

	void addSphereShape(float radius, std::string materialName = "default", bool unique = true)
	{
		PxGeometryHolder geometry;
		if (canAddGeometry())
		{
			geometry = PxSphereGeometry(radius);
			m_shape = m_physicsPtr->createAndSetShapeForActor(m_actor, geometry, materialName, unique);
		}
	}

	void addPlaneShape(std::string materialName = "default", bool unique = true)
	{
		PxGeometryHolder geometry;
		if (canAddGeometry(true))
		{
			geometry = PxPlaneGeometry();
			m_shape = m_physicsPtr->createAndSetShapeForActor(m_actor, geometry, materialName, unique);
		}
	}

	void addCapsuleShape(float radius, float halfHeight, std::string materialName = "default", bool unique = true)
	{
		PxGeometryHolder geometry;
		if (canAddGeometry())
		{
			geometry = PxCapsuleGeometry(radius, halfHeight);
			m_shape = m_physicsPtr->createAndSetShapeForActor(m_actor, geometry, materialName, unique);
		}
	}


	PxGeometry* addGeometryByModelData(PxGeometryType::Enum geometry, MeshComponent* meshComponent, std::string materialName, bool unique)
	{
		XMFLOAT3 min, max;
		PxGeometry* bb = nullptr;
		meshComponent->getMeshResourcePtr()->getMinMax(min, max);
		
		std::string name = meshComponent->getFilePath() + std::to_string(geometry);
		bb = m_physicsPtr->getGeometry(name);
		if (!bb)
		{
			bb = createPrimitiveGeometry(geometry, min, max, meshComponent->getMeshResourcePtr()->getVertexArray(), meshComponent->getMeshResourcePtr()->getVertexBuffer().getSize());
			m_physicsPtr->addGeometry(name, bb);
		}
		
		return bb;
	}


	void setPhysicsMaterial(std::string phycisMaterialName)
	{
		m_physicsPtr->setShapeMaterial(m_shape, phycisMaterialName);

	}

	void setMass(float mass)
	{
		m_dynamic ? m_physicsPtr->setMassOfActor(m_actor, mass) : ErrorLogger::get().logError("Trying to change mass on a static actor.");
	}


	void addForce(XMFLOAT3 forceAdd, bool massIndependant = true)
	{
		PxVec3 force(forceAdd.x, forceAdd.y, forceAdd.z);
		if (m_dynamic)
		{
			PxRigidDynamic* rigid = static_cast<PxRigidDynamic*>(m_actor);
			rigid->addForce(force, massIndependant ? PxForceMode::eVELOCITY_CHANGE : PxForceMode::eACCELERATION, true);
		}
		else
			ErrorLogger::get().logError("Tried to add force to static physicsComponent!");
	}

	void addForce(XMVECTOR forceAdd, bool massIndependant = true)
	{
		XMFLOAT3 forceFloat3;
		XMStoreFloat3(&forceFloat3, forceAdd);

		this->addForce(forceFloat3, massIndependant);
	}

	// Update
	void update(float dt) override 
	{
		m_transform->translation(this->getActorPosition());
		if(m_controllRotation)
			m_transform->setQuaternion(this->getActorQuaternion());
	}

	XMFLOAT3 getActorPosition()
	{
		return XMFLOAT3(m_actor->getGlobalPose().p.x, m_actor->getGlobalPose().p.y, m_actor->getGlobalPose().p.z);
	}

	XMFLOAT4 getActorQuaternion()
	{
		return XMFLOAT4(m_actor->getGlobalPose().q.x, m_actor->getGlobalPose().q.y, m_actor->getGlobalPose().q.z, m_actor->getGlobalPose().q.w);
	}

	void controllRotation(bool shouldControllRotation)
	{
		m_controllRotation = shouldControllRotation;
	}

	void setPosition(XMFLOAT3 pos)
	{
		m_physicsPtr->setPosition(m_actor, pos);
	}

	void setRotation(XMFLOAT4 rotQ)
	{
		m_physicsPtr->setRotation(m_actor, rotQ);
	}

	void setPose(XMFLOAT3 pos, XMFLOAT4 rotQ)
	{
		m_physicsPtr->setGlobalTransform(m_actor, pos, rotQ);
	}

	bool checkGround(Vector3 origin, Vector3 unitDirection, float distance)
	{
		return m_physicsPtr->castRay(origin, unitDirection, distance);
	}
};
