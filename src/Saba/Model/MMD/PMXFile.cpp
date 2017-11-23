//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "PMXFile.h"

#include <Saba/Base/Log.h>
#include <Saba/Base/File.h>
#include <Saba/Base/UnicodeUtil.h>

#include <vector>

namespace saba
{
	namespace
	{

		template <typename T>
		bool Read(T* val, File& file)
		{
			return file.Read(val);
		}

		template <typename T>
		bool Read(T* valArray, size_t size, File& file)
		{
			return file.Read(valArray, size);
		}

		bool ReadString(PMXFile* pmx, std::string* val, File& file)
		{
			uint32_t bufSize;
			if (!Read(&bufSize, file))
			{
				return false;
			}

			if (bufSize > 0)
			{
				if (pmx->m_header.m_encode == 0)
				{
					// UTF-16
					std::u16string utf16Str(bufSize / 2, u'\0');
					if (!file.Read(&utf16Str[0], utf16Str.size()))
					{
						return false;
					}
					if (!ConvU16ToU8(utf16Str, *val))
					{
						return false;
					}
				}
				else if (pmx->m_header.m_encode == 1)
				{
					// UTF-8
					std::string utf8Str(bufSize, '\0');
					file.Read(&utf8Str[0], bufSize);

					*val = utf8Str;
				}
			}

			return !file.IsBad();
		}

		bool ReadIndex(int32_t* index, uint8_t indexSize, File& file)
		{
			switch (indexSize)
			{
			case 1:
			{
				uint8_t idx;
				Read(&idx, file);
				if (idx != 0xFF)
				{
					*index = (int32_t)idx;
				}
				else
				{
					*index = -1;
				}
			}
				break;
			case 2:
			{
				uint16_t idx;
				Read(&idx, file);
				if (idx != 0xFFFF)
				{
					*index = (int32_t)idx;
				}
				else
				{
					*index = -1;
				}
			}
				break;
			case 4:
			{
				uint32_t idx;
				Read(&idx, file);
				*index = (int32_t)idx;
			}
				break;
			default:
				return false;
			}
			return !file.IsBad();
		}

		bool ReadHeader(PMXFile* pmxFile, File& file)
		{
			auto& header = pmxFile->m_header;

			Read(&header.m_magic, file);
			Read(&header.m_version, file);

			Read(&header.m_dataSize, file);

			Read(&header.m_encode, file);
			Read(&header.m_addUVNum, file);

			Read(&header.m_vertexIndexSize, file);
			Read(&header.m_textureIndexSize, file);
			Read(&header.m_materialIndexSize, file);
			Read(&header.m_boneIndexSize, file);
			Read(&header.m_morphIndexSize, file);
			Read(&header.m_rigidbodyIndexSize, file);

			return !file.IsBad();
		}

		bool ReadInfo(PMXFile* pmx, File& file)
		{
			auto& info = pmx->m_info;

			ReadString(pmx, &info.m_modelName, file);
			ReadString(pmx, &info.m_englishModelName, file);
			ReadString(pmx, &info.m_comment, file);
			ReadString(pmx, &info.m_englishComment, file);

			return !file.IsBad();
		}

		bool ReadVertex(PMXFile* pmx, File& file)
		{
			int32_t vertexCount;
			if (!Read(&vertexCount, file))
			{
				return false;
			}

			auto& vertices = pmx->m_vertices;
			vertices.resize(vertexCount);
			for (auto& vertex : vertices)
			{
				Read(&vertex.m_position, file);
				Read(&vertex.m_normal, file);
				Read(&vertex.m_uv, file);

				for (uint8_t i = 0; i < pmx->m_header.m_addUVNum; i++)
				{
					Read(&vertex.m_addUV[i], file);
				}

				Read(&vertex.m_weightType, file);

				switch (vertex.m_weightType)
				{
				case PMXVertexWeight::BDEF1:
					ReadIndex(&vertex.m_boneIndices[0], pmx->m_header.m_boneIndexSize, file);
					break;
				case PMXVertexWeight::BDEF2:
					ReadIndex(&vertex.m_boneIndices[0], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[1], pmx->m_header.m_boneIndexSize, file);
					Read(&vertex.m_boneWeights[0], file);
					break;
				case PMXVertexWeight::BDEF4:
					ReadIndex(&vertex.m_boneIndices[0], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[1], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[2], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[3], pmx->m_header.m_boneIndexSize, file);
					Read(&vertex.m_boneWeights[0], file);
					Read(&vertex.m_boneWeights[1], file);
					Read(&vertex.m_boneWeights[2], file);
					Read(&vertex.m_boneWeights[3], file);
					break;
				case PMXVertexWeight::SDEF:
					ReadIndex(&vertex.m_boneIndices[0], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[1], pmx->m_header.m_boneIndexSize, file);
					Read(&vertex.m_boneWeights[0], file);
					Read(&vertex.m_sdefC, file);
					Read(&vertex.m_sdefR0, file);
					Read(&vertex.m_sdefR1, file);
					break;
				case PMXVertexWeight::QDEF:
					ReadIndex(&vertex.m_boneIndices[0], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[1], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[2], pmx->m_header.m_boneIndexSize, file);
					ReadIndex(&vertex.m_boneIndices[3], pmx->m_header.m_boneIndexSize, file);
					Read(&vertex.m_boneWeights[0], file);
					Read(&vertex.m_boneWeights[1], file);
					Read(&vertex.m_boneWeights[3], file);
					Read(&vertex.m_boneWeights[4], file);
					break;
				default:
					return false;
				}
				Read(&vertex.m_edgeMag, file);
			}

			return !file.IsBad();
		}

		bool ReadFace(PMXFile* pmx, File& file)
		{
			int32_t faceCount = 0;
			if (!Read(&faceCount, file))
			{
				return false;
			}
			faceCount /= 3;

			pmx->m_faces.resize(faceCount);

			switch (pmx->m_header.m_vertexIndexSize)
			{
			case 1:
			{
				std::vector<uint8_t> vertices(faceCount * 3);
				Read(vertices.data(), vertices.size(), file);
				for (int32_t faceIdx = 0; faceIdx < faceCount; faceIdx++)
				{
					pmx->m_faces[faceIdx].m_vertices[0] = vertices[faceIdx * 3 + 0];
					pmx->m_faces[faceIdx].m_vertices[1] = vertices[faceIdx * 3 + 1];
					pmx->m_faces[faceIdx].m_vertices[2] = vertices[faceIdx * 3 + 2];
				}
			}
				break;
			case 2:
			{
				std::vector<uint16_t> vertices(faceCount * 3);
				Read(vertices.data(), vertices.size(), file);
				for (int32_t faceIdx = 0; faceIdx < faceCount; faceIdx++)
				{
					pmx->m_faces[faceIdx].m_vertices[0] = vertices[faceIdx * 3 + 0];
					pmx->m_faces[faceIdx].m_vertices[1] = vertices[faceIdx * 3 + 1];
					pmx->m_faces[faceIdx].m_vertices[2] = vertices[faceIdx * 3 + 2];
				}
			}
				break;
			case 4:
			{
				std::vector<uint32_t> vertices(faceCount * 3);
				Read(vertices.data(), vertices.size(), file);
				for (int32_t faceIdx = 0; faceIdx < faceCount; faceIdx++)
				{
					pmx->m_faces[faceIdx].m_vertices[0] = vertices[faceIdx * 3 + 0];
					pmx->m_faces[faceIdx].m_vertices[1] = vertices[faceIdx * 3 + 1];
					pmx->m_faces[faceIdx].m_vertices[2] = vertices[faceIdx * 3 + 2];
				}
			}
				break;
			default:
				return false;
			}

			return !file.IsBad();
		}

		bool ReadTexture(PMXFile* pmx, File& file)
		{
			int32_t texCount = 0;
			if (!Read(&texCount, file))
			{
				return false;
			}

			pmx->m_textures.resize(texCount);

			for (auto& tex : pmx->m_textures)
			{
				ReadString(pmx, &tex.m_textureName, file);
			}

			return !file.IsBad();
		}

		bool ReadMaterial(PMXFile* pmx, File& file)
		{
			int32_t matCount = 0;
			if (!Read(&matCount, file))
			{
				return false;
			}

			pmx->m_materials.resize(matCount);

			for (auto& mat : pmx->m_materials)
			{
				ReadString(pmx, &mat.m_name, file);
				ReadString(pmx, &mat.m_englishName, file);

				Read(&mat.m_diffuse, file);
				Read(&mat.m_specular, file);
				Read(&mat.m_specularPower, file);
				Read(&mat.m_ambient, file);

				Read(&mat.m_drawMode, file);

				Read(&mat.m_edgeColor, file);
				Read(&mat.m_edgeSize, file);

				ReadIndex(&mat.m_textureIndex, pmx->m_header.m_textureIndexSize, file);
				ReadIndex(&mat.m_sphereTextureIndex, pmx->m_header.m_textureIndexSize, file);
				Read(&mat.m_sphereMode, file);

				Read(&mat.m_toonMode, file);
				if (mat.m_toonMode == PMXToonMode::Separate)
				{
					ReadIndex(&mat.m_toonTextureIndex, pmx->m_header.m_textureIndexSize, file);
				}
				else if (mat.m_toonMode == PMXToonMode::Common)
				{
					uint8_t toonIndex;
					Read(&toonIndex, file);
					mat.m_toonTextureIndex = (int32_t)toonIndex;
				}
				else
				{
					return false;
				}

				ReadString(pmx, &mat.m_memo, file);

				Read(&mat.m_numFaceVertices, file);
			}

			return !file.IsBad();
		}

		bool ReadBone(PMXFile* pmx, File& file)
		{
			int32_t boneCount;
			if (!Read(&boneCount, file))
			{
				return false;
			}

			pmx->m_bones.resize(boneCount);

			for (auto& bone : pmx->m_bones)
			{
				ReadString(pmx, &bone.m_name, file);
				ReadString(pmx, &bone.m_englishName, file);

				Read(&bone.m_position, file);
				ReadIndex(&bone.m_parentBoneIndex, pmx->m_header.m_boneIndexSize, file);
				Read(&bone.m_deformDepth, file);

				Read(&bone.m_boneFlag, file);

				if (((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::TargetShowMode) == 0)
				{
					Read(&bone.m_positionOffset, file);
				}
				else
				{
					ReadIndex(&bone.m_linkBoneIndex, pmx->m_header.m_boneIndexSize, file);
				}

				if (((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::AppendRotate) ||
					((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::AppendTranslate))
				{
					ReadIndex(&bone.m_appendBoneIndex, pmx->m_header.m_boneIndexSize, file);
					Read(&bone.m_appendWeight, file);
				}

				if ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::FixedAxis)
				{
					Read(&bone.m_fixedAxis, file);
				}

				if ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::LocalAxis)
				{
					Read(&bone.m_localXAxis, file);
					Read(&bone.m_localZAxis, file);
				}

				if ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::DeformOuterParent)
				{
					Read(&bone.m_keyValue, file);
				}

				if ((uint16_t)bone.m_boneFlag & (uint16_t)PMXBoneFlags::IK)
				{
					ReadIndex(&bone.m_ikTargetBoneIndex, pmx->m_header.m_boneIndexSize, file);
					Read(&bone.m_ikIterationCount, file);
					Read(&bone.m_ikLimit, file);

					int32_t linkCount;
					if (!Read(&linkCount, file))
					{
						return false;
					}

					bone.m_ikLinks.resize(linkCount);
					for (auto& ikLink : bone.m_ikLinks)
					{
						ReadIndex(&ikLink.m_ikBoneIndex, pmx->m_header.m_boneIndexSize, file);
						Read(&ikLink.m_enableLimit, file);

						if (ikLink.m_enableLimit != 0)
						{
							Read(&ikLink.m_limitMin, file);
							Read(&ikLink.m_limitMax, file);
						}
					}
				}
			}

			return !file.IsBad();
		}

		bool ReadMorph(PMXFile* pmx, File& file)
		{
			int32_t morphCount;
			if (!Read(&morphCount, file))
			{
				return false;
			}

			pmx->m_morphs.resize(morphCount);

			for (auto& morph : pmx->m_morphs)
			{
				ReadString(pmx, &morph.m_name, file);
				ReadString(pmx, &morph.m_englishName, file);

				Read(&morph.m_controlPanel, file);
				Read(&morph.m_morphType, file);

				int32_t dataCount;
				if (!Read(&dataCount, file))
				{
					return false;
				}

				if (morph.m_morphType == PMXMorphType::Position)
				{
					morph.m_positionMorph.resize(dataCount);
					for (auto& data : morph.m_positionMorph)
					{
						ReadIndex(&data.m_vertexIndex, pmx->m_header.m_vertexIndexSize, file);
						Read(&data.m_position, file);
					}
				}
				else if (morph.m_morphType == PMXMorphType::UV ||
					morph.m_morphType == PMXMorphType::AddUV1 ||
					morph.m_morphType == PMXMorphType::AddUV2 ||
					morph.m_morphType == PMXMorphType::AddUV3 ||
					morph.m_morphType == PMXMorphType::AddUV4
					)
				{
					morph.m_uvMorph.resize(dataCount);
					for (auto& data : morph.m_uvMorph)
					{
						ReadIndex(&data.m_vertexIndex, pmx->m_header.m_vertexIndexSize, file);
						Read(&data.m_uv, file);
					}
				}
				else if (morph.m_morphType == PMXMorphType::Bone)
				{
					morph.m_boneMorph.resize(dataCount);
					for (auto& data : morph.m_boneMorph)
					{
						ReadIndex(&data.m_boneIndex, pmx->m_header.m_boneIndexSize, file);
						Read(&data.m_position, file);
						Read(&data.m_quaternion, file);
					}
				}
				else if (morph.m_morphType == PMXMorphType::Material)
				{
					morph.m_materialMorph.resize(dataCount);
					for (auto& data : morph.m_materialMorph)
					{
						ReadIndex(&data.m_materialIndex, pmx->m_header.m_materialIndexSize, file);
						Read(&data.m_opType, file);
						Read(&data.m_diffuse, file);
						Read(&data.m_specular, file);
						Read(&data.m_specularPower, file);
						Read(&data.m_ambient, file);
						Read(&data.m_edgeColor, file);
						Read(&data.m_edgeSize, file);
						Read(&data.m_textureFactor, file);
						Read(&data.m_sphereTextureFactor, file);
						Read(&data.m_toonTextureFactor, file);
					}
				}
				else if (morph.m_morphType == PMXMorphType::Group)
				{
					morph.m_groupMorph.resize(dataCount);
					for (auto& data : morph.m_groupMorph)
					{
						ReadIndex(&data.m_morphIndex, pmx->m_header.m_morphIndexSize, file);
						Read(&data.m_weight, file);
					}
				}
				else if (morph.m_morphType == PMXMorphType::Flip)
				{
					morph.m_flipMorph.resize(dataCount);
					for (auto& data : morph.m_flipMorph)
					{
						ReadIndex(&data.m_morphIndex, pmx->m_header.m_morphIndexSize, file);
						Read(&data.m_weight, file);
					}
				}
				else if (morph.m_morphType == PMXMorphType::Impluse)
				{
					morph.m_impulseMorph.resize(dataCount);
					for (auto& data : morph.m_impulseMorph)
					{
						ReadIndex(&data.m_rigidbodyIndex, pmx->m_header.m_rigidbodyIndexSize, file);
						Read(&data.m_localFlag, file);
						Read(&data.m_translateVelocity, file);
						Read(&data.m_rotateTorque, file);
					}
				}
				else
				{
					SABA_ERROR("Unsupported Morph Type:[{}]", (int)morph.m_morphType);
					return false;
				}
			}

			return !file.IsBad();
		}

		bool ReadDisplayFrame(PMXFile* pmx, File& file)
		{
			int32_t displayFrameCount;
			if (!Read(&displayFrameCount, file))
			{
				return false;
			}

			pmx->m_displayFrames.resize(displayFrameCount);

			for (auto& displayFrame : pmx->m_displayFrames)
			{
				ReadString(pmx, &displayFrame.m_name, file);
				ReadString(pmx, &displayFrame.m_englishName, file);

				Read(&displayFrame.m_flag, file);
				int32_t targetCount;
				if (!Read(&targetCount, file))
				{
					return false;
				}
				displayFrame.m_targets.resize(targetCount);
				for (auto& target : displayFrame.m_targets)
				{
					Read(&target.m_type, file);
					if (target.m_type == PMXDispalyFrame::TargetType::BoneIndex)
					{
						ReadIndex(&target.m_index, pmx->m_header.m_boneIndexSize, file);
					}
					else if (target.m_type == PMXDispalyFrame::TargetType::MorphIndex)
					{
						ReadIndex(&target.m_index, pmx->m_header.m_morphIndexSize, file);
					}
					else
					{
						return false;
					}
				}
			}

			return !file.IsBad();
		}

		bool ReadRigidbody(PMXFile* pmx, File& file)
		{
			int32_t rbCount;
			if (!Read(&rbCount, file))
			{
				return false;
			}

			pmx->m_rigidbodies.resize(rbCount);

			for (auto& rb : pmx->m_rigidbodies)
			{
				ReadString(pmx, &rb.m_name, file);
				ReadString(pmx, &rb.m_englishName, file);

				ReadIndex(&rb.m_boneIndex, pmx->m_header.m_boneIndexSize, file);
				Read(&rb.m_group, file);
				Read(&rb.m_collisionGroup, file);

				Read(&rb.m_shape, file);
				Read(&rb.m_shapeSize, file);

				Read(&rb.m_translate, file);
				Read(&rb.m_rotate, file);

				Read(&rb.m_mass, file);
				Read(&rb.m_translateDimmer, file);
				Read(&rb.m_rotateDimmer, file);
				Read(&rb.m_repulsion, file);
				Read(&rb.m_friction, file);

				Read(&rb.m_op, file);
			}

			return !file.IsBad();
		}

		bool ReadJoint(PMXFile* pmx, File& file)
		{
			int32_t jointCount;
			if (!Read(&jointCount, file))
			{
				return false;
			}

			pmx->m_joints.resize(jointCount);

			for (auto& joint : pmx->m_joints)
			{
				ReadString(pmx, &joint.m_name, file);
				ReadString(pmx, &joint.m_englishName, file);

				Read(&joint.m_type, file);
				ReadIndex(&joint.m_rigidbodyAIndex, pmx->m_header.m_rigidbodyIndexSize, file);
				ReadIndex(&joint.m_rigidbodyBIndex, pmx->m_header.m_rigidbodyIndexSize, file);

				Read(&joint.m_translate, file);
				Read(&joint.m_rotate, file);

				Read(&joint.m_translateLowerLimit, file);
				Read(&joint.m_translateUpperLimit, file);
				Read(&joint.m_rotateLowerLimit, file);
				Read(&joint.m_rotateUpperLimit, file);

				Read(&joint.m_springTranslateFactor, file);
				Read(&joint.m_springRotateFactor, file);
			}

			return !file.IsBad();
		}

		bool ReadSoftbody(PMXFile* pmx, File& file)
		{
			int32_t sbCount;
			if (!Read(&sbCount, file))
			{
				return false;
			}

			pmx->m_softbodies.resize(sbCount);

			for (auto& sb : pmx->m_softbodies)
			{
				ReadString(pmx, &sb.m_name, file);
				ReadString(pmx, &sb.m_englishName, file);

				Read(&sb.m_type, file);

				ReadIndex(&sb.m_materialIndex, pmx->m_header.m_materialIndexSize, file);

				Read(&sb.m_group, file);
				Read(&sb.m_collisionGroup, file);

				Read(&sb.m_flag, file);

				Read(&sb.m_BLinkLength, file);
				Read(&sb.m_numClusters, file);

				Read(&sb.m_totalMass, file);
				Read(&sb.m_collisionMargin, file);

				Read(&sb.m_aeroModel, file);

				Read(&sb.m_VCF, file);
				Read(&sb.m_DP, file);
				Read(&sb.m_DG, file);
				Read(&sb.m_LF, file);
				Read(&sb.m_PR, file);
				Read(&sb.m_VC, file);
				Read(&sb.m_DF, file);
				Read(&sb.m_MT, file);
				Read(&sb.m_CHR, file);
				Read(&sb.m_KHR, file);
				Read(&sb.m_SHR, file);
				Read(&sb.m_AHR, file);

				Read(&sb.m_SRHR_CL, file);
				Read(&sb.m_SKHR_CL, file);
				Read(&sb.m_SSHR_CL, file);
				Read(&sb.m_SR_SPLT_CL, file);
				Read(&sb.m_SK_SPLT_CL, file);
				Read(&sb.m_SS_SPLT_CL, file);

				Read(&sb.m_V_IT, file);
				Read(&sb.m_P_IT, file);
				Read(&sb.m_D_IT, file);
				Read(&sb.m_C_IT, file);

				Read(&sb.m_LST, file);
				Read(&sb.m_AST, file);
				Read(&sb.m_VST, file);

				int32_t arCount;
				if (!Read(&arCount, file))
				{
					return false;
				}
				sb.m_anchorRigidbodies.resize(arCount);
				for (auto& ar : sb.m_anchorRigidbodies)
				{
					ReadIndex(&ar.m_rigidBodyIndex, pmx->m_header.m_rigidbodyIndexSize, file);
					ReadIndex(&ar.m_vertexIndex, pmx->m_header.m_vertexIndexSize, file);
					Read(&ar.m_nearMode, file);
				}

				int32_t pvCount;
				if (!Read(&pvCount, file))
				{
					return false;
				}
				sb.m_pinVertexIndices.resize(pvCount);
				for (auto& pv : sb.m_pinVertexIndices)
				{
					ReadIndex(&pv, pmx->m_header.m_vertexIndexSize, file);
				}
			}

			return !file.IsBad();
		}

		bool ReadPMXFile(PMXFile * pmxFile, File& file)
		{
			if (!ReadHeader(pmxFile, file))
			{
				SABA_ERROR("ReadHeader Fail.");
				return false;
			}

			if (!ReadInfo(pmxFile, file))
			{
				SABA_ERROR("ReadInfo Fail.");
				return false;
			}

			if (!ReadVertex(pmxFile, file))
			{
				SABA_ERROR("ReadVertex Fail.");
				return false;
			}

			if (!ReadFace(pmxFile, file))
			{
				SABA_ERROR("ReadFace Fail.");
				return false;
			}

			if (!ReadTexture(pmxFile, file))
			{
				SABA_ERROR("ReadTexture Fail.");
				return false;
			}

			if (!ReadMaterial(pmxFile, file))
			{
				SABA_ERROR("ReadMaterial Fail.");
				return false;
			}

			if (!ReadBone(pmxFile, file))
			{
				SABA_ERROR("ReadBone Fail.");
				return false;
			}

			if (!ReadMorph(pmxFile, file))
			{
				SABA_ERROR("ReadMorph Fail.");
				return false;
			}

			if (!ReadDisplayFrame(pmxFile, file))
			{
				SABA_ERROR("ReadDisplayFrame Fail.");
				return false;
			}

			if (!ReadRigidbody(pmxFile, file))
			{
				SABA_ERROR("ReadRigidbody Fail.");
				return false;
			}

			if (!ReadJoint(pmxFile, file))
			{
				SABA_ERROR("ReadJoint Fail.");
				return false;
			}

			if (file.Tell() < file.GetSize())
			{
				if (!ReadSoftbody(pmxFile, file))
				{
					SABA_ERROR("ReadSoftbody Fail.");
					return false;
				}
			}

			return true;
		}
	}

	bool ReadPMXFile(PMXFile * pmxFile, const char* filename)
	{
		File file;
		if (!file.Open(filename))
		{
			SABA_INFO("PMX File Open Fail. {}", filename);
			return false;
		}

		if (!ReadPMXFile(pmxFile, file))
		{
			SABA_INFO("PMX File Read Fail. {}", filename);
			return false;
		}
		SABA_INFO("PMX File Read Successed. {}", filename);

		return true;
	}


}
