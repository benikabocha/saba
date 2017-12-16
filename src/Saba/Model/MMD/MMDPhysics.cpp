//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "MMDPhysics.h"

#include "MMDNode.h"
#include "MMDModel.h"
#include "Saba/Base/Log.h"

#include <glm/gtc/matrix_transform.hpp>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

namespace saba
{
	class MMDMotionState : public btMotionState
	{
	public:
		virtual void Reset() = 0;
		virtual void ReflectGlobalTransform() = 0;
	};

	namespace
	{
		glm::mat4 InvZ(const glm::mat4& m)
		{
			const glm::mat4 invZ = glm::scale(glm::mat4(), glm::vec3(1, 1, -1));
			return invZ * m * invZ;
		}
	}

	struct MMDFilterCallback : public btOverlapFilterCallback
	{
		bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const override
		{
			auto findIt = std::find_if(
				m_nonFilterProxy.begin(),
				m_nonFilterProxy.end(),
				[proxy0, proxy1](const auto& x) {return x == proxy0 || x == proxy1; }
			);
			if (findIt != m_nonFilterProxy.end())
			{
				return true;
			}
			bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
			collides = collides && (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);
			return collides;
		}

		std::vector<btBroadphaseProxy*> m_nonFilterProxy;
	};

	MMDPhysics::MMDPhysics()
	{
	}

	MMDPhysics::~MMDPhysics()
	{
		Destroy();
	}

	bool MMDPhysics::Create()
	{
		m_broadphase = std::make_unique<btDbvtBroadphase>();
		m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
		m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());

		m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();

		m_world = std::make_unique<btDiscreteDynamicsWorld>(
			m_dispatcher.get(),
			m_broadphase.get(),
			m_solver.get(),
			m_collisionConfig.get()
			);

		m_world->setGravity(btVector3(0, -9.8f * 10.0f, 0));

		m_groundShape = std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0.0f);

		btTransform groundTransform;
		groundTransform.setIdentity();

		m_groundMS = std::make_unique<btDefaultMotionState>(groundTransform);

		btRigidBody::btRigidBodyConstructionInfo groundInfo(0, m_groundMS.get(), m_groundShape.get(), btVector3(0, 0, 0));
		m_groundRB = std::make_unique<btRigidBody>(groundInfo);

		m_world->addRigidBody(m_groundRB.get());

		auto filterCB = std::make_unique<MMDFilterCallback>();
		filterCB->m_nonFilterProxy.push_back(m_groundRB->getBroadphaseProxy());
		m_world->getPairCache()->setOverlapFilterCallback(filterCB.get());
		m_filterCB = std::move(filterCB);

		return true;
	}

	void MMDPhysics::Destroy()
	{
		if (m_world != nullptr && m_groundRB != nullptr)
		{
			m_world->removeRigidBody(m_groundRB.get());
		}

		m_broadphase = nullptr;
		m_collisionConfig = nullptr;
		m_dispatcher = nullptr;
		m_solver = nullptr;
		m_world = nullptr;
		m_groundShape = nullptr;
		m_groundMS = nullptr;
		m_groundRB = nullptr;
	}

	void MMDPhysics::Update(float time)
	{
		if (m_world != nullptr)
		{
			m_world->stepSimulation(time, 10, 1.0f / 120.0f);
		}
	}

	void MMDPhysics::AddRigidBody(MMDRigidBody * mmdRB)
	{
		m_world->addRigidBody(
			mmdRB->GetRigidBody(),
			1 << mmdRB->GetGroup(),
			mmdRB->GetGroupMask()
		);
	}

	void MMDPhysics::RemoveRigidBody(MMDRigidBody * mmdRB)
	{
		m_world->removeRigidBody(mmdRB->GetRigidBody());
	}

	void MMDPhysics::AddJoint(MMDJoint * mmdJoint)
	{
		if (mmdJoint->GetConstraint() != nullptr)
		{
			m_world->addConstraint(mmdJoint->GetConstraint());
		}
	}

	void MMDPhysics::RemoveJoint(MMDJoint * mmdJoint)
	{
		if (mmdJoint->GetConstraint() != nullptr)
		{
			m_world->removeConstraint(mmdJoint->GetConstraint());
		}
	}

	btDiscreteDynamicsWorld * MMDPhysics::GetDynamicsWorld() const
	{
		return m_world.get();
	}

	//*******************
	// MMDRigidBody
	//*******************

	class DefaultMotionState : public MMDMotionState
	{
	public:
		DefaultMotionState(const glm::mat4& transform)
		{
			glm::mat4 trans = InvZ(transform);
			m_transform.setFromOpenGLMatrix(&trans[0][0]);
			m_initialTransform = m_transform;
		}

		void getWorldTransform(btTransform& worldTransform) const override
		{
			worldTransform = m_transform;
		}

		void setWorldTransform(const btTransform& worldTransform) override
		{
			m_transform = worldTransform;
		}

		virtual void Reset() override
		{
			m_transform = m_initialTransform;
		}

		virtual void ReflectGlobalTransform() override
		{
		}


	private:
		btTransform	m_initialTransform;
		btTransform	m_transform;
	};

	class DynamicMotionState : public MMDMotionState
	{
	public:
		DynamicMotionState(MMDNode* node, const glm::mat4& offset, bool override = true)
			: m_node(node)
			, m_offset(offset)
			, m_override(override)
		{
			m_invOffset = glm::inverse(offset);
			Reset();
		}

		void getWorldTransform(btTransform& worldTransform) const override
		{
			worldTransform = m_transform;
		}

		void setWorldTransform(const btTransform& worldTransform) override
		{
			m_transform = worldTransform;
		}

		void Reset() override
		{
			glm::mat4 global = InvZ(m_node->GetGlobalTransform() * m_offset);
			m_transform.setFromOpenGLMatrix(&global[0][0]);
		}

		void ReflectGlobalTransform() override
		{
			glm::mat4 world;
			m_transform.getOpenGLMatrix(&world[0][0]);
			glm::mat4 btGlobal = InvZ(world) * m_invOffset;

			if (m_override)
			{
				m_node->SetGlobalTransform(btGlobal);
			}
		}

	private:
		MMDNode*	m_node;
		glm::mat4	m_offset;
		glm::mat4	m_invOffset;
		btTransform	m_transform;
		bool		m_override;
	};

	class DynamicAndBoneMergeMotionState : public MMDMotionState
	{
	public:
		DynamicAndBoneMergeMotionState(MMDNode* node, const glm::mat4& offset, bool override = true)
			: m_node(node)
			, m_offset(offset)
			, m_override(override)
		{
			m_invOffset = glm::inverse(offset);
			Reset();
		}

		void getWorldTransform(btTransform& worldTransform) const override
		{
			worldTransform = m_transform;
		}

		void setWorldTransform(const btTransform& worldTransform) override
		{
			m_transform = worldTransform;
		}

		void Reset() override
		{
			glm::mat4 global = InvZ(m_node->GetGlobalTransform() * m_offset);
			m_transform.setFromOpenGLMatrix(&global[0][0]);
		}

		void ReflectGlobalTransform() override
		{
			glm::mat4 world;
			m_transform.getOpenGLMatrix(&world[0][0]);
			glm::mat4 btGlobal = InvZ(world) * m_invOffset;
			glm::mat4 global = m_node->GetGlobalTransform();
			btGlobal[3] = global[3];

			if (m_override)
			{
				m_node->SetGlobalTransform(btGlobal);
				m_node->UpdateChildTransform();
			}
		}

	private:
		MMDNode*	m_node;
		glm::mat4	m_offset;
		glm::mat4	m_invOffset;
		btTransform	m_transform;
		bool		m_override;

	};

	class KinematicMotionState : public MMDMotionState
	{
	public:
		KinematicMotionState(MMDNode* node, const glm::mat4& offset)
			: m_node(node)
			, m_offset(offset)
		{
		}

		void getWorldTransform(btTransform& worldTransform) const override
		{
			glm::mat4 m;
			if (m_node != nullptr)
			{
				m = m_node->GetGlobalTransform() * m_offset;
			}
			else
			{
				m = m_offset;
			}
			m = InvZ(m);
			worldTransform.setFromOpenGLMatrix(&m[0][0]);
		}

		void setWorldTransform(const btTransform& worldTransform) override
		{
		}

		void Reset() override
		{
		}

		void ReflectGlobalTransform() override
		{
		}

	private:
		MMDNode*	m_node;
		glm::mat4	m_offset;
	};

	MMDRigidBody::MMDRigidBody()
	{
	}

	MMDRigidBody::~MMDRigidBody()
	{
	}

	bool MMDRigidBody::Create(const PMDRigidBodyExt& pmdRigidBody, MMDModel* model, MMDNode* node)
	{
		Destroy();

		switch (pmdRigidBody.m_shapeType)
		{
		case PMDRigidBodyShape::Sphere:
			m_shape = std::make_unique<btSphereShape>(pmdRigidBody.m_shapeWidth);
			break;
		case PMDRigidBodyShape::Box:
			m_shape = std::make_unique<btBoxShape>(btVector3(
				pmdRigidBody.m_shapeWidth,
				pmdRigidBody.m_shapeHeight,
				pmdRigidBody.m_shapeDepth
			));
			break;
		case PMDRigidBodyShape::Capsule:
			m_shape = std::make_unique<btCapsuleShape>(
				pmdRigidBody.m_shapeWidth,
				pmdRigidBody.m_shapeHeight
				);
			break;
		default:
			break;
		}
		if (m_shape == nullptr)
		{
			return false;
		}

		btScalar mass(0.0f);
		btVector3 localInteria(0, 0, 0);
		if (pmdRigidBody.m_rigidBodyType != PMDRigidBodyOperation::Static)
		{
			mass = pmdRigidBody.m_rigidBodyWeight;
		}
		if (mass != 0)
		{
			m_shape->calculateLocalInertia(mass, localInteria);
		}

		auto rx = glm::rotate(glm::mat4(), pmdRigidBody.m_rot.x, glm::vec3(1, 0, 0));
		auto ry = glm::rotate(glm::mat4(), pmdRigidBody.m_rot.y, glm::vec3(0, 1, 0));
		auto rz = glm::rotate(glm::mat4(), pmdRigidBody.m_rot.z, glm::vec3(0, 0, 1));
		glm::mat4 rotMat = ry * rx * rz;
		glm::mat4 translateMat = glm::translate(glm::mat4(), pmdRigidBody.m_pos);

		glm::mat4 rbMat = translateMat * rotMat;
		if (node != nullptr)
		{
			glm::mat4 global = node->GetGlobalTransform();
			rbMat = InvZ(global) * rbMat;
		}
		else
		{
			MMDNode* root = model->GetNodeManager()->GetMMDNode(0);
			glm::mat4 global = root->GetGlobalTransform();
			rbMat = InvZ(global) * rbMat;
		}
		rbMat = InvZ(rbMat);

		if (node != nullptr)
		{
			m_offsetMat = glm::inverse(node->GetGlobalTransform()) * rbMat;
			m_invOffsetMat = glm::inverse(m_offsetMat);
		}
		else
		{
			MMDNode* root = model->GetNodeManager()->GetMMDNode(0);
			m_offsetMat = glm::inverse(root->GetGlobalTransform()) * rbMat;
			m_invOffsetMat = glm::inverse(m_offsetMat);
		}

		btMotionState* motionState = nullptr;
		MMDNode* kinematicNode = nullptr;
		bool overrideNode = true;
		if (node != nullptr)
		{
			kinematicNode = node;
		}
		else
		{
			kinematicNode = model->GetNodeManager()->GetMMDNode(0);
			overrideNode = false;
		}
		if (pmdRigidBody.m_rigidBodyType == PMDRigidBodyOperation::Static)
		{
			m_kinematicMotionState = std::make_unique<KinematicMotionState>(kinematicNode, m_offsetMat);
			motionState = m_kinematicMotionState.get();
		}
		else if (pmdRigidBody.m_rigidBodyType == PMDRigidBodyOperation::Dynamic)
		{
			m_activeMotionState = std::make_unique<DynamicMotionState>(kinematicNode, m_offsetMat, overrideNode);
			m_kinematicMotionState = std::make_unique<KinematicMotionState>(kinematicNode, m_offsetMat);
			motionState = m_activeMotionState.get();
		}
		else if (pmdRigidBody.m_rigidBodyType == PMDRigidBodyOperation::DynamicAdjustBone)
		{
			m_activeMotionState = std::make_unique<DynamicAndBoneMergeMotionState>(kinematicNode, m_offsetMat, overrideNode);
			m_kinematicMotionState = std::make_unique<KinematicMotionState>(kinematicNode, m_offsetMat);
			motionState = m_activeMotionState.get();
		}

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, m_shape.get(), localInteria);
		rbInfo.m_linearDamping = pmdRigidBody.m_rigidBodyPosDimmer;
		rbInfo.m_angularDamping = pmdRigidBody.m_rigidBodyRotDimmer;
		rbInfo.m_restitution = pmdRigidBody.m_rigidBodyRecoil;
		rbInfo.m_friction = pmdRigidBody.m_rigidBodyFriction;
		rbInfo.m_additionalDamping = true;

		m_rigidBody = std::make_unique<btRigidBody>(rbInfo);
		m_rigidBody->setUserPointer(this);
		m_rigidBody->setSleepingThresholds(0.01f, glm::radians(0.1f));
		m_rigidBody->setActivationState(DISABLE_DEACTIVATION);
		if (pmdRigidBody.m_rigidBodyType == PMDRigidBodyOperation::Static)
		{
			m_rigidBody->setCollisionFlags(m_rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}

		m_rigidBodyType = (RigidBodyType)pmdRigidBody.m_rigidBodyType;
		m_group = pmdRigidBody.m_groupIndex;
		m_groupMask = pmdRigidBody.m_groupTarget;
		m_node = node;
		m_name = pmdRigidBody.m_rigidBodyName.ToUtf8String();

		return true;
	}

	bool MMDRigidBody::Create(const PMXRigidbody & pmxRigidBody, MMDModel* model, MMDNode * node)
	{
		Destroy();

		switch (pmxRigidBody.m_shape)
		{
		case PMXRigidbody::Shape::Sphere:
			m_shape = std::make_unique<btSphereShape>(pmxRigidBody.m_shapeSize.x);
			break;
		case PMXRigidbody::Shape::Box:
			m_shape = std::make_unique<btBoxShape>(btVector3(
				pmxRigidBody.m_shapeSize.x,
				pmxRigidBody.m_shapeSize.y,
				pmxRigidBody.m_shapeSize.z
			));
			break;
		case PMXRigidbody::Shape::Capsule:
			m_shape = std::make_unique<btCapsuleShape>(
				pmxRigidBody.m_shapeSize.x,
				pmxRigidBody.m_shapeSize.y
				);
			break;
		default:
			break;
		}
		if (m_shape == nullptr)
		{
			return false;
		}

		btScalar mass(0.0f);
		btVector3 localInteria(0, 0, 0);
		if (pmxRigidBody.m_op != PMXRigidbody::Operation::Static)
		{
			mass = pmxRigidBody.m_mass;
		}
		if (mass != 0)
		{
			m_shape->calculateLocalInertia(mass, localInteria);
		}

		auto rx = glm::rotate(glm::mat4(), pmxRigidBody.m_rotate.x, glm::vec3(1, 0, 0));
		auto ry = glm::rotate(glm::mat4(), pmxRigidBody.m_rotate.y, glm::vec3(0, 1, 0));
		auto rz = glm::rotate(glm::mat4(), pmxRigidBody.m_rotate.z, glm::vec3(0, 0, 1));
		glm::mat4 rotMat = ry * rx * rz;
		glm::mat4 translateMat = glm::translate(glm::mat4(), pmxRigidBody.m_translate);

		glm::mat4 rbMat = InvZ(translateMat * rotMat);

		MMDNode* kinematicNode = nullptr;
		bool overrideNode = true;
		if (node != nullptr)
		{
			m_offsetMat = glm::inverse(node->GetGlobalTransform()) * rbMat;
			m_invOffsetMat = glm::inverse(m_offsetMat);
			kinematicNode = node;
		}
		else
		{
			MMDNode* root = model->GetNodeManager()->GetMMDNode(0);
			m_offsetMat = glm::inverse(root->GetGlobalTransform()) * rbMat;
			m_invOffsetMat = glm::inverse(m_offsetMat);
			kinematicNode = model->GetNodeManager()->GetMMDNode(0);
			overrideNode = false;
		}

		btMotionState* MMDMotionState = nullptr;
		if (pmxRigidBody.m_op == PMXRigidbody::Operation::Static)
		{
			m_kinematicMotionState = std::make_unique<KinematicMotionState>(kinematicNode, m_offsetMat);
			MMDMotionState = m_kinematicMotionState.get();
		}
		else
		{
			if (node != nullptr)
			{
				if (pmxRigidBody.m_op == PMXRigidbody::Operation::Dynamic)
				{
					m_activeMotionState = std::make_unique<DynamicMotionState>(kinematicNode, m_offsetMat);
					m_kinematicMotionState = std::make_unique<KinematicMotionState>(kinematicNode, m_offsetMat);
					MMDMotionState = m_activeMotionState.get();
				}
				else if (pmxRigidBody.m_op == PMXRigidbody::Operation::DynamicAndBoneMerge)
				{
					m_activeMotionState = std::make_unique<DynamicAndBoneMergeMotionState>(kinematicNode, m_offsetMat);
					m_kinematicMotionState = std::make_unique<KinematicMotionState>(kinematicNode, m_offsetMat);
					MMDMotionState = m_activeMotionState.get();
				}
			}
			else
			{
				m_activeMotionState = std::make_unique<DefaultMotionState>(m_offsetMat);
				m_kinematicMotionState = std::make_unique<KinematicMotionState>(kinematicNode, m_offsetMat);
				MMDMotionState = m_activeMotionState.get();
			}
		}

		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, MMDMotionState, m_shape.get(), localInteria);
		rbInfo.m_linearDamping = pmxRigidBody.m_translateDimmer;
		rbInfo.m_angularDamping = pmxRigidBody.m_rotateDimmer;
		rbInfo.m_restitution = pmxRigidBody.m_repulsion;
		rbInfo.m_friction = pmxRigidBody.m_friction;
		rbInfo.m_additionalDamping = true;

		m_rigidBody = std::make_unique<btRigidBody>(rbInfo);
		m_rigidBody->setUserPointer(this);
		m_rigidBody->setSleepingThresholds(0.01f, glm::radians(0.1f));
		m_rigidBody->setActivationState(DISABLE_DEACTIVATION);
		if (pmxRigidBody.m_op == PMXRigidbody::Operation::Static)
		{
			m_rigidBody->setCollisionFlags(m_rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
		}

		m_rigidBodyType = (RigidBodyType)pmxRigidBody.m_op;
		m_group = pmxRigidBody.m_group;
		m_groupMask = pmxRigidBody.m_collisionGroup;
		m_node = node;
		m_name = pmxRigidBody.m_name;

		return true;
	}

	void MMDRigidBody::Destroy()
	{
		m_shape = nullptr;
	}

	btRigidBody * MMDRigidBody::GetRigidBody() const
	{
		return m_rigidBody.get();
	}

	uint16_t MMDRigidBody::GetGroup() const
	{
		return m_group;
	}

	uint16_t MMDRigidBody::GetGroupMask() const
	{
		return m_groupMask;
	}

	void MMDRigidBody::SetActivation(bool activation)
	{
		if (m_rigidBodyType != RigidBodyType::Kinematic)
		{
			if (activation)
			{
				m_rigidBody->setCollisionFlags(m_rigidBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
				m_rigidBody->setMotionState(m_activeMotionState.get());
			}
			else
			{
				m_rigidBody->setCollisionFlags(m_rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				m_rigidBody->setMotionState(m_kinematicMotionState.get());
			}
		}
		else
		{
			m_rigidBody->setMotionState(m_kinematicMotionState.get());
		}
	}

	void MMDRigidBody::ResetTransform()
	{
		if (m_activeMotionState != nullptr)
		{
			m_activeMotionState->Reset();
		}
	}

	void MMDRigidBody::Reset(MMDPhysics* physics)
	{
		auto cache = physics->GetDynamicsWorld()->getPairCache();
		if (cache != nullptr)
		{
			auto dispatcher = physics->GetDynamicsWorld()->getDispatcher();
			cache->cleanProxyFromPairs(m_rigidBody->getBroadphaseHandle(), dispatcher);
		}
		m_rigidBody->setAngularVelocity(btVector3(0, 0, 0));
		m_rigidBody->setLinearVelocity(btVector3(0, 0, 0));
		m_rigidBody->clearForces();
	}

	void MMDRigidBody::ReflectGlobalTransform()
	{
		if (m_activeMotionState != nullptr)
		{
			m_activeMotionState->ReflectGlobalTransform();
		}
		if (m_kinematicMotionState != nullptr)
		{
			m_kinematicMotionState->ReflectGlobalTransform();
		}
	}

	void MMDRigidBody::CalcLocalTransform()
	{
		if (m_node != nullptr)
		{
			auto parent = m_node->GetParent();
			if (parent != nullptr)
			{
				auto local = glm::inverse(parent->GetGlobalTransform()) * m_node->GetGlobalTransform();
				m_node->SetLocalTransform(local);
			}
			else
			{
				m_node->SetLocalTransform(m_node->GetGlobalTransform());
			}
		}
	}

	glm::mat4 MMDRigidBody::GetTransform()
	{
		btTransform transform = m_rigidBody->getCenterOfMassTransform();
		glm::mat4 mat;
		transform.getOpenGLMatrix(&mat[0][0]);
		return InvZ(mat);
	}


	//*******************
	// MMDJoint
	//*******************
	MMDJoint::MMDJoint()
	{
	}

	MMDJoint::~MMDJoint()
	{
	}

	bool MMDJoint::CreateJoint(const PMDJointExt& pmdJoint, MMDRigidBody* rigidBodyA, MMDRigidBody* rigidBodyB)
	{
		Destroy();

		btMatrix3x3 rotMat;
		rotMat.setEulerZYX(pmdJoint.m_jointRot.x, pmdJoint.m_jointRot.y, pmdJoint.m_jointRot.z);

		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(
			pmdJoint.m_jointPos.x,
			pmdJoint.m_jointPos.y,
			pmdJoint.m_jointPos.z
		));
		transform.setBasis(rotMat);

		btTransform invA = rigidBodyA->GetRigidBody()->getWorldTransform().inverse();
		btTransform invB = rigidBodyB->GetRigidBody()->getWorldTransform().inverse();
		invA = invA * transform;
		invB = invB * transform;

		auto constraint = std::make_unique<btGeneric6DofSpringConstraint>(
			*rigidBodyA->GetRigidBody(),
			*rigidBodyB->GetRigidBody(),
			invA,
			invB,
			true);
		constraint->setLinearLowerLimit(btVector3(
			pmdJoint.m_constrainPos1.x,
			pmdJoint.m_constrainPos1.y,
			pmdJoint.m_constrainPos1.z
		));
		constraint->setLinearUpperLimit(btVector3(
			pmdJoint.m_constrainPos2.x,
			pmdJoint.m_constrainPos2.y,
			pmdJoint.m_constrainPos2.z
		));

		constraint->setAngularLowerLimit(btVector3(
			pmdJoint.m_constrainRot1.x,
			pmdJoint.m_constrainRot1.y,
			pmdJoint.m_constrainRot1.z
		));
		constraint->setAngularUpperLimit(btVector3(
			pmdJoint.m_constrainRot2.x,
			pmdJoint.m_constrainRot2.y,
			pmdJoint.m_constrainRot2.z
		));

		if (pmdJoint.m_springPos.x != 0)
		{
			constraint->enableSpring(0, true);
			constraint->setStiffness(0, pmdJoint.m_springPos.x);
		}
		if (pmdJoint.m_springPos.y != 0)
		{
			constraint->enableSpring(1, true);
			constraint->setStiffness(1, pmdJoint.m_springPos.y);
		}
		if (pmdJoint.m_springPos.z != 0)
		{
			constraint->enableSpring(2, true);
			constraint->setStiffness(2, -pmdJoint.m_springPos.z);
		}
		if (pmdJoint.m_springRot.x != 0)
		{
			constraint->enableSpring(3, true);
			constraint->setStiffness(3, pmdJoint.m_springRot.x);
		}
		if (pmdJoint.m_springRot.y != 0)
		{
			constraint->enableSpring(4, true);
			constraint->setStiffness(4, pmdJoint.m_springRot.y);
		}
		if (pmdJoint.m_springRot.z != 0)
		{
			constraint->enableSpring(5, true);
			constraint->setStiffness(5, pmdJoint.m_springRot.z);
		}

		m_constraint = std::move(constraint);

		return true;
	}

	bool MMDJoint::CreateJoint(const PMXJoint& pmxJoint, MMDRigidBody* rigidBodyA, MMDRigidBody* rigidBodyB)
	{
		Destroy();

		btMatrix3x3 rotMat;
		rotMat.setEulerZYX(pmxJoint.m_rotate.x, pmxJoint.m_rotate.y, pmxJoint.m_rotate.z);

		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(
			pmxJoint.m_translate.x,
			pmxJoint.m_translate.y,
			pmxJoint.m_translate.z
		));
		transform.setBasis(rotMat);

		btTransform invA = rigidBodyA->GetRigidBody()->getWorldTransform().inverse();
		btTransform invB = rigidBodyB->GetRigidBody()->getWorldTransform().inverse();
		invA = invA * transform;
		invB = invB * transform;

		auto constraint = std::make_unique<btGeneric6DofSpringConstraint>(
			*rigidBodyA->GetRigidBody(),
			*rigidBodyB->GetRigidBody(),
			invA,
			invB,
			true);
		constraint->setLinearLowerLimit(btVector3(
			pmxJoint.m_translateLowerLimit.x,
			pmxJoint.m_translateLowerLimit.y,
			pmxJoint.m_translateLowerLimit.z
		));
		constraint->setLinearUpperLimit(btVector3(
			pmxJoint.m_translateUpperLimit.x,
			pmxJoint.m_translateUpperLimit.y,
			pmxJoint.m_translateUpperLimit.z
		));

		constraint->setAngularLowerLimit(btVector3(
			pmxJoint.m_rotateLowerLimit.x,
			pmxJoint.m_rotateLowerLimit.y,
			pmxJoint.m_rotateLowerLimit.z
		));
		constraint->setAngularUpperLimit(btVector3(
			pmxJoint.m_rotateUpperLimit.x,
			pmxJoint.m_rotateUpperLimit.y,
			pmxJoint.m_rotateUpperLimit.z
		));

		if (pmxJoint.m_springTranslateFactor.x != 0)
		{
			constraint->enableSpring(0, true);
			constraint->setStiffness(0, pmxJoint.m_springTranslateFactor.x);
		}
		if (pmxJoint.m_springTranslateFactor.y != 0)
		{
			constraint->enableSpring(1, true);
			constraint->setStiffness(1, pmxJoint.m_springTranslateFactor.y);
		}
		if (pmxJoint.m_springTranslateFactor.z != 0)
		{
			constraint->enableSpring(2, true);
			constraint->setStiffness(2, pmxJoint.m_springTranslateFactor.z);
		}
		if (pmxJoint.m_springRotateFactor.x != 0)
		{
			constraint->enableSpring(3, true);
			constraint->setStiffness(3, pmxJoint.m_springRotateFactor.x);
		}
		if (pmxJoint.m_springRotateFactor.y != 0)
		{
			constraint->enableSpring(4, true);
			constraint->setStiffness(4, pmxJoint.m_springRotateFactor.y);
		}
		if (pmxJoint.m_springRotateFactor.z != 0)
		{
			constraint->enableSpring(5, true);
			constraint->setStiffness(5, pmxJoint.m_springRotateFactor.z);
		}

		m_constraint = std::move(constraint);

		return true;
	}

	void MMDJoint::Destroy()
	{
		m_constraint = nullptr;
	}

	btTypedConstraint * MMDJoint::GetConstraint() const
	{
		return m_constraint.get();
	}

}
