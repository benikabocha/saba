#ifndef SABA_MODEL_MMD_MMDIKSOLVER_H_
#define	SABA_MODEL_MMD_MMDIKSOLVER_H_

#include "MMDNode.h"

#include <vector>
#include <glm/vec3.hpp>

namespace saba
{
	class MMDIkSolver
	{
	public:
		MMDIkSolver();

		void SetIKNode(MMDNode* node) { m_ikNode = node; }
		void SetTargetNode(MMDNode* node) { m_ikTarget = node; }
		MMDNode* GetIKNode() const { return m_ikNode; }
		MMDNode* GetTargetNode() const { return m_ikTarget; }
		std::string GetName() const
		{
			if (m_ikNode != nullptr)
			{
				return m_ikNode->GetName();
			}
			else
			{
				return "";
			}
		}

		void SetIterateCount(uint32_t count) { m_iterateCount = count; }
		void SetLimitAngle(float angle) { m_limitAngle = angle; }
		void Enable(bool enable) { m_enable = enable; }

		void AddIKChain(MMDNode* node, bool isKnee = false);
		void AddIKChain(
			MMDNode* node,
			bool axisLimit,
			const glm::vec3& limixMin,
			const glm::vec3& limitMax
		);

		void Solve();

	private:
		struct IKChain
		{
			MMDNode*	m_node;
			bool		m_enableAxisLimit;
			glm::vec3	m_limitMax;
			glm::vec3	m_limitMin;
			glm::vec3	m_prevAngle;
		};

	private:
		void SolveCore(uint32_t iteration);

	private:
		std::vector<IKChain>	m_chains;
		MMDNode*	m_ikNode;
		MMDNode*	m_ikTarget;
		uint32_t	m_iterateCount;
		float		m_limitAngle;
		bool		m_enable;

	};
}

#endif // !SABA_MODEL_MMD_MMDIKSOLVER_H_
