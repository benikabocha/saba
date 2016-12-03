#include "MMDIkSolver.h"

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
		m_chains.push_back(chain);
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
		m_chains.push_back(chain);
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
			if (chain.m_enableAxisLimit)
			{
				auto angle = (chain.m_limitMax - chain.m_limitMin) * 0.5f;
				auto r = glm::rotate(glm::quat(), angle.x, glm::vec3(1, 0, 0));
				r = glm::rotate(r, angle.y, glm::vec3(0, 1, 0));
				r = glm::rotate(r, angle.z, glm::vec3(0, 0, 1));
				chain.m_prevAngle = angle;
				auto rm = glm::mat3_cast(r) * glm::inverse(glm::mat3_cast(chain.m_node->m_rotate));
				chain.m_node->m_ikRotate = glm::quat_cast(rm);
			}
			else
			{
				chain.m_node->m_ikRotate = glm::quat();
			}

			chain.m_node->UpdateLocalMatrix();
			chain.m_node->UpdateGlobalMatrix();
		}

		for (uint32_t i = 0; i < m_iterateCount; i++)
		{
			SolveCore(i);
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
	}

	void MMDIkSolver::SolveCore(uint32_t iteration)
	{
		auto ikPos =  glm::vec3(m_ikNode->m_global[3]);
		for (auto& chain : m_chains)
		{
			MMDNode* chainNode = chain.m_node;
			auto targetPos = glm::vec3(m_ikTarget->m_global[3]);

			auto invChain = glm::inverse(chain.m_node->m_global);

			auto chainIkPos = glm::vec3(invChain * glm::vec4(ikPos, 1));
			auto chainTargetPos = glm::vec3(invChain * glm::vec4(targetPos, 1));

			auto chainIkVec = glm::normalize(chainIkPos);
			auto chainTargetVec = glm::normalize(chainTargetPos);

			auto dot = glm::dot(chainTargetVec, chainIkVec);
			if ((1.0f - dot) < 0.00001f)
			{
				continue;
			}

			float angle = std::acos(dot);
			angle = glm::clamp(angle, -m_limitAngle, m_limitAngle);
			auto cross = glm::cross(chainTargetVec, chainIkVec);
			auto rot = glm::rotate(glm::quat(), angle, cross);

			bool avoidUpdate = false;
			auto chainRotM = glm::mat3_cast(chainNode->m_ikRotate)
				* glm::mat3_cast(chainNode->m_rotate)
				* glm::mat3_cast(rot);
			if (chain.m_enableAxisLimit)
			{
				auto rotXYZ = Decompose(chainRotM, chain.m_prevAngle);
				glm::vec3 clampXYZ;
				clampXYZ.x = ClampAngle(rotXYZ.x, chain.m_limitMin.x, chain.m_limitMax.x);
				clampXYZ.y = ClampAngle(rotXYZ.y, chain.m_limitMin.y, chain.m_limitMax.y);
				clampXYZ.z = ClampAngle(rotXYZ.z, chain.m_limitMin.z, chain.m_limitMax.z);

				clampXYZ = glm::clamp(clampXYZ - chain.m_prevAngle, -m_limitAngle, m_limitAngle) + chain.m_prevAngle;
				auto r = glm::rotate(glm::quat(), clampXYZ.x, glm::vec3(1, 0, 0));
				r = glm::rotate(r, clampXYZ.y, glm::vec3(0, 1, 0));
				r = glm::rotate(r, clampXYZ.z, glm::vec3(0, 0, 1));
				chainRotM = glm::mat3_cast(r);
				if (iteration == 0)
				{
					/*
					最初の IKSolve で Limit に達すると、
					以降IKが動かなくなることがある。
					初回の Solve のみ Limit に達した場合は、何も行わないようにする。
					*/
					if (clampXYZ.x == chain.m_limitMin.x ||
						clampXYZ.y == chain.m_limitMin.y ||
						clampXYZ.z == chain.m_limitMin.z ||
						clampXYZ.x == chain.m_limitMax.x ||
						clampXYZ.y == chain.m_limitMax.y ||
						clampXYZ.z == chain.m_limitMax.z
						)
					{
						avoidUpdate = true;
					}
				}
				if (!avoidUpdate)
				{
					chain.m_prevAngle = clampXYZ;
				}
			}

			if (!avoidUpdate)
			{
				auto ikRotM = chainRotM
					* glm::inverse(glm::mat3_cast(chainNode->m_rotate));
				chainNode->m_ikRotate = glm::quat_cast(ikRotM);
			}

			chainNode->UpdateLocalMatrix();
			chainNode->UpdateGlobalMatrix();
		}
	}
}

