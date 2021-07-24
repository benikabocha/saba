//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <Saba/Base/UnicodeUtil.h>
#include <Saba/Base/Path.h>
#include <Saba/Model/MMD/MMDModel.h>
#include <Saba/Model/MMD/PMDModel.h>
#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/Model/MMD/VMDFile.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <Saba/Model/MMD/VPDFile.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

void Usage()
{
	std::cout << "mmd2obj <pmd/pmx file> [-vmd <vmd file>] [-t <animation time (sec)>] [-vpd <vpd file>]\n";
}

bool MMD2Obj(const std::vector<std::string>& args)
{
	if (args.size() <= 1)
	{
		Usage();
		return false;
	}

	// Analyze commad line.
	const std::string& modelPath = args[1];
	std::vector<std::string> vmdPaths;
	std::string vpdPath;
	double	animTime = 0.0;

	for (size_t i = 2; i < args.size(); i++)
	{
		if (args[i] == "-vmd")
		{
			i++;
			if (i < args.size())
			{
				vmdPaths.push_back(args[i]);
			}
			else
			{
				Usage();
				return false;
			}
		}
		else if (args[i] == "-t")
		{
			i++;
			if (i < args.size())
			{
				animTime = std::stod(args[i]);
			}
			else
			{
				Usage();
				return false;
			}
		}
		else if (args[i] == "-vpd")
		{
			i++;
			if (i < args.size())
			{
				vpdPath = args[i];
			}
			else
			{
				Usage();
				return false;
			}
		}
		else
		{
			Usage();
			return false;
		}
	}

	// Load model.
	std::shared_ptr<saba::MMDModel> mmdModel;
	std::string mmdDataPath = "";	// Set MMD data path(default toon texture path).
	std::string ext = saba::PathUtil::GetExt(modelPath);
	if (ext == "pmd")
	{
		auto pmdModel = std::make_unique<saba::PMDModel>();
		if (!pmdModel->Load(modelPath, mmdDataPath))
		{
			std::cout << "Failed to load PMDModel.\n";
			return false;
		}
		mmdModel = std::move(pmdModel);
	}
	else if (ext == "pmx")
	{
		auto pmxModel = std::make_unique<saba::PMXModel>();
		if (!pmxModel->Load(modelPath, mmdDataPath))
		{
			std::cout << "Failed to load PMXModel.\n";
			return false;
		}
		mmdModel = std::move(pmxModel);
	}
	else
	{
		std::cout << "Unsupported Model Ext : " << ext << "\n";
		return false;
	}

	// Load animation.
	bool useVMDAnimation = !vmdPaths.empty();
	auto vmdAnim = std::make_unique<saba::VMDAnimation>();
	if (!vmdAnim->Create(mmdModel))
	{
		std::cout << "Failed to create VMDAnimation.\n";
		return false;
	}
	for (const auto& vmdPath : vmdPaths)
	{
		saba::VMDFile vmdFile;
		if (!saba::ReadVMDFile(&vmdFile, vmdPath.c_str()))
		{
			std::cout << "Failed to read VMD file.\n";
			return false;
		}
		if (!vmdAnim->Add(vmdFile))
		{
			std::cout << "Failed to add VMDAnimation.\n";
			return false;
		}
	}

	// Load pose.
	saba::VPDFile vpdFile;
	if (!vpdPath.empty())
	{
		if (!vmdPaths.empty())
		{
			std::cout << "Warning : Canceled to read VPD file.";
		}
		else
		{
			if (!saba::ReadVPDFile(&vpdFile, vpdPath.c_str()))
			{
				std::cout << "Failed to read VPD file.\n";
				return false;
			}

		}
	}

	// Initialize pose.
	{
		// Sync physics animation.
		mmdModel->InitializeAnimation();
		if (useVMDAnimation)
		{
			vmdAnim->SyncPhysics((float)animTime * 30.0f);
		}
		else
		{
			mmdModel->LoadPose(vpdFile);
		}
	}

	// Update animation(animation loop).
	{
		// Update animation.
		mmdModel->BeginAnimation();
		if (useVMDAnimation)
		{
			mmdModel->UpdateAllAnimation(vmdAnim.get(), (float)animTime * 30.0f, 1.0f / 60.0f);
		}
		else
		{
			mmdModel->UpdateAllAnimation(nullptr, 0, 1.0f / 60.0f);
		}
		mmdModel->EndAnimation();

		// Update vertices.
		mmdModel->Update();
	}

	// Output OBJ file.
	std::ofstream objFile;
	objFile.open("output.obj");
	if (!objFile.is_open())
	{
		std::cout << "Failed to open OBJ file.\n";
		return false;
	}
	objFile << "# mmmd2obj\n";
	objFile << "mtllib output.mtl\n";

	// Write positions.
	size_t vtxCount = mmdModel->GetVertexCount();
	const glm::vec3* positions = mmdModel->GetUpdatePositions();
	for (size_t i = 0; i < vtxCount; i++)
	{
		objFile << "v " << positions[i].x << " " << positions[i].y << " " << positions[i].z << "\n";
	}
	const glm::vec3* normals = mmdModel->GetUpdateNormals();
	for (size_t i = 0; i < vtxCount; i++)
	{
		objFile << "vn " << normals[i].x << " " << normals[i].y << " " << normals[i].z << "\n";
	}
	const glm::vec2* uvs = mmdModel->GetUpdateUVs();
	for (size_t i = 0; i < vtxCount; i++)
	{
		objFile << "vt " << uvs[i].x << " " << uvs[i].y << "\n";
	}

	// Copy vertex indices.
	std::vector<size_t> indices(mmdModel->GetIndexCount());
	if (mmdModel->GetIndexElementSize() == 1)
	{
		uint8_t* mmdIndices = (uint8_t*)mmdModel->GetIndices();
		for (size_t i = 0; i < indices.size(); i++)
		{
			indices[i] = mmdIndices[i];
		}
	}
	else if (mmdModel->GetIndexElementSize() == 2)
	{
		uint16_t* mmdIndices = (uint16_t*)mmdModel->GetIndices();
		for (size_t i = 0; i < indices.size(); i++)
		{
			indices[i] = mmdIndices[i];
		}
	}
	else if (mmdModel->GetIndexElementSize() == 4)
	{
		uint32_t* mmdIndices = (uint32_t*)mmdModel->GetIndices();
		for (size_t i = 0; i < indices.size(); i++)
		{
			indices[i] = mmdIndices[i];
		}
	}
	else
	{
		return false;
	}

	// Write faces.
	size_t subMeshCount = mmdModel->GetSubMeshCount();
	const saba::MMDSubMesh* subMeshes = mmdModel->GetSubMeshes();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		objFile << "\n";
		objFile << "usemtl " << subMeshes[i].m_materialID << "\n";

		for (size_t j = 0; j < subMeshes[i].m_vertexCount; j += 3)
		{
			auto vtxIdx = subMeshes[i].m_beginIndex + j;
			auto vi0 = indices[vtxIdx + 0] + 1;
			auto vi1 = indices[vtxIdx + 1] + 1;
			auto vi2 = indices[vtxIdx + 2] + 1;
			objFile << "f "
				<< vi0 << "/" << vi0 << "/" << vi0 << " "
				<< vi1 << "/" << vi1 << "/" << vi1 << " "
				<< vi2 << "/" << vi2 << "/" << vi2 << "\n";
		}
	}
	objFile.close();

	// Write materials.
	std::ofstream mtlFile;
	mtlFile.open("output.mtl");
	if (!mtlFile.is_open())
	{
		std::cout << "Failed to open MTL file.\n";
		return false;
	}

	objFile << "# mmmd2obj\n";
	size_t materialCount = mmdModel->GetMaterialCount();
	const saba::MMDMaterial* materials = mmdModel->GetMaterials();
	for (size_t i = 0; i < materialCount; i++)
	{
		const auto& m = materials[i];
		mtlFile << "newmtl " << i << "\n";

		mtlFile << "Ka " << m.m_ambient.r << " " << m.m_ambient.g << " " << m.m_ambient.b << "\n";
		mtlFile << "Kd " << m.m_diffuse.r << " " << m.m_diffuse.g << " " << m.m_diffuse.b << "\n";
		mtlFile << "Ks " << m.m_specular.r << " " << m.m_specular.g << " " << m.m_specular.b << "\n";
		mtlFile << "d " << m.m_alpha << "\n";
		mtlFile << "map_Kd " << saba::PathUtil::GetFilename(m.m_texture) << "\n";
		mtlFile << "\n";
	}
	mtlFile.close();

	return true;
}

#if _WIN32
#include <Windows.h>
#include <shellapi.h>
#endif

int main(int argc, char** argv)
{
	std::vector<std::string> args(argc);
#if _WIN32
	{
		WCHAR* cmdline = GetCommandLineW();
		int wArgc;
		WCHAR** wArgs = CommandLineToArgvW(cmdline, &wArgc);
		for (int i = 0; i < argc; i++)
		{
			args[i] = saba::ToUtf8String(wArgs[i]);
		}
	}
#else // _WIN32
	for (int i = 0; i < argc; i++)
	{
		args[i] = argv[i];
	}
#endif

	if (!MMD2Obj(args))
	{
		std::cout << "Failed to convert model data.\n";
		return 1;
	}

	return 0;
}
