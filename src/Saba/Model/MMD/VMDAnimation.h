//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_VMDANIMATION_H_
#define SABA_MODEL_MMD_VMDANIMATION_H_

#include "MMDModel.h"
#include "MMDNode.h"
#include "VMDFile.h"
#include "MMDIkSolver.h"

#include <vector>
#include <algorithm>
#include <memory>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace saba
{
	struct VMDBezier
	{
		float EvalX(float t) const;
		float EvalY(float t) const;
		glm::vec2 Eval(float t) const;

		float FindBezierX(float time) const;

		glm::vec2	m_cp[4];
	};

	struct VMDNodeAnimationKey
	{
		void Set(const VMDMotion& motion);

		int32_t		m_time;
		glm::vec3	m_translate;
		glm::quat	m_rotate;

		VMDBezier	m_txBezier;
		VMDBezier	m_tyBezier;
		VMDBezier	m_tzBezier;
		VMDBezier	m_rotBezier;
	};

	struct VMDMorphAnimationKey
	{
		int32_t	m_time;
		float	m_weight;
	};

	struct VMDIKAnimationKey
	{
		int32_t	m_time;
		bool	m_enable;
	};

	class VMDNodeController
	{
	public:
		using KeyType = VMDNodeAnimationKey;

		VMDNodeController();

		void SetNode(MMDNode* node);
		void Evaluate(float t, float weight = 1.0f);
		
		void AddKey(const KeyType& key)
		{
			m_keys.push_back(key);
		}
		void SortKeys();

		MMDNode* GetNode() const { return m_node; }

	private:
		MMDNode*	m_node;
		std::vector<KeyType>	m_keys;
	};

	class VMDMorphController
	{
	public:
		using KeyType = VMDMorphAnimationKey;

		VMDMorphController();

		void SetBlendKeyShape(MMDMorph* morph);
		void Evaluate(float t, float weight = 1.0f);

		void AddKey(const KeyType& key)
		{
			m_keys.push_back(key);
		}
		void SortKeys();

		MMDMorph* GetMorph() const { return m_morph; }

	private:
		MMDMorph*				m_morph;
		std::vector<KeyType>	m_keys;
	};

	class VMDIKController
	{
	public:
		using KeyType = VMDIKAnimationKey;

		VMDIKController();

		void SetIKSolver(MMDIkSolver* ikSolver);
		void Evaluate(float t, float weight = 1.0f);

		void AddKey(const KeyType& key)
		{
			m_keys.push_back(key);
		}
		void SortKeys();

		MMDIkSolver* GetIkSolver() const { return m_ikSolver; }

	private:
		MMDIkSolver*					m_ikSolver;
		std::vector<VMDIKAnimationKey>	m_keys;
	};

	class VMDAnimation
	{
	public:
		VMDAnimation();

		bool Create(std::shared_ptr<MMDModel> model);
		bool Add(const VMDFile& vmd);
		void Destroy();

		void Evaluate(float t, float weight = 1.0f);

		// Physics を同期させる
		void SyncPhysics(float t, int frameCount = 30);

	private:
		using NodeControllerPtr = std::unique_ptr<VMDNodeController>;
		using IKControllerPtr = std::unique_ptr<VMDIKController>;
		using MorphControllerPtr = std::unique_ptr<VMDMorphController>;

		std::shared_ptr<MMDModel>			m_model;
		std::vector<NodeControllerPtr>		m_nodeControllers;
		std::vector<IKControllerPtr>		m_ikControllers;
		std::vector<MorphControllerPtr>		m_morphControllers;
	};

}

#endif // !SABA_MODEL_MMD_VMDANIMATION_H_
