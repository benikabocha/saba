#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define	STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

#include <Saba/Base/Path.h>
#include <Saba/Base/File.h>
#include <Saba/Base/UnicodeUtil.h>
#include <Saba/Base/Time.h>
#include <Saba/Model/MMD/PMDModel.h>
#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/Model/MMD/VMDFile.h>
#include <Saba/Model/MMD/VMDAnimation.h>
#include <Saba/Model/MMD/VMDCameraAnimation.h>

GLuint CreateShader(GLenum shaderType, const std::string code)
{
	GLuint shader = glCreateShader(shaderType);
	if (shader == 0)
	{
		std::cout << "Failed to create shader.\n";
		return 0;
	}
	const char* codes[] = { code.c_str() };
	GLint codesLen[] = { (GLint)code.size() };
	glShaderSource(shader, 1, codes, codesLen);
	glCompileShader(shader);

	GLint infoLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
	if (infoLength != 0)
	{
		std::vector<char> info;
		info.reserve(infoLength + 1);
		info.resize(infoLength);

		GLsizei len;
		glGetShaderInfoLog(shader, infoLength, &len, &info[0]);
		if (info[infoLength - 1] != '\0')
		{
			info.push_back('\0');
		}

		std::cout << &info[0] << "\n";
	}

	GLint compileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus != GL_TRUE)
	{
		glDeleteShader(shader);
		std::cout << "Failed to compile shader.\n";
		return 0;
	}

	return shader;
}

GLuint CreateShaderProgram(const std::string vsFile, const std::string fsFile)
{
	saba::TextFileReader vsFileText;
	if (!vsFileText.Open(vsFile))
	{
		std::cout << "Failed to open shader file. [" << vsFile << "].\n";
		return 0;
	}
	std::string vsCode = vsFileText.ReadAll();
	vsFileText.Close();

	saba::TextFileReader fsFileText;
	if (!fsFileText.Open(fsFile))
	{
		std::cout << "Failed to open shader file. [" << fsFile << "].\n";
		return 0;
	}
	std::string fsCode = fsFileText.ReadAll();
	fsFileText.Close();

	GLuint vs = CreateShader(GL_VERTEX_SHADER, vsCode);
	GLuint fs = CreateShader(GL_FRAGMENT_SHADER, fsCode);
	if (vs == 0 || fs == 0)
	{
		if (vs != 0) { glDeleteShader(vs); }
		if (fs != 0) { glDeleteShader(fs); }
		return 0;
	}

	GLuint prog = glCreateProgram();
	if (prog == 0)
	{
		glDeleteShader(vs);
		glDeleteShader(fs);
		std::cout << "Failed to create program.\n";
		return 0;
	}
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

	GLint infoLength;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLength);
	if (infoLength != 0)
	{
		std::vector<char> info;
		info.reserve(infoLength + 1);
		info.resize(infoLength);

		GLsizei len;
		glGetProgramInfoLog(prog, infoLength, &len, &info[0]);
		if (info[infoLength - 1] != '\0')
		{
			info.push_back('\0');
		}

		std::cout << &info[0] << "\n";
	}

	GLint linkStatus;
	glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE)
	{
		glDeleteShader(vs);
		glDeleteShader(fs);
		std::cout << "Failed to link shader.\n";
		return 0;
	}

	glDeleteShader(vs);
	glDeleteShader(fs);
	return prog;
}

struct AppContext;

struct Input
{
	std::string					m_modelPath;
	std::vector<std::string>	m_vmdPaths;
};

struct MMDShader
{
	~MMDShader()
	{
		Clear();
	}

	GLuint	m_prog = 0;

	// attribute
	GLint	m_inPos = -1;
	GLint	m_inNor = -1;
	GLint	m_inUV = -1;

	// uniform
	GLint	m_uWV = -1;
	GLint	m_uWVP = -1;

	GLint	m_uAmbinet = -1;
	GLint	m_uDiffuse = -1;
	GLint	m_uSpecular = -1;
	GLint	m_uSpecularPower = -1;
	GLint	m_uAlpha = -1;

	GLint	m_uTexMode = -1;
	GLint	m_uTex = -1;
	GLint	m_uTexMulFactor = -1;
	GLint	m_uTexAddFactor = -1;

	GLint	m_uSphereTexMode = -1;
	GLint	m_uSphereTex = -1;
	GLint	m_uSphereTexMulFactor = -1;
	GLint	m_uSphereTexAddFactor = -1;

	GLint	m_uToonTexMode = -1;
	GLint	m_uToonTex = -1;
	GLint	m_uToonTexMulFactor = -1;
	GLint	m_uToonTexAddFactor = -1;

	GLint	m_uLightColor = -1;
	GLint	m_uLightDir = -1;

	GLint	m_uLightVP = -1;
	GLint	m_uShadowMapSplitPositions = -1;
	GLint	m_uShadowMap0 = -1;
	GLint	m_uShadowMap1 = -1;
	GLint	m_uShadowMap2 = -1;
	GLint	m_uShadowMap3 = -1;
	GLint	m_uShadowMapEnabled = -1;

	bool Setup(const AppContext& appContext);
	void Clear();
};

struct MMDEdgeShader
{
	~MMDEdgeShader()
	{
		Clear();
	}

	GLuint	m_prog = 0;

	// attribute
	GLint	m_inPos = -1;
	GLint	m_inNor = -1;

	// uniform
	GLint	m_uWV = -1;
	GLint	m_uWVP = -1;
	GLint	m_uScreenSize = -1;
	GLint	m_uEdgeSize = -1;

	GLint	m_uEdgeColor = -1;

	bool Setup(const AppContext& appContext);
	void Clear();
};

struct MMDGroundShadowShader
{
	~MMDGroundShadowShader()
	{
		Clear();
	}

	GLuint	m_prog = 0;

	// attribute
	GLint	m_inPos = -1;

	// uniform
	GLint	m_uWVP = -1;
	GLint	m_uShadowColor = -1;

	bool Setup(const AppContext& appContext);
	void Clear();
};

struct Texture
{
	GLuint	m_texture;
	bool	m_hasAlpha;
};

struct AppContext
{
	~AppContext()
	{
		Clear();
	}

	std::string m_resourceDir;
	std::string	m_shaderDir;
	std::string	m_mmdDir;

	std::unique_ptr<MMDShader>				m_mmdShader;
	std::unique_ptr<MMDEdgeShader>			m_mmdEdgeShader;
	std::unique_ptr<MMDGroundShadowShader>	m_mmdGroundShadowShader;

	glm::mat4	m_viewMat;
	glm::mat4	m_projMat;
	int			m_screenWidth = 0;
	int			m_screenHeight = 0;

	glm::vec3	m_lightColor = glm::vec3(1, 1, 1);
	glm::vec3	m_lightDir = glm::vec3(-0.5f, -1.0f, -0.5f);

	std::map<std::string, Texture>	m_textures;
	GLuint	m_dummyColorTex = 0;
	GLuint	m_dummyShadowDepthTex = 0;

	float	m_animTime = 0.0f;
	std::unique_ptr<saba::VMDCameraAnimation>	m_vmdCameraAnim;

	bool Setup();
	void Clear();

	Texture GetTexture(const std::string& texturePath);
};

struct Material
{
	explicit Material(const saba::MMDMaterial& mat)
		: m_mmdMat(mat)
	{}

	const saba::MMDMaterial&	m_mmdMat;
	GLuint	m_texture = 0;
	bool	m_textureHasAlpha = false;
	GLuint	m_spTexture = 0;
	GLuint	m_toonTexture = 0;
};

struct Model
{
	std::shared_ptr<saba::MMDModel>	m_mmdModel;
	std::unique_ptr<saba::VMDAnimation>	m_vmdAnim;

	GLuint	m_posVBO = 0;
	GLuint	m_norVBO = 0;
	GLuint	m_uvVBO = 0;
	GLuint	m_ibo = 0;
	GLenum	m_indexType;

	GLuint	m_mmdVAO = 0;
	GLuint	m_mmdEdgeVAO = 0;
	GLuint	m_mmdGroundShadowVAO = 0;

	std::vector<Material>	m_materials;

	bool Setup(AppContext& appContext);
	void Clear();

	void Update(const AppContext& appContext);
	void Draw(const AppContext& appContext);
};

/*
	AppContext
*/

bool AppContext::Setup()
{
	// Setup resource directory.
	m_resourceDir = saba::PathUtil::GetExecutablePath();
	m_resourceDir = saba::PathUtil::GetDirectoryName(m_resourceDir);
	m_resourceDir = saba::PathUtil::Combine(m_resourceDir, "resource");
	m_shaderDir = saba::PathUtil::Combine(m_resourceDir, "shader");
	m_mmdDir = saba::PathUtil::Combine(m_resourceDir, "mmd");

	m_mmdShader = std::make_unique<MMDShader>();
	if (!m_mmdShader->Setup(*this))
	{
		return false;
	}

	m_mmdEdgeShader = std::make_unique<MMDEdgeShader>();
	if (!m_mmdEdgeShader->Setup(*this))
	{
		return false;
	}

	m_mmdGroundShadowShader = std::make_unique<MMDGroundShadowShader>();
	if (!m_mmdGroundShadowShader->Setup(*this))
	{
		return false;
	}

	glGenTextures(1, &m_dummyColorTex);
	glBindTexture(GL_TEXTURE_2D, m_dummyColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &m_dummyShadowDepthTex);
	glBindTexture(GL_TEXTURE_2D, m_dummyShadowDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

void AppContext::Clear()
{
	m_mmdShader.reset();
	m_mmdEdgeShader.reset();
	m_mmdGroundShadowShader.reset();

	for (auto& tex : m_textures)
	{
		glDeleteTextures(1, &tex.second.m_texture);
	}
	m_textures.clear();

	if (m_dummyColorTex != 0) { glDeleteTextures(1, &m_dummyColorTex); }
	if (m_dummyShadowDepthTex != 0) { glDeleteTextures(1, &m_dummyShadowDepthTex); }
	m_dummyColorTex = 0;
	m_dummyShadowDepthTex = 0;

	m_vmdCameraAnim.reset();
}

Texture AppContext::GetTexture(const std::string & texturePath)
{
	auto it = m_textures.find(texturePath);
	if (it == m_textures.end())
	{
		stbi_set_flip_vertically_on_load(true);
		saba::File file;
		if (!file.Open(texturePath))
		{
			return Texture{ 0, false };
		}
		int x, y, comp;
		int ret = stbi_info_from_file(file.GetFilePointer(), &x, &y, &comp);
		if (ret == 0)
		{
			return Texture{ 0, false };
		}

		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		int reqComp = 0;
		bool hasAlpha = false;
		if (comp != 4)
		{
			uint8_t* image = stbi_load_from_file(file.GetFilePointer(), &x, &y, &comp, STBI_rgb);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
			stbi_image_free(image);
			hasAlpha = false;
		}
		else
		{
			uint8_t* image = stbi_load_from_file(file.GetFilePointer(), &x, &y, &comp, STBI_rgb_alpha);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
			stbi_image_free(image);
			hasAlpha = true;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);

		m_textures[texturePath] = Texture{ tex, hasAlpha };

		return m_textures[texturePath];
	}
	else
	{
		return (*it).second;
	}
}

/*
	MMDShader
*/

bool MMDShader::Setup(const AppContext& appContext)
{
	m_prog = CreateShaderProgram(
		saba::PathUtil::Combine(appContext.m_shaderDir, "mmd.vert"),
		saba::PathUtil::Combine(appContext.m_shaderDir, "mmd.frag")
	);
	if (m_prog == 0)
	{
		return false;
	}

	// attribute
	m_inPos = glGetAttribLocation(m_prog, "in_Pos");
	m_inNor = glGetAttribLocation(m_prog, "in_Nor");
	m_inUV = glGetAttribLocation(m_prog, "in_UV");

	// uniform
	m_uWV = glGetUniformLocation(m_prog, "u_WV");
	m_uWVP = glGetUniformLocation(m_prog, "u_WVP");

	m_uAmbinet = glGetUniformLocation(m_prog, "u_Ambient");
	m_uDiffuse = glGetUniformLocation(m_prog, "u_Diffuse");
	m_uSpecular = glGetUniformLocation(m_prog, "u_Specular");
	m_uSpecularPower = glGetUniformLocation(m_prog, "u_SpecularPower");
	m_uAlpha = glGetUniformLocation(m_prog, "u_Alpha");

	m_uTexMode = glGetUniformLocation(m_prog, "u_TexMode");
	m_uTex = glGetUniformLocation(m_prog, "u_Tex");
	m_uTexMulFactor = glGetUniformLocation(m_prog, "u_TexMulFactor");
	m_uTexAddFactor = glGetUniformLocation(m_prog, "u_TexAddFactor");

	m_uSphereTexMode = glGetUniformLocation(m_prog, "u_SphereTexMode");
	m_uSphereTex = glGetUniformLocation(m_prog, "u_SphereTex");
	m_uSphereTexMulFactor = glGetUniformLocation(m_prog, "u_SphereTexMulFactor");
	m_uSphereTexAddFactor = glGetUniformLocation(m_prog, "u_SphereTexAddFactor");

	m_uToonTexMode = glGetUniformLocation(m_prog, "u_ToonTexMode");
	m_uToonTex = glGetUniformLocation(m_prog, "u_ToonTex");
	m_uToonTexMulFactor = glGetUniformLocation(m_prog, "u_ToonTexMulFactor");
	m_uToonTexAddFactor = glGetUniformLocation(m_prog, "u_ToonTexAddFactor");

	m_uLightColor = glGetUniformLocation(m_prog, "u_LightColor");
	m_uLightDir = glGetUniformLocation(m_prog, "u_LightDir");

	m_uLightVP = glGetUniformLocation(m_prog, "u_LightWVP");
	m_uShadowMapSplitPositions = glGetUniformLocation(m_prog, "u_ShadowMapSplitPositions");
	m_uShadowMap0 = glGetUniformLocation(m_prog, "u_ShadowMap0");
	m_uShadowMap1 = glGetUniformLocation(m_prog, "u_ShadowMap1");
	m_uShadowMap2 = glGetUniformLocation(m_prog, "u_ShadowMap2");
	m_uShadowMap3 = glGetUniformLocation(m_prog, "u_ShadowMap3");
	m_uShadowMapEnabled = glGetUniformLocation(m_prog, "u_ShadowMapEnabled");

	return true;
}

void MMDShader::Clear()
{
	if (m_prog != 0) { glDeleteProgram(m_prog); }
	m_prog = 0;
}

/*
	MMDEdgeShader
*/

bool MMDEdgeShader::Setup(const AppContext& appContext)
{
	m_prog = CreateShaderProgram(
		saba::PathUtil::Combine(appContext.m_shaderDir, "mmd_edge.vert"),
		saba::PathUtil::Combine(appContext.m_shaderDir, "mmd_edge.frag")
	);
	if (m_prog == 0)
	{
		return false;
	}

	// attribute
	m_inPos = glGetAttribLocation(m_prog, "in_Pos");
	m_inNor = glGetAttribLocation(m_prog, "in_Nor");

	// uniform
	m_uWV = glGetUniformLocation(m_prog, "u_WV");
	m_uWVP = glGetUniformLocation(m_prog, "u_WVP");
	m_uScreenSize = glGetUniformLocation(m_prog, "u_ScreenSize");
	m_uEdgeSize = glGetUniformLocation(m_prog, "u_EdgeSize");
	m_uEdgeColor = glGetUniformLocation(m_prog, "u_EdgeColor");

	return true;
}

void MMDEdgeShader::Clear()
{
	if (m_prog != 0) { glDeleteProgram(m_prog); }
	m_prog = 0;
}

/*
	MMDGroundShadowShader
*/

bool MMDGroundShadowShader::Setup(const AppContext& appContext)
{
	m_prog = CreateShaderProgram(
		saba::PathUtil::Combine(appContext.m_shaderDir, "mmd_ground_shadow.vert"),
		saba::PathUtil::Combine(appContext.m_shaderDir, "mmd_ground_shadow.frag")
	);
	if (m_prog == 0)
	{
		return false;
	}

	// attribute
	m_inPos = glGetAttribLocation(m_prog, "in_Pos");

	// uniform
	m_uWVP = glGetUniformLocation(m_prog, "u_WVP");
	m_uShadowColor = glGetUniformLocation(m_prog, "u_ShadowColor");

	return true;
}

void MMDGroundShadowShader::Clear()
{
	if (m_prog != 0) { glDeleteProgram(m_prog); }
	m_prog = 0;
}

/*
	Model
*/

bool Model::Setup(AppContext& appContext)
{
	if (m_mmdModel == nullptr)
	{
		return false;
	}

	// Setup vertices
	size_t vtxCount = m_mmdModel->GetVertexCount();
	glGenBuffers(1, &m_posVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vtxCount, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &m_norVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_norVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vtxCount, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &m_uvVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_uvVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * vtxCount, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	size_t idxSize = m_mmdModel->GetIndexElementSize();
	size_t idxCount = m_mmdModel->GetIndexCount();
	glGenBuffers(1, &m_ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxSize * idxCount, m_mmdModel->GetIndices(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	if (idxSize == 1)
	{
		m_indexType = GL_UNSIGNED_BYTE;
	}
	else if (idxSize == 2)
	{
		m_indexType = GL_UNSIGNED_SHORT;
	}
	else if (idxSize == 4)
	{
		m_indexType = GL_UNSIGNED_INT;
	}
	else
	{
		return false;
	}

	// Setup MMD VAO
	glGenVertexArrays(1, &m_mmdVAO);
	glBindVertexArray(m_mmdVAO);

	const auto& mmdShader = appContext.m_mmdShader;
	glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
	glVertexAttribPointer(mmdShader->m_inPos, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (const void*)0);
	glEnableVertexAttribArray(mmdShader->m_inPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_norVBO);
	glVertexAttribPointer(mmdShader->m_inNor, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (const void*)0);
	glEnableVertexAttribArray(mmdShader->m_inNor);

	glBindBuffer(GL_ARRAY_BUFFER, m_uvVBO);
	glVertexAttribPointer(mmdShader->m_inUV, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (const void*)0);
	glEnableVertexAttribArray(mmdShader->m_inUV);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

	glBindVertexArray(0);

	// Setup MMD Edge VAO
	glGenVertexArrays(1, &m_mmdEdgeVAO);
	glBindVertexArray(m_mmdEdgeVAO);

	const auto& mmdEdgeShader = appContext.m_mmdEdgeShader;
	glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
	glVertexAttribPointer(mmdEdgeShader->m_inPos, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (const void*)0);
	glEnableVertexAttribArray(mmdEdgeShader->m_inPos);

	glBindBuffer(GL_ARRAY_BUFFER, m_norVBO);
	glVertexAttribPointer(mmdEdgeShader->m_inNor, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (const void*)0);
	glEnableVertexAttribArray(mmdEdgeShader->m_inNor);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

	glBindVertexArray(0);

	// Setup MMD Ground Shadow VAO
	glGenVertexArrays(1, &m_mmdGroundShadowVAO);
	glBindVertexArray(m_mmdGroundShadowVAO);

	const auto& mmdGroundShadowShader = appContext.m_mmdGroundShadowShader;
	glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
	glVertexAttribPointer(mmdGroundShadowShader->m_inPos, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (const void*)0);
	glEnableVertexAttribArray(mmdGroundShadowShader->m_inPos);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

	glBindVertexArray(0);

	// Setup materials
	for (size_t i = 0; i < m_mmdModel->GetMaterialCount(); i++)
	{
		const auto& mmdMat = m_mmdModel->GetMaterials()[i];
		Material mat(mmdMat);
		if (!mmdMat.m_texture.empty())
		{
			auto tex = appContext.GetTexture(mmdMat.m_texture);
			mat.m_texture = tex.m_texture;
			mat.m_textureHasAlpha = tex.m_hasAlpha;
		}
		if (!mmdMat.m_spTexture.empty())
		{
			auto tex = appContext.GetTexture(mmdMat.m_spTexture);
			mat.m_spTexture = tex.m_texture;
		}
		if (!mmdMat.m_toonTexture.empty())
		{
			auto tex = appContext.GetTexture(mmdMat.m_toonTexture);
			mat.m_toonTexture = tex.m_texture;
		}
		m_materials.emplace_back(std::move(mat));
	}

	return true;
}

void Model::Clear()
{
	if (m_posVBO != 0) { glDeleteBuffers(1, &m_posVBO); }
	if (m_norVBO != 0) { glDeleteBuffers(1, &m_norVBO); }
	if (m_uvVBO != 0) { glDeleteBuffers(1, &m_uvVBO); }
	if (m_ibo != 0) { glDeleteBuffers(1, &m_ibo); }
	m_posVBO = 0;
	m_norVBO = 0;
	m_uvVBO = 0;
	m_ibo = 0;

	if (m_mmdVAO != 0) { glDeleteVertexArrays(1, &m_mmdVAO); }
	if (m_mmdEdgeVAO != 0) { glDeleteVertexArrays(1, &m_mmdEdgeVAO); }
	if (m_mmdGroundShadowVAO != 0) { glDeleteVertexArrays(1, &m_mmdGroundShadowVAO); }
	m_mmdVAO = 0;
	m_mmdEdgeVAO = 0;
	m_mmdGroundShadowVAO = 0;
}

void Model::Update(const AppContext& appContext)
{
	m_mmdModel->Update();

	size_t vtxCount = m_mmdModel->GetVertexCount();
	glBindBuffer(GL_ARRAY_BUFFER, m_posVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * vtxCount, m_mmdModel->GetUpdatePositions());
	glBindBuffer(GL_ARRAY_BUFFER, m_norVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * vtxCount, m_mmdModel->GetUpdateNormals());
	glBindBuffer(GL_ARRAY_BUFFER, m_uvVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * vtxCount, m_mmdModel->GetUpdateUVs());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Model::Draw(const AppContext& appContext)
{
	const auto& view = appContext.m_viewMat;
	const auto& proj = appContext.m_projMat;

	auto world = glm::mat4(1.0f);
	auto wv = view * world;
	auto wvp = proj * view * world;
	auto wvit = glm::mat3(view * world);
	wvit = glm::inverse(wvit);
	wvit = glm::transpose(wvit);

	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, appContext.m_dummyShadowDepthTex);
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, appContext.m_dummyShadowDepthTex);
	glActiveTexture(GL_TEXTURE0 + 5);
	glBindTexture(GL_TEXTURE_2D, appContext.m_dummyShadowDepthTex);
	glActiveTexture(GL_TEXTURE0 + 6);
	glBindTexture(GL_TEXTURE_2D, appContext.m_dummyShadowDepthTex);

	glEnable(GL_DEPTH_TEST);

	// Draw model
	size_t subMeshCount = m_mmdModel->GetSubMeshCount();
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		const auto& shader = appContext.m_mmdShader;
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (mat.m_mmdMat.m_alpha == 0)
		{
			continue;
		}

		glUseProgram(shader->m_prog);
		glBindVertexArray(m_mmdVAO);

		glUniformMatrix4fv(shader->m_uWV, 1, GL_FALSE, &wv[0][0]);
		glUniformMatrix4fv(shader->m_uWVP, 1, GL_FALSE, &wvp[0][0]);

		bool alphaBlend = true;

		glUniform3fv(shader->m_uAmbinet, 1, &mmdMat.m_ambient[0]);
		glUniform3fv(shader->m_uDiffuse, 1, &mmdMat.m_diffuse[0]);
		glUniform3fv(shader->m_uSpecular, 1, &mmdMat.m_specular[0]);
		glUniform1f(shader->m_uSpecularPower, mmdMat.m_specularPower);
		glUniform1f(shader->m_uAlpha, mmdMat.m_alpha);

		glActiveTexture(GL_TEXTURE0 + 0);
		glUniform1i(shader->m_uTex, 0);
		if (mat.m_texture != 0)
		{
			if (!mat.m_textureHasAlpha)
			{
				// Use Material Alpha
				glUniform1i(shader->m_uTexMode, 1);
			}
			else
			{
				// Use Material Alpha * Texture Alpha
				glUniform1i(shader->m_uTexMode, 2);
			}
			glUniform4fv(shader->m_uTexMulFactor, 1, &mmdMat.m_textureMulFactor[0]);
			glUniform4fv(shader->m_uTexAddFactor, 1, &mmdMat.m_textureAddFactor[0]);
			glBindTexture(GL_TEXTURE_2D, mat.m_texture);
		}
		else
		{
			glUniform1i(shader->m_uTexMode, 0);
			glBindTexture(GL_TEXTURE_2D, appContext.m_dummyColorTex);
		}

		glActiveTexture(GL_TEXTURE0 + 1);
		glUniform1i(shader->m_uSphereTex, 1);
		if (mat.m_spTexture != 0)
		{
			if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Mul)
			{
				glUniform1i(shader->m_uSphereTexMode, 1);
			}
			else if (mmdMat.m_spTextureMode == saba::MMDMaterial::SphereTextureMode::Add)
			{
				glUniform1i(shader->m_uSphereTexMode, 2);
			}
			glUniform4fv(shader->m_uSphereTexMulFactor, 1, &mmdMat.m_spTextureMulFactor[0]);
			glUniform4fv(shader->m_uSphereTexAddFactor, 1, &mmdMat.m_spTextureAddFactor[0]);
			glBindTexture(GL_TEXTURE_2D, mat.m_spTexture);
		}
		else
		{
			glUniform1i(shader->m_uSphereTexMode, 0);
			glBindTexture(GL_TEXTURE_2D, appContext.m_dummyColorTex);
		}

		glActiveTexture(GL_TEXTURE0 + 2);
		glUniform1i(shader->m_uToonTex, 2);
		if (mat.m_toonTexture != 0)
		{
			glUniform4fv(shader->m_uToonTexMulFactor, 1, &mmdMat.m_toonTextureMulFactor[0]);
			glUniform4fv(shader->m_uToonTexAddFactor, 1, &mmdMat.m_toonTextureAddFactor[0]);
			glUniform1i(shader->m_uToonTexMode, 1);
			glBindTexture(GL_TEXTURE_2D, mat.m_toonTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		else
		{
			glUniform1i(shader->m_uToonTexMode, 0);
			glBindTexture(GL_TEXTURE_2D, appContext.m_dummyColorTex);
		}

		glm::vec3 lightColor = appContext.m_lightColor;
		glm::vec3 lightDir = appContext.m_lightDir;
		glm::mat3 viewMat = glm::mat3(appContext.m_viewMat);
		lightDir = viewMat * lightDir;
		glUniform3fv(shader->m_uLightDir, 1, &lightDir[0]);
		glUniform3fv(shader->m_uLightColor, 1, &lightColor[0]);

		if (mmdMat.m_bothFace)
		{
			glDisable(GL_CULL_FACE);
		}
		else
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
		}

		if (alphaBlend)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		glUniform1i(shader->m_uShadowMapEnabled, 0);
		glUniform1i(shader->m_uShadowMap0, 3);
		glUniform1i(shader->m_uShadowMap1, 4);
		glUniform1i(shader->m_uShadowMap2, 5);
		glUniform1i(shader->m_uShadowMap3, 6);

		size_t offset = subMesh.m_beginIndex * m_mmdModel->GetIndexElementSize();
		glDrawElements(GL_TRIANGLES, subMesh.m_vertexCount, m_indexType, (GLvoid*)offset);

		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glUseProgram(0);
	}

	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0 + 5);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0 + 6);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Draw edge
	glm::vec2 screenSize(appContext.m_screenWidth, appContext.m_screenHeight);
	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		int matID = subMesh.m_materialID;
		const auto& shader = appContext.m_mmdEdgeShader;
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;

		if (!mmdMat.m_edgeFlag)
		{
			continue;
		}
		if (mmdMat.m_alpha == 0.0f)
		{
			continue;
		}

		glUseProgram(shader->m_prog);
		glBindVertexArray(m_mmdEdgeVAO);

		glUniformMatrix4fv(shader->m_uWV, 1, GL_FALSE, &wv[0][0]);
		glUniformMatrix4fv(shader->m_uWVP, 1, GL_FALSE, &wvp[0][0]);
		glUniform2fv(shader->m_uScreenSize, 1, &screenSize[0]);
		glUniform1f(shader->m_uEdgeSize, mmdMat.m_edgeSize);
		glUniform4fv(shader->m_uEdgeColor, 1, &mmdMat.m_edgeColor[0]);

		bool alphaBlend = true;

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		if (alphaBlend)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		size_t offset = subMesh.m_beginIndex * m_mmdModel->GetIndexElementSize();
		glDrawElements(GL_TRIANGLES, subMesh.m_vertexCount, m_indexType, (GLvoid*)offset);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	// Draw ground shadow
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -1);
	auto plane = glm::vec4(0, 1, 0, 0);
	auto light = -appContext.m_lightDir;
	auto shadow = glm::mat4();

	shadow[0][0] = plane.y * light.y + plane.z * light.z;
	shadow[0][1] = -plane.x * light.y;
	shadow[0][2] = -plane.x * light.z;
	shadow[0][3] = 0;

	shadow[1][0] = -plane.y * light.x;
	shadow[1][1] = plane.x * light.x + plane.z * light.z;
	shadow[1][2] = -plane.y * light.z;
	shadow[1][3] = 0;

	shadow[2][0] = -plane.z * light.x;
	shadow[2][1] = -plane.z * light.y;
	shadow[2][2] = plane.x * light.x + plane.y * light.y;
	shadow[2][3] = 0;

	shadow[3][0] = -plane.w * light.x;
	shadow[3][1] = -plane.w * light.y;
	shadow[3][2] = -plane.w * light.z;
	shadow[3][3] = plane.x * light.x + plane.y * light.y + plane.z * light.z;

	auto wsvp = proj * view * shadow * world;

	auto shadowColor = glm::vec4(0.4f, 0.2f, 0.2f, 0.7f);
	if (shadowColor.a < 1.0f)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_NOTEQUAL, 1, 1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glEnable(GL_STENCIL_TEST);
	}
	else
	{
		glDisable(GL_BLEND);
	}
	glDisable(GL_CULL_FACE);

	for (size_t i = 0; i < subMeshCount; i++)
	{
		const auto& subMesh = m_mmdModel->GetSubMeshes()[i];
		int matID = subMesh.m_materialID;
		const auto& mat = m_materials[subMesh.m_materialID];
		const auto& mmdMat = mat.m_mmdMat;
		const auto& shader = appContext.m_mmdGroundShadowShader;

		if (!mmdMat.m_groundShadow)
		{
			continue;
		}

		glUseProgram(shader->m_prog);
		glBindVertexArray(m_mmdGroundShadowVAO);

		glUniformMatrix4fv(shader->m_uWVP, 1, GL_FALSE, &wsvp[0][0]);
		glUniform4fv(shader->m_uShadowColor, 1, &shadowColor[0]);

		size_t offset = subMesh.m_beginIndex * m_mmdModel->GetIndexElementSize();
		glDrawElements(GL_TRIANGLES, subMesh.m_vertexCount, m_indexType, (GLvoid*)offset);

		glBindVertexArray(0);
		glUseProgram(0);
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
}

void Usage()
{
	std::cout << "app [-model <pmd|pmx file path>] [-vmd <vmd file path>]\n";
	std::cout << "e.g. app -model model1.pmx -vmd anim1_1.vmd -vmd anim1_2.vmd  -model model2.pmx\n";
}

bool SampleMain(std::vector<std::string>& args)
{
	Input currentInput;
	std::vector<Input> inputModels;
	for (auto argIt = args.begin(); argIt != args.end(); ++argIt)
	{
		const auto& arg = (*argIt);
		if (arg == "-model")
		{
			if (!currentInput.m_modelPath.empty())
			{
				inputModels.emplace_back(currentInput);
			}

			++argIt;
			if (argIt == args.end())
			{
				Usage();
				return false;
			}
			currentInput.m_modelPath = (*argIt);
		}
		else if (arg == "-vmd")
		{
			if (currentInput.m_modelPath.empty())
			{
				Usage();
				return false;
			}

			++argIt;
			if (argIt == args.end())
			{
				Usage();
				return false;
			}
			currentInput.m_vmdPaths.push_back((*argIt));
		}
	}
	if (!currentInput.m_modelPath.empty())
	{
		inputModels.emplace_back(currentInput);
	}

	// Initialize glfw
	if (!glfwInit())
	{
		return false;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	auto window = glfwCreateWindow(1280, 800, "simple mmd viewer", nullptr, nullptr);
	if (window == nullptr)
	{
		return false;
	}

	glfwMakeContextCurrent(window);

	if (gl3wInit() != 0)
	{
		return false;
	}

	glfwSwapInterval(0);
	glEnable(GL_MULTISAMPLE);

	// Initialize application
	AppContext appContext;
	if (!appContext.Setup())
	{
		std::cout << "Failed to setup AppContext.\n";
		return false;
	}

	// Load MMD model
	std::vector<Model> models;
	for (const auto& input : inputModels)
	{
		// Load MMD model
		Model model;
		auto ext = saba::PathUtil::GetExt(input.m_modelPath);
		if (ext == "pmd")
		{
			auto pmdModel = std::make_unique<saba::PMDModel>();
			if (!pmdModel->Load(input.m_modelPath, appContext.m_mmdDir))
			{
				std::cout << "Failed to load pmd file.\n";
				return false;
			}
			model.m_mmdModel = std::move(pmdModel);
		}
		else if (ext == "pmx")
		{
			auto pmxModel = std::make_unique<saba::PMXModel>();
			if (!pmxModel->Load(input.m_modelPath, appContext.m_mmdDir))
			{
				std::cout << "Failed to load pmx file.\n";
				return false;
			}
			model.m_mmdModel = std::move(pmxModel);
		}
		else
		{
			std::cout << "Unknown file type. [" << ext << "]\n";
			return false;
		}
		model.m_mmdModel->InitializeAnimation();

		// Load VMD animation
		auto vmdAnim = std::make_unique<saba::VMDAnimation>();
		if (!vmdAnim->Create(model.m_mmdModel))
		{
			std::cout << "Failed to create VMDAnimation.\n";
			return false;
		}
		for (const auto& vmdPath : input.m_vmdPaths)
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
			if (!vmdFile.m_cameras.empty())
			{
				auto vmdCamAnim = std::make_unique<saba::VMDCameraAnimation>();
				if (!vmdCamAnim->Create(vmdFile))
				{
					std::cout << "Failed to create VMDCameraAnimation.\n";
				}
				appContext.m_vmdCameraAnim = std::move(vmdCamAnim);
			}
		}
		vmdAnim->SyncPhysics(0.0f);

		model.m_vmdAnim = std::move(vmdAnim);

		model.Setup(appContext);

		models.emplace_back(std::move(model));
	}

	double fpsTime = saba::GetTime();
	int fpsFrame = 0;
	double saveTime = saba::GetTime();
	while (!glfwWindowShouldClose(window))
	{
		double time = saba::GetTime();
		double elapsed = time - saveTime;
		if (elapsed > 1.0 / 30.0)
		{
			elapsed = 1.0 / 30.0;
		}
		saveTime = time;
		appContext.m_animTime += float(elapsed);

		glClearColor(1.0f, 0.8f, 0.75f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		appContext.m_screenWidth = width;
		appContext.m_screenHeight = height;

		glViewport(0, 0, width, height);

		// Setup camera
		if (appContext.m_vmdCameraAnim)
		{
			appContext.m_vmdCameraAnim->Evaluate(appContext.m_animTime * 30.0f);
			const auto mmdCam = appContext.m_vmdCameraAnim->GetCamera();
			saba::MMDLookAtCamera lookAtCam(mmdCam);
			appContext.m_viewMat = glm::lookAt(lookAtCam.m_eye, lookAtCam.m_center, lookAtCam.m_up);
			appContext.m_projMat = glm::perspectiveFovRH(mmdCam.m_fov, float(width), float(height), 1.0f, 10000.0f);
		}
		else
		{
			appContext.m_viewMat = glm::lookAt(glm::vec3(0, 10, 50), glm::vec3(0, 10, 0), glm::vec3(0, 1, 0));
			appContext.m_projMat = glm::perspectiveFovRH(glm::radians(30.0f), float(width), float(height), 1.0f, 10000.0f);
		}

		for (auto& model : models)
		{
			// Update bone animation.
			model.m_mmdModel->BeginAnimation();
			model.m_vmdAnim->Evaluate(appContext.m_animTime * 30.0f);
			model.m_mmdModel->UpdateAnimation();
			model.m_mmdModel->EndAnimation();

			// Update physics animation.
			model.m_mmdModel->UpdatePhysics(float(elapsed));

			// Draw
			model.Update(appContext);
			model.Draw(appContext);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();

		//FPS
		{
			fpsFrame++;
			double time = saba::GetTime();
			double deltaTime = time - fpsTime;
			if (deltaTime > 1.0)
			{
				double fps = double(fpsFrame) / deltaTime;
				std::cout << fps << " fps\n";
				fpsFrame = 0;
				fpsTime = time;
			}
		}
	}

	appContext.Clear();

	glfwTerminate();

	return true;
}

#if _WIN32
int wmain(int argc, wchar_t** argv)
{
	if (argc < 2)
	{
		Usage();
		return 1;
	}

	std::vector<std::string> args;
	for (int i = 1; i < argc; i++)
	{
		args.emplace_back(saba::ToUtf8String(argv[i]));
	}

	if (!SampleMain(args))
	{
		std::cout << "Failed to run.\n";
		return 1;
	}

	return 0;
}
#else // _WIN32
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		Usage();
		return 1;
	}

	std::vector<std::string> args;
	for (int i = 1; i < argc; i++)
	{
		args.emplace_back(argv[i]);
	}

	if (!SampleMain(args))
	{
		std::cout << "Failed to run.\n";
		return 1;
	}

	return 0;
}
#endif // _WIN32
