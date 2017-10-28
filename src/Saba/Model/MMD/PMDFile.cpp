//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "PMDFile.h"

#include <Saba/Base/File.h>
#include <Saba/Base/Log.h>

#include <sstream>
#include <iomanip>

namespace saba
{
	namespace
	{
		template <typename T>
		bool Read(T* data, File& file)
		{
			return file.Read(data);
		}

		bool ReadHeader(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			auto& header = pmdFile->m_header;
			Read(&header.m_magic, file);
			Read(&header.m_version, file);
			Read(&header.m_modelName, file);
			Read(&header.m_comment, file);

			header.m_haveEnglishNameExt = 0;

			if (header.m_magic.ToString() != "Pmd")
			{
				SABA_ERROR("PMD Header Error.");
				return false;
			}

			if (header.m_version != 1.0f)
			{
				SABA_ERROR("PMD Version Error.");
				return false;
			}

			return !file.IsBad();
		}

		bool ReadVertex(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint32_t vertexCount = 0;
			if (!Read(&vertexCount, file))
			{
				return false;
			}

			auto& vertices = pmdFile->m_vertices;
			vertices.resize(vertexCount);
			for (auto& vertex : vertices)
			{
				Read(&vertex.m_position, file);
				Read(&vertex.m_normal, file);
				Read(&vertex.m_uv, file);
				Read(&vertex.m_bone[0], file);
				Read(&vertex.m_bone[1], file);
				Read(&vertex.m_boneWeight, file);
				Read(&vertex.m_edge, file);
			}

			return !file.IsBad();
		}

		bool ReadFace(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint32_t faceCount = 0;
			if (!Read(&faceCount, file))
			{
				return false;
			}

			auto& faces = pmdFile->m_faces;
			faces.resize(faceCount / 3);
			for (auto& face : faces)
			{
				Read(&face.m_vertices[0], file);
				Read(&face.m_vertices[1], file);
				Read(&face.m_vertices[2], file);
			}

			return !file.IsBad();
		}

		bool ReadMaterial(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint32_t materialCount = 0;
			if (!Read(&materialCount, file))
			{
				return false;
			}

			auto& materials = pmdFile->m_materials;
			materials.resize(materialCount);
			for (auto& material : materials)
			{
				Read(&material.m_diffuse, file);
				Read(&material.m_alpha, file);
				Read(&material.m_specularPower, file);
				Read(&material.m_specular, file);
				Read(&material.m_ambient, file);
				Read(&material.m_toonIndex, file);
				Read(&material.m_edgeFlag, file);
				Read(&material.m_faceVertexCount, file);
				Read(&material.m_textureName, file);
			}

			return !file.IsBad();
		}

		bool ReadBone(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint16_t boneCount = 0;
			if (!Read(&boneCount, file))
			{
				return false;
			}

			auto& bones = pmdFile->m_bones;
			bones.resize(boneCount);
			for (auto& bone : bones)
			{
				Read(&bone.m_boneName, file);
				Read(&bone.m_parent, file);
				Read(&bone.m_tail, file);
				Read(&bone.m_boneType, file);
				Read(&bone.m_ikParent, file);
				Read(&bone.m_position, file);
			}

			return !file.IsBad();
		}

		bool ReadIK(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint16_t ikCount = 0;
			if (!Read(&ikCount, file))
			{
				return false;
			}

			auto& iks = pmdFile->m_iks;
			iks.resize(ikCount);
			for (auto& ik : iks)
			{
				Read(&ik.m_ikNode, file);
				Read(&ik.m_ikTarget, file);
				Read(&ik.m_numChain, file);
				Read(&ik.m_numIteration, file);
				Read(&ik.m_rotateLimit, file);

				ik.m_chanins.resize(ik.m_numChain);
				for (auto& ikChain : ik.m_chanins)
				{
					Read(&ikChain, file);
				}
			}

			return !file.IsBad();
		}

		bool ReadBlendShape(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint16_t blendShapeCount = 0;
			if (!Read(&blendShapeCount, file))
			{
				return false;
			}

			auto& morphs = pmdFile->m_morphs;
			morphs.resize(blendShapeCount);
			for (auto& morph : morphs)
			{
				Read(&morph.m_morphName, file);

				uint32_t vertexCount = 0;
				Read(&vertexCount, file);
				morph.m_vertices.resize(vertexCount);

				Read(&morph.m_morphType, file);
				for (auto& vtx : morph.m_vertices)
				{
					Read(&vtx.m_vertexIndex, file);
					Read(&vtx.m_position, file);
				}
			}

			return !file.IsBad();
		}

		bool ReadBlendShapeDisplayList(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint8_t displayListCount = 0;
			if (!Read(&displayListCount, file))
			{
				return false;
			}

			pmdFile->m_morphDisplayList.m_displayList.resize(displayListCount);
			for (auto& displayList : pmdFile->m_morphDisplayList.m_displayList)
			{
				Read(&displayList, file);
			}

			return !file.IsBad();
		}

		bool ReadBoneDisplayList(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint8_t displayListCount = 0;
			if (!Read(&displayListCount, file))
			{
				return false;
			}

			// ボーンの枠にはデフォルトでセンターが用意されているので、サイズを一つ多くする
			pmdFile->m_boneDisplayLists.resize(displayListCount + 1);
			bool first = true;
			for (auto& displayList : pmdFile->m_boneDisplayLists)
			{
				if (first)
				{
					first = false;
				}
				else
				{
					Read(&displayList.m_name, file);
				}
			}

			uint32_t displayCount = 0;
			if (!Read(&displayCount, file))
			{
				return false;
			}
			for (uint32_t displayIdx = 0; displayIdx < displayCount; displayIdx++)
			{
				uint16_t boneIdx = 0;
				Read(&boneIdx, file);

				uint8_t frameIdx = 0;
				Read(&frameIdx, file);
				pmdFile->m_boneDisplayLists[frameIdx].m_displayList.push_back(boneIdx);
			}

			return !file.IsBad();
		}

		bool ReadExt(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			Read(&pmdFile->m_header.m_haveEnglishNameExt, file);

			if (pmdFile->m_header.m_haveEnglishNameExt != 0)
			{
				// ModelName
				Read(&pmdFile->m_header.m_englishModelNameExt, file);

				// Comment
				Read(&pmdFile->m_header.m_englishCommentExt, file);

				// BoneName
				for (auto& bone : pmdFile->m_bones)
				{
					Read(&bone.m_englishBoneNameExt, file);
				}

				// BlendShape Name
				/*
				BlendShapeの最初はbaseなので、EnglishNameを読まないようにする
				*/
				size_t numBlensShape = pmdFile->m_morphs.size();
				for (size_t bsIdx = 1; bsIdx < numBlensShape; bsIdx++)
				{
					auto& morpsh = pmdFile->m_morphs[bsIdx];
					Read(&morpsh.m_englishShapeNameExt, file);
				}

				// BoneDisplayNameLists
				/*
				BoneDisplayNameの最初はセンターとして使用しているので、EnglishNameを読まないようにする
				*/
				size_t numBoneDisplayName = pmdFile->m_boneDisplayLists.size();
				for (size_t displayIdx = 1; displayIdx < numBoneDisplayName; displayIdx++)
				{
					auto& display = pmdFile->m_boneDisplayLists[displayIdx];
					Read(&display.m_englishNameExt, file);
				}
			}

			return !file.IsBad();
		}

		bool ReadToonTextureName(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			for (auto& toonTexName : pmdFile->m_toonTextureNames)
			{
				Read(&toonTexName, file);
			}

			return !file.IsBad();
		}

		bool ReadRigidBodyExt(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint32_t rigidBodyCount = 0;
			if (!Read(&rigidBodyCount, file))
			{
				return false;
			}

			pmdFile->m_rigidBodies.resize(rigidBodyCount);
			for (auto& rb : pmdFile->m_rigidBodies)
			{
				Read(&rb.m_rigidBodyName, file);
				Read(&rb.m_boneIndex, file);
				Read(&rb.m_groupIndex, file);
				Read(&rb.m_groupTarget, file);
				Read(&rb.m_shapeType, file);
				Read(&rb.m_shapeWidth, file);
				Read(&rb.m_shapeHeight, file);
				Read(&rb.m_shapeDepth, file);
				Read(&rb.m_pos, file);
				Read(&rb.m_rot, file);
				Read(&rb.m_rigidBodyWeight, file);
				Read(&rb.m_rigidBodyPosDimmer, file);
				Read(&rb.m_rigidBodyRotDimmer, file);
				Read(&rb.m_rigidBodyRecoil, file);
				Read(&rb.m_rigidBodyFriction, file);
				Read(&rb.m_rigidBodyType, file);
			}

			return !file.IsBad();
		}

		bool ReadJointExt(PMDFile* pmdFile, File& file)
		{
			if (file.IsBad())
			{
				return false;
			}

			uint32_t jointCount = 0;
			if (!Read(&jointCount, file))
			{
				return false;
			}

			pmdFile->m_joints.resize(jointCount);
			for (auto& joint : pmdFile->m_joints)
			{
				Read(&joint.m_jointName, file);
				Read(&joint.m_rigidBodyA, file);
				Read(&joint.m_rigidBodyB, file);
				Read(&joint.m_jointPos, file);
				Read(&joint.m_jointRot, file);
				Read(&joint.m_constrainPos1, file);
				Read(&joint.m_constrainPos2, file);
				Read(&joint.m_constrainRot1, file);
				Read(&joint.m_constrainRot2, file);
				Read(&joint.m_springPos, file);
				Read(&joint.m_springRot, file);
			}

			return !file.IsBad();
		}

		bool ReadPMDFile(PMDFile* pmdFile, File& file)
		{
			if (!ReadHeader(pmdFile, file))
			{
				SABA_ERROR("ReadHeader Fail.");
				return false;
			}

			if (!ReadVertex(pmdFile, file))
			{
				SABA_ERROR("ReadVertex Fail.");
				return false;
			}

			if (!ReadFace(pmdFile, file))
			{
				SABA_ERROR("ReadFace Fail.");
				return false;
			}

			if (!ReadMaterial(pmdFile, file))
			{
				SABA_ERROR("ReadMaterial Fail.");
				return false;
			}

			if (!ReadBone(pmdFile, file))
			{
				SABA_ERROR("ReadBone Fail.");
				return false;
			}

			if (!ReadIK(pmdFile, file))
			{
				SABA_ERROR("ReadIK Fail.");
				return false;
			}

			if (!ReadBlendShape(pmdFile, file))
			{
				SABA_ERROR("ReadBlendShape Fail.");
				return false;
			}

			if (!ReadBlendShapeDisplayList(pmdFile, file))
			{
				SABA_ERROR("ReadBlendShapeDisplayList Fail.");
				return false;
			}

			if (!ReadBoneDisplayList(pmdFile, file))
			{
				SABA_ERROR("ReadBoneDisplayList Fail.");
				return false;
			}

			size_t toonTexIdx = 1;
			for (auto& toonTexName : pmdFile->m_toonTextureNames)
			{
				std::stringstream ss;
				ss << "toon" << std::setfill('0') << std::setw(2) << toonTexIdx << ".bmp";
				std::string name = ss.str();
				toonTexName.Set(name.c_str());
				toonTexIdx++;
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadExt(pmdFile, file))
				{
					SABA_ERROR("ReadExt Fail.");
					return false;
				}
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadToonTextureName(pmdFile, file))
				{
					SABA_ERROR("ReadToonTextureName Fail.");
					return false;
				}
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadRigidBodyExt(pmdFile, file))
				{
					SABA_ERROR("ReadRigidBodyExt Fail.");
					return false;
				}
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadJointExt(pmdFile, file))
				{
					SABA_ERROR("ReadJointExt Fail.");
					return false;
				}
			}

			return true;
		}
	}

	bool ReadPMDFile(PMDFile* pmdFile, const char * filename)
	{
		SABA_INFO("PMD File Open. {}", filename);

		File file;
		if (!file.Open(filename))
		{
			SABA_INFO("PMD File Open Fail. {}", filename);
			return false;
		}

		if (!ReadPMDFile(pmdFile, file))
		{
			SABA_INFO("PMD File Read Fail. {}", filename);
			return false;
		}
		SABA_INFO("PMD File Read Successed. {}", filename);

		return true;
	}

}
