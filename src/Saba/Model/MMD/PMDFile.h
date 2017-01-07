//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef SABA_MODEL_MMD_PMDFILE_H_
#define SABA_MODEL_MMD_PMDFILE_H_

#include "MMDFileString.h"

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <array>
#include <string>

namespace saba
{
	template <size_t Size>
	using PMDString = MMDFileString<Size>;

	struct PMDHeader
	{
		PMDString<3>	m_magic;
		float			m_version;
		PMDString<20>	m_modelName;
		PMDString<256>	m_comment;

		uint8_t			m_haveEnglishNameExt;
		PMDString<20>	m_englishModelNameExt;
		PMDString<256>	m_englishCommentExt;
	};

	struct PMDVertex
	{
		glm::vec3	m_position;
		glm::vec3	m_normal;
		glm::vec2	m_uv;
		uint16_t	m_bone[2];
		uint8_t		m_boneWeight;
		uint8_t		m_edge;
	};

	struct PMDFace
	{
		uint16_t  m_vertices[3];
	};

	struct PMDMaterial
	{
		glm::vec3		m_diffuse;
		float			m_alpha;
		float			m_specularPower;
		glm::vec3		m_specular;
		glm::vec3		m_ambient;
		uint8_t			m_toonIndex;
		uint8_t			m_edgeFlag;
		uint32_t		m_faceVertexCount;
		PMDString<20>	m_textureName;
	};

	struct PMDBone
	{
		PMDString<20>	m_boneName;
		uint16_t		m_parent;
		uint16_t		m_tail;
		uint8_t			m_boneType;
		uint16_t		m_ikParent;
		glm::vec3		m_position;
		PMDString<20>	m_englishBoneNameExt;
	};

	struct PMDIk
	{
		using ChainList = std::vector<uint16_t>;

		uint16_t	m_ikNode;
		uint16_t	m_ikTarget;
		uint8_t		m_numChain;
		uint16_t	m_numIteration;
		float		m_rotateLimit;
		ChainList	m_chanins;
	};

	struct PMDMorph
	{
		struct Vertex
		{
			uint32_t	m_vertexIndex;
			glm::vec3	m_position;
		};
		using VertexList = std::vector<Vertex>;

		enum MorphType : uint8_t
		{
			Base,
			Eyebrow,
			Eye,
			Rip,
			Other
		};

		PMDString<20>	m_morphName;
		MorphType		m_morphType;
		VertexList		m_vertices;
		PMDString<20>	m_englishShapeNameExt;

	};

	struct PMDMorphDisplayList
	{
		typedef std::vector<uint16_t> DisplayList;

		DisplayList	m_displayList;
	};

	struct PMDBoneDisplayList
	{
		typedef std::vector<uint16_t> DisplayList;

		PMDString<50>	m_name;
		DisplayList		m_displayList;
		PMDString<50>	m_englishNameExt;
	};

	enum class PMDRigidBodyShape : uint8_t
	{
		Sphere,		//!< 0:球
		Box,		//!< 1:箱
		Capsule,	//!< 0:カプセル
	};

	enum class PMDRigidBodyOperation : uint8_t
	{
		Static,				//!< 0:Bone追従
		Dynamic,			//!< 1:物理演算
		DynamicAdjustBone	//!< 2:物理演算(Bone位置合わせ)
	};

	struct PMDRigidBodyExt
	{
		PMDString<20>	m_rigidBodyName;
		uint16_t		m_boneIndex;
		uint8_t			m_groupIndex;
		uint16_t		m_groupTarget;		// 衝突グループ（各ビットがグループとの衝突フラグになっている）
		PMDRigidBodyShape	m_shapeType;
		float			m_shapeWidth;
		float			m_shapeHeight;
		float			m_shapeDepth;
		glm::vec3		m_pos;
		glm::vec3		m_rot;
		float			m_rigidBodyWeight;
		float			m_rigidBodyPosDimmer;
		float			m_rigidBodyRotDimmer;
		float			m_rigidBodyRecoil;
		float			m_rigidBodyFriction;
		PMDRigidBodyOperation	m_rigidBodyType;
	};

	struct PMDJointExt
	{
		enum { NumJointName = 20 };

		PMDString<20>	m_jointName;
		uint32_t		m_rigidBodyA;
		uint32_t		m_rigidBodyB;
		glm::vec3		m_jointPos;
		glm::vec3		m_jointRot;
		glm::vec3		m_constrainPos1;
		glm::vec3		m_constrainPos2;
		glm::vec3		m_constrainRot1;
		glm::vec3		m_constrainRot2;
		glm::vec3		m_springPos;
		glm::vec3		m_springRot;
	};

	struct PMDFile
	{
		PMDHeader						m_header;
		std::vector<PMDVertex>			m_vertices;
		std::vector<PMDFace>			m_faces;
		std::vector<PMDMaterial>		m_materials;
		std::vector<PMDBone>			m_bones;
		std::vector<PMDIk>				m_iks;
		std::vector<PMDMorph>			m_morphs;
		PMDMorphDisplayList				m_morphDisplayList;
		std::vector<PMDBoneDisplayList>	m_boneDisplayLists;
		std::array<PMDString<100>, 10>	m_toonTextureNames;
		std::vector<PMDRigidBodyExt>	m_rigidBodies;
		std::vector<PMDJointExt>		m_joints;
	};

	bool ReadPMDFile(PMDFile* pmdFile, const char* filename);

}

#endif // !SABA_MODEL_MMD_PMDFILE_H_
