//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "MMDIkSolver.h"

#include <algorithm>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>

namespace saba
{
	MMDIkSolver::MMDIkSolver()
		: m_ikNode(nullptr)
		, m_ikTarget(nullptr)
		, m_iterateCount(1)
		, m_limitAngle(glm::pi<float>() * 2.0f)
		, m_enable(true)
	{
	}

	void MMDIkSolver::AddIKChain(MMDNode * node, bool isKnee)
	{
		IKChain chain;
		chain.m_node = node;
		chain.m_enableAxisLimit = isKnee;
		if (isKnee)
		{
			chain.m_limitMin = glm::vec3(glm::radians(0.5f), 0, 0);
			chain.m_limitMax = glm::vec3(glm::radians(180.0f), 0, 0);
		}
		AddIKChain(std::move(chain));
	}

	void MMDIkSolver::AddIKChain(
		MMDNode * node,
		bool axisLimit,
		const glm::vec3 & limixMin,
		const glm::vec3 & limitMax
	)
	{
		IKChain chain;
		chain.m_node = node;
		chain.m_enableAxisLimit = axisLimit;
		chain.m_limitMin = limixMin;
		chain.m_limitMax = limitMax;
		AddIKChain(std::move(chain));
	}

	void MMDIkSolver::AddIKChain(MMDIkSolver::IKChain&& chain)
	{
		m_chains.emplace_back(chain);
	}

	void MMDIkSolver::Solve()
	{
		if (!m_enable)
		{
			return;
		}
		// Initialize IKChain
		for (auto& chain : m_chains)
		{
			chain.m_prevAngle = glm::vec3(0);
			chain.m_node->SetIKRotate(glm::quat(1, 0, 0, 0));
			chain.m_planeModeAngle = 0;

			chain.m_node->UpdateLocalTransform();
			chain.m_node->UpdateGlobalTransform();
		}

		float maxDist = std::numeric_limits<float>::max();
		for (uint32_t i = 0; i < m_iterateCount; i++)
		{
			SolveCore(i);

			auto targetPos = glm::vec3(m_ikTarget->GetGlobalTransform()[3]);
			auto ikPos = glm::vec3(m_ikNode->GetGlobalTransform()[3]);
			float dist = glm::length(targetPos - ikPos);
			if (dist < maxDist)
			{
				maxDist = dist;
				for (auto& chain : m_chains)
				{
					chain.m_saveIKRot = chain.m_node->GetIKRotate();
				}
			}
			else
			{
				for (auto& chain : m_chains)
				{
					chain.m_node->SetIKRotate(chain.m_saveIKRot);
					chain.m_node->UpdateLocalTransform();
					chain.m_node->UpdateGlobalTransform();
				}
				break;
			}
		}
	}

	namespace
	{
		float NormalizeAngle(float angle)
		{
			float ret = angle;
			while (ret >= glm::two_pi<float>())
			{
				ret -= glm::two_pi<float>();
			}
			while (ret < 0)
			{
				ret += glm::two_pi<float>();
			}

			return ret;
		}

		float DiffAngle(float a, float b)
		{
			float diff = NormalizeAngle(a) - NormalizeAngle(b);
			if (diff > glm::pi<float>())
			{
				return diff - glm::two_pi<float>();
			}
			else if (diff < -glm::pi<float>())
			{
				return diff + glm::two_pi<float>();
			}
			return diff;
		}

		float ClampAngle(float angle, float minAngle, float maxAngle)
		{
			if (minAngle == maxAngle)
			{
				return minAngle;
			}

			float ret = angle;
			while (ret < minAngle)
			{
				ret += glm::two_pi<float>();
			}
			if (ret < maxAngle)
			{
				return ret;
			}

			while (ret > maxAngle)
			{
				ret -= glm::two_pi<float>();
			}
			if (ret > minAngle)
			{
				return ret;
			}

			float minDiff = std::abs(DiffAngle(minAngle, ret));
			float maxDiff = std::abs(DiffAngle(maxAngle, ret));
			if (minDiff < maxDiff)
			{
				return minAngle;
			}
			else
			{
				return maxAngle;
			}
		}

		glm::vec3 Decompose(const glm::mat3& m, const glm::vec3& before)
		{
			glm::vec3 r;
			float sy = -m[0][2];
			const float e = 1.0e-6f;
			if ((1.0f - std::abs(sy)) < e)
			{
				r.y = std::asin(sy);
				// 180°に近いほうを探す
				float sx = std::sin(before.x);
				float sz = std::sin(before.z);
				if (std::abs(sx) < std::abs(sz))
				{
					// Xのほうが0または180
					float cx = std::cos(before.x);
					if (cx > 0)
					{
						r.x = 0;
						r.z = std::asin(-m[1][0]);
					}
					else
					{
						r.x = glm::pi<float>();
						r.z = std::asin(m[1][0]);
					}
				}
				else
				{
					float cz = std::cos(before.z);
					if (cz > 0)
					{
						r.z = 0;
						r.x = std::asin(-m[2][1]);
					}
					else
					{
						r.z = glm::pi<float>();
						r.x = std::asin(m[2][1]);
					}
				}
			}
			else
			{
				r.x = std::atan2(m[1][2], m[2][2]);
				r.y = std::asin(-m[0][2]);
				r.z = std::atan2(m[0][1], m[0][0]);
			}

			const float pi = glm::pi<float>();
			glm::vec3 tests[] =
			{
				{ r.x + pi, pi - r.y, r.z + pi },
				{ r.x + pi, pi - r.y, r.z - pi },
				{ r.x + pi, -pi - r.y, r.z + pi },
				{ r.x + pi, -pi - r.y, r.z - pi },
				{ r.x - pi, pi - r.y, r.z + pi },
				{ r.x - pi, pi - r.y, r.z - pi },
				{ r.x - pi, -pi - r.y, r.z + pi },
				{ r.x - pi, -pi - r.y, r.z - pi },
			};

			float errX = std::abs(DiffAngle(r.x, before.x));
			float errY = std::abs(DiffAngle(r.y, before.y));
			float errZ = std::abs(DiffAngle(r.z, before.z));
			float minErr = errX + errY + errZ;
			for (const auto test : tests)
			{
				float err = std::abs(DiffAngle(test.x, before.x))
					+ std::abs(DiffAngle(test.y, before.y))
					+ std::abs(DiffAngle(test.z, before.z));
				if (err < minErr)
				{
					minErr = err;
					r = test;
				}
			}
			return r;
		}

		glm::quat RotateFromTo(const glm::vec3& from, const glm::vec3& to)
		{
			auto const nf = glm::normalize(from);
			auto const nt = glm::normalize(to);
			auto const localW = glm::cross(nf, nt);
			auto dot = glm::dot(nf, nt);
			if (glm::abs(1.0f + dot) < 1.0e-7)
			{
				glm::vec3 v = glm::abs(from);
				if (v.x < v.y)
				{
					if (v.x < v.z) { v = glm::vec3(1, 0, 0); }
					else { v = glm::vec3(0, 0, 1); }
				}
				else
				{
					if (v.y < v.z) { v = glm::vec3(0, 1, 0); }
					else { v = glm::vec3(0, 0, 1); }
				}
				auto axis = glm::normalize(glm::cross(from, v));
				return glm::quat(0, axis);
			}
			else
			{
				return glm::normalize(glm::quat(1.0f + dot, localW));
			}
		}
	}

	void MMDIkSolver::SolveCore(uint32_t iteration)
	{
		auto ikPos = glm::vec3(m_ikNode->GetGlobalTransform()[3]);
		//for (auto& chain : m_chains)
		for (size_t chainIdx = 0; chainIdx < m_chains.size(); chainIdx++)
		{
			auto& chain = m_chains[chainIdx];
			MMDNode* chainNode = chain.m_node;
			if (chainNode == m_ikTarget)
			{
				/*
				ターゲットとチェインが同じ場合、 chainTargetVec が0ベクトルとなる。
				その後の計算で求める回転値がnanになるため、計算を行わない
				対象モデル：ぽんぷ長式比叡.pmx
				*/
				continue;
			}

			if (chain.m_enableAxisLimit)
			{
				// X,Y,Z 軸のいずれかしか回転しないものは専用の Solver を使用する
				if ((chain.m_limitMin.x != 0 || chain.m_limitMax.x != 0) &&
					(chain.m_limitMin.y == 0 || chain.m_limitMax.y == 0) &&
					(chain.m_limitMin.z == 0 || chain.m_limitMax.z == 0)
					)
				{
					SolvePlane(iteration, chainIdx, SolveAxis::X);
					continue;
				}
				else if ((chain.m_limitMin.y != 0 || chain.m_limitMax.y != 0) &&
					(chain.m_limitMin.x == 0 || chain.m_limitMax.x == 0) &&
					(chain.m_limitMin.z == 0 || chain.m_limitMax.z == 0)
					)
				{
					SolvePlane(iteration, chainIdx, SolveAxis::Y);
					continue;
				}
				else if ((chain.m_limitMin.z != 0 || chain.m_limitMax.z != 0) &&
					(chain.m_limitMin.x == 0 || chain.m_limitMax.x == 0) &&
					(chain.m_limitMin.y == 0 || chain.m_limitMax.y == 0)
					)
				{
					SolvePlane(iteration, chainIdx, SolveAxis::Z);
					continue;
				}
			}

			auto targetPos = glm::vec3(m_ikTarget->GetGlobalTransform()[3]);

			auto invChain = glm::inverse(chain.m_node->GetGlobalTransform());

			auto chainIkPos = glm::vec3(invChain * glm::vec4(ikPos, 1));
			auto chainTargetPos = glm::vec3(invChain * glm::vec4(targetPos, 1));

			auto chainIkVec = glm::normalize(chainIkPos);
			auto chainTargetVec = glm::normalize(chainTargetPos);

			auto dot = glm::dot(chainTargetVec, chainIkVec);
			dot = glm::clamp(dot, -1.0f, 1.0f);

			float angle = std::acos(dot);
			float angleDeg = glm::degrees(angle);
			if (angleDeg < 1.0e-3f)
			{
				continue;
			}
			angle = glm::clamp(angle, -m_limitAngle, m_limitAngle);
			auto cross = glm::normalize(glm::cross(chainTargetVec, chainIkVec));
			auto rot = glm::rotate(glm::quat(1, 0, 0, 0), angle, cross);

			auto chainRotM = glm::mat3_cast(chainNode->GetIKRotate())
				* glm::mat3_cast(chainNode->AnimateRotate())
				* glm::mat3_cast(rot);
			if (chain.m_enableAxisLimit)
			{
				auto rotXYZ = Decompose(chainRotM, chain.m_prevAngle);
				glm::vec3 clampXYZ;
				clampXYZ = glm::clamp(rotXYZ, chain.m_limitMin, chain.m_limitMax);

				clampXYZ = glm::clamp(clampXYZ - chain.m_prevAngle, -m_limitAngle, m_limitAngle) + chain.m_prevAngle;
				auto r = glm::rotate(glm::quat(1, 0, 0, 0), clampXYZ.x, glm::vec3(1, 0, 0));
				r = glm::rotate(r, clampXYZ.y, glm::vec3(0, 1, 0));
				r = glm::rotate(r, clampXYZ.z, glm::vec3(0, 0, 1));
				chainRotM = glm::mat3_cast(r);
				chain.m_prevAngle = clampXYZ;
			}

			auto ikRotM = chainRotM
				* glm::inverse(glm::mat3_cast(chainNode->AnimateRotate()));
			chainNode->SetIKRotate(glm::quat_cast(ikRotM));

			chainNode->UpdateLocalTransform();
			chainNode->UpdateGlobalTransform();
		}
	}

	void MMDIkSolver::SolvePlane(uint32_t iteration, size_t chainIdx, SolveAxis solveAxis)
	{
		int RotateAxisIndex = 0; // X axis
		glm::vec3 RotateAxis = glm::vec3(1, 0, 0);
		glm::vec3 Plane = glm::vec3(0, 1, 1);
		switch (solveAxis)
		{
		case SolveAxis::X:
			RotateAxisIndex = 0; // X axis
			RotateAxis = glm::vec3(1, 0, 0);
			Plane = glm::vec3(0, 1, 1);
			break;
		case SolveAxis::Y:
			RotateAxisIndex = 1; // Y axis
			RotateAxis = glm::vec3(0, 1, 0);
			Plane = glm::vec3(1, 0, 1);
			break;
		case SolveAxis::Z:
			RotateAxisIndex = 2; // Z axis
			RotateAxis = glm::vec3(0, 0, 1);
			Plane = glm::vec3(1, 1, 0);
			break;
		default:
			break;
		}

		auto& chain = m_chains[chainIdx];
		auto ikPos = glm::vec3(m_ikNode->GetGlobalTransform()[3]);

		auto targetPos = glm::vec3(m_ikTarget->GetGlobalTransform()[3]);

		auto invChain = glm::inverse(chain.m_node->GetGlobalTransform());

		auto chainIkPos = glm::vec3(invChain * glm::vec4(ikPos, 1));
		auto chainTargetPos = glm::vec3(invChain * glm::vec4(targetPos, 1));

		auto chainIkVec = glm::normalize(chainIkPos);
		auto chainTargetVec = glm::normalize(chainTargetPos);

		auto dot = glm::dot(chainTargetVec, chainIkVec);
		dot = glm::clamp(dot, -1.0f, 1.0f);

		float angle = std::acos(dot);
		float angleDeg = glm::degrees(angle);

		angle = glm::clamp(angle, -m_limitAngle, m_limitAngle);

		auto rot1 = glm::rotate(glm::quat(), angle, RotateAxis);
		auto targetVec1 = rot1 * chainTargetVec;
		auto dot1 = glm::dot(targetVec1, chainIkVec);

		auto rot2 = glm::rotate(glm::quat(), -angle, RotateAxis);
		auto targetVec2 = rot2 * chainTargetVec;
		auto dot2 = glm::dot(targetVec2, chainIkVec);

		if (iteration != 0)
		{
			// 初回以外は動かすかどうか判断する
			// 回転面とボーンが平行ではないため、動かさないほうが良い場合がある
			if (dot > dot1 && dot > dot2)
			{
				// 動かさないほうが差が小さい
				return;
			}
		}

		auto newAngle = chain.m_planeModeAngle;
		if (dot1 > dot2)
		{
			newAngle += angle;
		}
		else
		{
			newAngle -= angle;
		}
		if (iteration == 0)
		{
			if (newAngle < chain.m_limitMin[RotateAxisIndex] || newAngle > chain.m_limitMax[RotateAxisIndex])
			{
				if (-newAngle > chain.m_limitMin[RotateAxisIndex] && -newAngle < chain.m_limitMax[RotateAxisIndex])
				{
					newAngle *= -1;
				}
				else
				{
					auto halfRad = (chain.m_limitMin[RotateAxisIndex] + chain.m_limitMax[RotateAxisIndex]) * 0.5f;
					if (glm::abs(halfRad - newAngle) > glm::abs(halfRad + newAngle))
					{
						newAngle *= -1;
					}
				}
			}
		}

		newAngle = glm::clamp(newAngle, chain.m_limitMin[RotateAxisIndex], chain.m_limitMax[RotateAxisIndex]);
		chain.m_planeModeAngle = newAngle;

		auto ikRotM = glm::rotate(glm::quat(), newAngle, RotateAxis) * glm::inverse(chain.m_node->AnimateRotate());
		chain.m_node->SetIKRotate(ikRotM);

		chain.m_node->UpdateLocalTransform();
		chain.m_node->UpdateGlobalTransform();
	}
}

