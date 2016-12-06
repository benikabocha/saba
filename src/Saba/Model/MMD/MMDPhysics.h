#ifndef SABA_MODEL_MMDMODEL_MMDPHYSICS_H_
#define SABA_MODEL_MMDMODEL_MMDPHYSICS_H_

#include "PMDFile.h"
#include "PMXFile.h"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <memory>
#include <cinttypes>

namespace saba
{
	class MMDPhysics;
	class MMDModel;
	class MMDNode;

	class MMDMotionState : public btMotionState
	{
	public:
		virtual void Reset() = 0;
		virtual void BeginUpdate() = 0;
		virtual void EndUpdate() = 0;
	};

	class MMDRigidBody
	{
	public:
		MMDRigidBody() = default;
		MMDRigidBody(const MMDRigidBody& rhs) = delete;
		MMDRigidBody& operator = (const MMDRigidBody& rhs) = delete;

		bool Create(const PMDRigidBodyExt& pmdRigidBody, MMDModel* model, MMDNode* node);
		bool Create(const PMXRigidbody& pmxRigidBody, MMDModel* model, MMDNode* node);
		void Destroy();

		btRigidBody* GetRigidBody() const;
		uint16_t GetGroup() const;
		uint16_t GetGroupMask() const;

		void SetActivation(bool activation);
		void Reset(MMDPhysics* physics);

		void BeginUpdate();
		void EndUpdate();

		glm::mat4 GetTransform();

	private:
		enum class RigidBodyType
		{
			Kinematic,
			Dynamic,
			Aligned,
		};

	private:
		std::unique_ptr<btCollisionShape>	m_shape;
		std::unique_ptr<MMDMotionState>		m_activeMotionState;
		std::unique_ptr<MMDMotionState>		m_kinematicMotionState;
		std::unique_ptr<btRigidBody>		m_rigidBody;

		RigidBodyType	m_rigidBodyType;
		uint16_t		m_group;
		uint16_t		m_groupMask;

		MMDNode*	m_node;
		glm::mat4	m_offsetMat;
		glm::mat4	m_invOffsetMat;

		std::string					m_name;
	};

	class MMDJoint
	{
	public:
		MMDJoint() = default;
		MMDJoint(const MMDJoint& rhs) = delete;
		MMDJoint& operator = (const MMDJoint& rhs) = delete;

		bool CreateJoint(const PMDJointExt& pmdJoint, MMDRigidBody* rigidBodyA, MMDRigidBody* rigidBodyB);
		bool CreateJoint(const PMXJoint& pmxJoint, MMDRigidBody* rigidBodyA, MMDRigidBody* rigidBodyB);
		void Destroy();

		btTypedConstraint* GetConstraint() const;

	private:
		std::unique_ptr<btTypedConstraint>	m_constraint;
	};

	class MMDPhysics
	{
	public:
		MMDPhysics();
		~MMDPhysics();

		MMDPhysics(const MMDPhysics& rhs) = delete;
		MMDPhysics& operator = (const MMDPhysics& rhs) = delete;

		bool Create();
		void Destroy();

		void Update(float time);

		void AddRigidBody(MMDRigidBody* mmdRB);
		void RemoveRigidBody(MMDRigidBody* mmdRB);
		void AddJoint(MMDJoint* mmdJoint);
		void RemoveJoint(MMDJoint* mmdJoint);

		btDiscreteDynamicsWorld* GetDynamicsWorld() const;

	private:
		std::unique_ptr<btBroadphaseInterface>				m_broadphase;
		std::unique_ptr<btDefaultCollisionConfiguration>	m_collisionConfig;
		std::unique_ptr<btCollisionDispatcher>				m_dispatcher;
		std::unique_ptr<btSequentialImpulseConstraintSolver>	m_solver;
		std::unique_ptr<btDiscreteDynamicsWorld>			m_world;
		std::unique_ptr<btCollisionShape>					m_groundShape;
		std::unique_ptr<btMotionState>						m_groundMS;
		std::unique_ptr<btRigidBody>						m_groundRB;
	};

}
#endif // SABA_MODEL_MMDMODEL_MMDPHYSICS_H_

