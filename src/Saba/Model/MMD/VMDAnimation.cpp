//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "VMDAnimation.h"
#include "VMDAnimationCommon.h"

#include <Saba/Base/Log.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <glm/gtc/matrix_transform.hpp>

namespace saba
{
	namespace
	{
		void SetVMDBezier(VMDBezier& bezier, const unsigned char* cp)
		{
			int x0 = cp[0];
			int y0 = cp[4];
			int x1 = cp[8];
			int y1 = cp[12];

			bezier.m_cp1 = glm::vec2((float)x0 / 127.0f, (float)y0 / 127.0f);
			bezier.m_cp2 = glm::vec2((float)x1 / 127.0f, (float)y1 / 127.0f);
		}

		glm::mat3 InvZ(const glm::mat3& m)
		{
			const glm::mat3 invZ = glm::scale(glm::mat4(1), glm::vec3(1, 1, -1));
			return invZ * m * invZ;
		}
	} // namespace

	float VMDBezier::EvalX(float t) const
	{
		const float t2 = t * t;
		const float t3 = t2 * t;
		const float it = 1.0f - t;
		const float it2 = it * it;
		const float it3 = it2 * it;
		const float x[4] = {
			0,
			m_cp1.x,
			m_cp2.x,
			1,
		};

		return t3 * x[3] + 3 * t2 * it * x[2] + 3 * t * it2 * x[1] + it3 * x[0];
	}

	float VMDBezier::EvalY(float t) const
	{
		const float t2 = t * t;
		const float t3 = t2 * t;
		const float it = 1.0f - t;
		const float it2 = it * it;
		const float it3 = it2 * it;
		const float y[4] = {
			0,
			m_cp1.y,
			m_cp2.y,
			1,
		};

		return t3 * y[3] + 3 * t2 * it * y[2] + 3 * t * it2 * y[1] + it3 * y[0];
	}

	glm::vec2 VMDBezier::Eval(float t) const
	{
		return glm::vec2(EvalX(t), EvalY(t));
	}

	float VMDBezier::FindBezierX(float time) const
	{
		const float e = 0.00001f;
		float start = 0.0f;
		float stop = 1.0f;
		float t = 0.5f;
		float x = EvalX(t);
		while (std::abs(time - x) > e)
		{
			if (time < x)
			{
				stop = t;
			}
			else
			{
				start = t;
			}
			t = (stop + start) * 0.5f;
			x = EvalX(t);
		}

		return t;
	}

	VMDNodeController::VMDNodeController()
		: m_node(nullptr)
		, m_startKeyIndex(0)
	{
	}

	void VMDNodeController::SetNode(MMDNode * node)
	{
		m_node = node;
	}

	void VMDNodeController::Evaluate(float t, float weight)
	{
		SABA_ASSERT(m_node != nullptr);
		if (m_node == nullptr)
		{
			return;
		}
		if (m_keys.empty())
		{
			m_node->SetAnimationTranslate(glm::vec3(0));
			m_node->SetAnimationRotate(glm::quat(1, 0, 0, 0));
			return;
		}

		auto boundIt = FindBoundKey(m_keys, int32_t(t), m_startKeyIndex);
		glm::vec3 vt;
		glm::quat q;
		if (boundIt == std::end(m_keys))
		{
			vt = m_keys[m_keys.size() - 1].m_translate;
			q = m_keys[m_keys.size() - 1].m_rotate;
		}
		else
		{
			vt = (*boundIt).m_translate;
			q = (*boundIt).m_rotate;
			if (boundIt != std::begin(m_keys))
			{
				const auto& key0 = *(boundIt - 1);
				const auto& key1 = *boundIt;

				float timeRange = float(key1.m_time - key0.m_time);
				float time = (t - float(key0.m_time)) / timeRange;
				float tx_x = key0.m_txBezier.FindBezierX(time);
				float ty_x = key0.m_tyBezier.FindBezierX(time);
				float tz_x = key0.m_tzBezier.FindBezierX(time);
				float rot_x = key0.m_rotBezier.FindBezierX(time);
				float tx_y = key0.m_txBezier.EvalY(tx_x);
				float ty_y = key0.m_tyBezier.EvalY(ty_x);
				float tz_y = key0.m_tzBezier.EvalY(tz_x);
				float rot_y = key0.m_rotBezier.EvalY(rot_x);

				vt = glm::mix(key0.m_translate, key1.m_translate, glm::vec3(tx_y, ty_y, tz_y));
				q = glm::slerp(key0.m_rotate, key1.m_rotate, rot_y);

				m_startKeyIndex = std::distance(m_keys.cbegin(), boundIt);
			}
		}

		if (weight == 1.0f)
		{
			m_node->SetAnimationRotate(q);
			m_node->SetAnimationTranslate(vt);
		}
		else
		{
			auto baseQ = m_node->GetBaseAnimationRotate();
			auto baseT = m_node->GetBaseAnimationTranslate();
			m_node->SetAnimationRotate(glm::slerp(baseQ, q, weight));
			m_node->SetAnimationTranslate(glm::mix(baseT, vt, weight));
		}
	}

	void VMDNodeController::SortKeys()
	{
		std::sort(
			std::begin(m_keys),
			std::end(m_keys),
			[](const KeyType& a, const KeyType& b) { return a.m_time < b.m_time; }
		);
	}

	VMDAnimation::VMDAnimation()
	{
	}

	bool VMDAnimation::Create(std::shared_ptr<MMDModel> model)
	{
		m_model = model;
		return true;
	}

	bool VMDAnimation::Add(const VMDFile & vmd)
	{
		// Node Controller
		std::map<std::string, NodeControllerPtr> nodeCtrlMap;
		for (auto& nodeCtrl : m_nodeControllers)
		{
			std::string name = nodeCtrl->GetNode()->GetName();
			nodeCtrlMap.emplace(std::make_pair(name, std::move(nodeCtrl)));
		}
		m_nodeControllers.clear();
		for (const auto& motion : vmd.m_motions)
		{
			std::string nodeName = motion.m_boneName.ToUtf8String();
			auto findIt = nodeCtrlMap.find(nodeName);
			VMDNodeController* nodeCtrl = nullptr;
			if (findIt == std::end(nodeCtrlMap))
			{
				auto node = m_model->GetNodeManager()->GetMMDNode(nodeName);
				if (node != nullptr)
				{
					auto val = std::make_pair(
						nodeName,
						std::make_unique<VMDNodeController>()
					);
					nodeCtrl = val.second.get();
					nodeCtrl->SetNode(node);
					nodeCtrlMap.emplace(std::move(val));
				}
			}
			else
			{
				nodeCtrl = (*findIt).second.get();
			}

			if (nodeCtrl != nullptr)
			{
				VMDNodeAnimationKey key;
				key.Set(motion);
				nodeCtrl->AddKey(key);
			}
		}
		m_nodeControllers.reserve(nodeCtrlMap.size());
		for (auto& pair : nodeCtrlMap)
		{
			pair.second->SortKeys();
			m_nodeControllers.emplace_back(std::move(pair.second));
		}
		nodeCtrlMap.clear();

		// IK Contoroller
		std::map<std::string, IKControllerPtr> ikCtrlMap;
		for (auto& ikCtrl : m_ikControllers)
		{
			std::string name = ikCtrl->GetIkSolver()->GetName();
			ikCtrlMap.emplace(std::make_pair(name, std::move(ikCtrl)));
		}
		m_ikControllers.clear();
		for (const auto& ik : vmd.m_iks)
		{
			for (const auto& ikInfo : ik.m_ikInfos)
			{
				std::string ikName = ikInfo.m_name.ToUtf8String();
				auto findIt = ikCtrlMap.find(ikName);
				VMDIKController* ikCtrl = nullptr;
				if (findIt == std::end(ikCtrlMap))
				{
					auto* ikSolver = m_model->GetIKManager()->GetMMDIKSolver(ikName);
					if (ikSolver != nullptr)
					{
						auto val = std::make_pair(
							ikName,
							std::make_unique<VMDIKController>()
						);
						ikCtrl = val.second.get();
						ikCtrl->SetIKSolver(ikSolver);
						ikCtrlMap.emplace(std::move(val));
					}
				}
				else
				{
					ikCtrl = (*findIt).second.get();
				}

				if (ikCtrl != nullptr)
				{
					VMDIKAnimationKey key;
					key.m_time = int32_t(ik.m_frame);
					key.m_enable = ikInfo.m_enable != 0;
					ikCtrl->AddKey(key);
				}
			}
		}
		m_ikControllers.reserve(ikCtrlMap.size());
		for (auto& pair : ikCtrlMap)
		{
			pair.second->SortKeys();
			m_ikControllers.emplace_back(std::move(pair.second));
		}
		ikCtrlMap.clear();

		// Morph Controller
		std::map<std::string, MorphControllerPtr> morphCtrlMap;
		for (auto& morphCtrl : m_morphControllers)
		{
			std::string name = morphCtrl->GetMorph()->GetName();
			morphCtrlMap.emplace(std::make_pair(name, std::move(morphCtrl)));
		}
		m_morphControllers.clear();
		for (const auto& morph : vmd.m_morphs)
		{
			std::string morphName = morph.m_blendShapeName.ToUtf8String();
			auto findIt = morphCtrlMap.find(morphName);
			VMDMorphController* morphCtrl = nullptr;
			if (findIt == std::end(morphCtrlMap))
			{
				auto* mmdMorph = m_model->GetMorphManager()->GetMorph(morphName);
				if (mmdMorph != nullptr)
				{
					auto val = std::make_pair(
						morphName,
						std::make_unique<VMDMorphController>()
					);
					morphCtrl = val.second.get();
					morphCtrl->SetBlendKeyShape(mmdMorph);
					morphCtrlMap.emplace(std::move(val));
				}
			}
			else
			{
				morphCtrl = (*findIt).second.get();
			}

			if (morphCtrl != nullptr)
			{
				VMDMorphAnimationKey key;
				key.m_time = int32_t(morph.m_frame);
				key.m_weight = morph.m_weight;
				morphCtrl->AddKey(key);
			}
		}
		m_morphControllers.reserve(morphCtrlMap.size());
		for (auto& pair : morphCtrlMap)
		{
			pair.second->SortKeys();
			m_morphControllers.emplace_back(std::move(pair.second));
		}
		morphCtrlMap.clear();

		m_maxKeyTime = CalculateMaxKeyTime();

		return true;
	}

	void VMDAnimation::Destroy()
	{
		m_model.reset();
		m_nodeControllers.clear();
		m_ikControllers.clear();
		m_morphControllers.clear();
		m_maxKeyTime = 0;
	}

	void VMDAnimation::Evaluate(float t, float weight)
	{
		for (auto& nodeCtrl : m_nodeControllers)
		{
			nodeCtrl->Evaluate(t, weight);
		}

		for (auto& ikCtrl : m_ikControllers)
		{
			ikCtrl->Evaluate(t, weight);
		}

		for (auto& morphCtrl : m_morphControllers)
		{
			morphCtrl->Evaluate(t, weight);
		}
	}

	void VMDAnimation::SyncPhysics(float t, int frameCount)
	{
		/*
		すぐにアニメーションを反映すると、Physics が破たんする場合がある。
		例：足がスカートを突き破る等
		アニメーションを反映する際、初期状態から数フレームかけて、
		目的のポーズへ遷移させる。
		*/
		m_model->SaveBaseAnimation();

		// Physicsを反映する
		for (int i = 0; i < frameCount; i++)
		{
			m_model->BeginAnimation();

			Evaluate((float)t, float(1 + i) / float(frameCount));

			m_model->UpdateMorphAnimation();

			m_model->UpdateNodeAnimation(false);

			m_model->UpdatePhysicsAnimation(1.0f / 30.0f);

			m_model->UpdateNodeAnimation(true);

			m_model->EndAnimation();
		}
	}

	int32_t VMDAnimation::CalculateMaxKeyTime() const
	{
		int32_t maxTime = 0;
		for (const auto& nodeController : m_nodeControllers)
		{
			const auto& keys = nodeController->GetKeys();
			if (!keys.empty())
			{
				maxTime = std::max(maxTime, keys.rbegin()->m_time);
			}
		}

		for (const auto& ikController : m_ikControllers)
		{
			const auto& keys = ikController->GetKeys();
			if (!keys.empty())
			{
				maxTime = std::max(maxTime, keys.rbegin()->m_time);
			}
		}

		for (const auto& morphController : m_morphControllers)
		{
			const auto& keys = morphController->GetKeys();
			if (!keys.empty())
			{
				maxTime = std::max(maxTime, keys.rbegin()->m_time);
			}
		}

		return maxTime;
	}

	void VMDNodeAnimationKey::Set(const VMDMotion & motion)
	{
		m_time = int32_t(motion.m_frame);

		m_translate = motion.m_translate * glm::vec3(1, 1, -1);

		const glm::quat q = motion.m_quaternion;
		auto rot0 = glm::mat3_cast(q);
		auto rot1 = InvZ(rot0);
		m_rotate = glm::quat_cast(rot1);

		SetVMDBezier(m_txBezier, &motion.m_interpolation[0]);
		SetVMDBezier(m_tyBezier, &motion.m_interpolation[1]);
		SetVMDBezier(m_tzBezier, &motion.m_interpolation[2]);
		SetVMDBezier(m_rotBezier, &motion.m_interpolation[3]);
	}

	VMDIKController::VMDIKController()
		: m_ikSolver(nullptr)
		, m_startKeyIndex(0)
	{
	}

	void VMDIKController::SetIKSolver(MMDIkSolver * ikSolver)
	{
		m_ikSolver = ikSolver;
	}

	void VMDIKController::Evaluate(float t, float weight)
	{
		if (m_ikSolver == nullptr)
		{
			return;
		}
		if (m_keys.empty())
		{
			m_ikSolver->Enable(true);
			return;
		}

		auto boundIt = FindBoundKey(m_keys, int32_t(t), m_startKeyIndex);
		bool enable = true;
		if (boundIt == std::end(m_keys))
		{
			enable = m_keys.rbegin()->m_enable;
		}
		else
		{
			enable = m_keys.begin()->m_enable;
			if (boundIt != std::begin(m_keys))
			{
				const auto& key = *(boundIt - 1);
				enable = key.m_enable;

				m_startKeyIndex = std::distance(m_keys.cbegin(), boundIt);
			}
		}

		if (weight == 1.0f)
		{
			m_ikSolver->Enable(enable);
		}
		else
		{
			if (weight < 1.0f)
			{
				m_ikSolver->Enable(m_ikSolver->GetBaseAnimationEnabled());
			}
			else
			{
				m_ikSolver->Enable(enable);
			}
		}
	}

	void VMDIKController::SortKeys()
	{
		std::sort(
			std::begin(m_keys),
			std::end(m_keys),
			[](const KeyType& a, const KeyType& b) { return a.m_time < b.m_time; }
		);
	}

	VMDMorphController::VMDMorphController()
		: m_morph(nullptr)
		, m_startKeyIndex(0)
	{
	}

	void VMDMorphController::SetBlendKeyShape(MMDMorph* morph)
	{
		m_morph = morph;
	}

	void VMDMorphController::Evaluate(float t, float animWeight)
	{
		if (m_morph == nullptr)
		{
			return;
		}

		if (m_keys.empty())
		{
			return;
		}

		float weight;
		auto boundIt = FindBoundKey(m_keys, int32_t(t), m_startKeyIndex);
		if (boundIt == std::end(m_keys))
		{
			weight = m_keys.rbegin()->m_weight;
		}
		else
		{
			weight = (*boundIt).m_weight;
			if (boundIt != std::begin(m_keys))
			{
				VMDMorphAnimationKey key0 = *(boundIt - 1);
				VMDMorphAnimationKey key1 = *boundIt;

				float timeRange = float(key1.m_time - key0.m_time);
				float time = (t - float(key0.m_time)) / timeRange;
				weight = (key1.m_weight - key0.m_weight) * time + key0.m_weight;

				m_startKeyIndex = std::distance(m_keys.cbegin(), boundIt);
			}
		}

		if (animWeight == 1.0f)
		{
			m_morph->SetWeight(weight);
		}
		else
		{
			m_morph->SetWeight(glm::mix(m_morph->GetBaseAnimationWeight(), weight, animWeight));
		}
	}

	void VMDMorphController::SortKeys()
	{
		std::sort(
			std::begin(m_keys),
			std::end(m_keys),
			[](const KeyType& a, const KeyType& b) { return a.m_time < b.m_time; }
		);
	}
}
