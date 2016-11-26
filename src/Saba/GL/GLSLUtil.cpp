#include "GLSLUtil.h"
#include "GLShaderUtil.h"
#include "../Base/Path.h"
#include "../Base/File.h"
#include "../Base/Singleton.h"
#include "../Base/Log.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include <glslang/Public/ShaderLang.h>

namespace saba
{
	namespace
	{
		class GLSLangInitializer
		{
		public:
			GLSLangInitializer()
			{
				m_initialized = glslang::InitializeProcess();
			}

			~GLSLangInitializer()
			{
				glslang::FinalizeProcess();
			}

			bool IsInitialized() const { return m_initialized; }

		private:
			bool m_initialized;
		};
	}
	void GLSLDefine::Define(const std::string & def, const std::string & defValue)
	{
		m_defines[def] = defValue;
	}

	void GLSLDefine::Undefine(const std::string & def)
	{
		m_defines.erase(def);
	}

	void GLSLDefine::Clear()
	{
		m_defines.clear();
	}

	GLSLInclude::GLSLInclude(const std::string & workDir)
		: m_workDir(workDir)
	{
	}

	void GLSLInclude::AddInclude(const std::string & include)
	{
		m_pathList.push_back(include);
	}

	void GLSLInclude::Clear()
	{
		m_pathList.clear();
	}


	bool InitializeGLSLUtil()
	{
		return glslang::InitializeProcess();;
	}

	void UninitializeGLSLUtil()
	{
		glslang::FinalizeProcess();
	}

	namespace
	{
		const TBuiltInResource DefaultTBuiltInResource = {
			/* .MaxLights = */ 32,
			/* .MaxClipPlanes = */ 6,
			/* .MaxTextureUnits = */ 32,
			/* .MaxTextureCoords = */ 32,
			/* .MaxVertexAttribs = */ 64,
			/* .MaxVertexUniformComponents = */ 4096,
			/* .MaxVaryingFloats = */ 64,
			/* .MaxVertexTextureImageUnits = */ 32,
			/* .MaxCombinedTextureImageUnits = */ 80,
			/* .MaxTextureImageUnits = */ 32,
			/* .MaxFragmentUniformComponents = */ 4096,
			/* .MaxDrawBuffers = */ 32,
			/* .MaxVertexUniformVectors = */ 128,
			/* .MaxVaryingVectors = */ 8,
			/* .MaxFragmentUniformVectors = */ 16,
			/* .MaxVertexOutputVectors = */ 16,
			/* .MaxFragmentInputVectors = */ 15,
			/* .MinProgramTexelOffset = */ -8,
			/* .MaxProgramTexelOffset = */ 7,
			/* .MaxClipDistances = */ 8,
			/* .MaxComputeWorkGroupCountX = */ 65535,
			/* .MaxComputeWorkGroupCountY = */ 65535,
			/* .MaxComputeWorkGroupCountZ = */ 65535,
			/* .MaxComputeWorkGroupSizeX = */ 1024,
			/* .MaxComputeWorkGroupSizeY = */ 1024,
			/* .MaxComputeWorkGroupSizeZ = */ 64,
			/* .MaxComputeUniformComponents = */ 1024,
			/* .MaxComputeTextureImageUnits = */ 16,
			/* .MaxComputeImageUniforms = */ 8,
			/* .MaxComputeAtomicCounters = */ 8,
			/* .MaxComputeAtomicCounterBuffers = */ 1,
			/* .MaxVaryingComponents = */ 60,
			/* .MaxVertexOutputComponents = */ 64,
			/* .MaxGeometryInputComponents = */ 64,
			/* .MaxGeometryOutputComponents = */ 128,
			/* .MaxFragmentInputComponents = */ 128,
			/* .MaxImageUnits = */ 8,
			/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
			/* .MaxCombinedShaderOutputResources = */ 8,
			/* .MaxImageSamples = */ 0,
			/* .MaxVertexImageUniforms = */ 0,
			/* .MaxTessControlImageUniforms = */ 0,
			/* .MaxTessEvaluationImageUniforms = */ 0,
			/* .MaxGeometryImageUniforms = */ 0,
			/* .MaxFragmentImageUniforms = */ 8,
			/* .MaxCombinedImageUniforms = */ 8,
			/* .MaxGeometryTextureImageUnits = */ 16,
			/* .MaxGeometryOutputVertices = */ 256,
			/* .MaxGeometryTotalOutputComponents = */ 1024,
			/* .MaxGeometryUniformComponents = */ 1024,
			/* .MaxGeometryVaryingComponents = */ 64,
			/* .MaxTessControlInputComponents = */ 128,
			/* .MaxTessControlOutputComponents = */ 128,
			/* .MaxTessControlTextureImageUnits = */ 16,
			/* .MaxTessControlUniformComponents = */ 1024,
			/* .MaxTessControlTotalOutputComponents = */ 4096,
			/* .MaxTessEvaluationInputComponents = */ 128,
			/* .MaxTessEvaluationOutputComponents = */ 128,
			/* .MaxTessEvaluationTextureImageUnits = */ 16,
			/* .MaxTessEvaluationUniformComponents = */ 1024,
			/* .MaxTessPatchComponents = */ 120,
			/* .MaxPatchVertices = */ 32,
			/* .MaxTessGenLevel = */ 64,
			/* .MaxViewports = */ 16,
			/* .MaxVertexAtomicCounters = */ 0,
			/* .MaxTessControlAtomicCounters = */ 0,
			/* .MaxTessEvaluationAtomicCounters = */ 0,
			/* .MaxGeometryAtomicCounters = */ 0,
			/* .MaxFragmentAtomicCounters = */ 8,
			/* .MaxCombinedAtomicCounters = */ 8,
			/* .MaxAtomicCounterBindings = */ 1,
			/* .MaxVertexAtomicCounterBuffers = */ 0,
			/* .MaxTessControlAtomicCounterBuffers = */ 0,
			/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
			/* .MaxGeometryAtomicCounterBuffers = */ 0,
			/* .MaxFragmentAtomicCounterBuffers = */ 1,
			/* .MaxCombinedAtomicCounterBuffers = */ 1,
			/* .MaxAtomicCounterBufferSize = */ 16384,
			/* .MaxTransformFeedbackBuffers = */ 4,
			/* .MaxTransformFeedbackInterleavedComponents = */ 64,
			/* .MaxCullDistances = */ 8,
			/* .MaxCombinedClipAndCullDistances = */ 8,
			/* .MaxSamples = */ 4,
			/* .limits = */{
				/* .nonInductiveForLoops = */ 1,
				/* .whileLoops = */ 1,
				/* .doWhileLoops = */ 1,
				/* .generalUniformIndexing = */ 1,
				/* .generalAttributeMatrixVectorIndexing = */ 1,
				/* .generalVaryingIndexing = */ 1,
				/* .generalSamplerIndexing = */ 1,
				/* .generalVariableIndexing = */ 1,
				/* .generalConstantMatrixVectorIndexing = */ 1,
			} };

		class GLSLUtilIncluder : public glslang::TShader::Includer
		{
		public:
			GLSLUtilIncluder(const GLSLInclude& include)
				: m_include(include)
			{
			}

			virtual IncludeResult* include(
				const char* requested_source,
				IncludeType type,
				const char* requesting_source,
				size_t inclusion_depth
			) override
			{
				std::string path;
				TextFileReader glslFile;
				if (type == EIncludeRelative)
				{
					if (requesting_source[0] == '\0')
					{
						path = PathUtil::Combine(m_include.GetWorkDir(), requested_source);
					}
					else
					{
						std::string dir = PathUtil::GetDirectoryName(requesting_source);
						path = PathUtil::Combine(dir, requested_source);
					}
					glslFile.Open(path);
				}

				if (!glslFile.IsOpen())
				{
					for (const auto& dir : m_include.GetPathList())
					{
						path = PathUtil::Combine(dir, requested_source);
						glslFile.Open(path);
						if (!glslFile.IsOpen())
						{
							break;
						}
					}
				}

				IncludeResult* result = nullptr;
				if (glslFile.IsOpen())
				{
					std::string header = glslFile.ReadAll();

					std::string* data = new std::string();
					*data = std::move(header);
					result = new IncludeResult(path, data->c_str(), data->size(), data);
				}
				else
				{
					if (requesting_source[0] != '\0')
					{
						std::cout << requesting_source << ": ";
					}
					std::cout << "File not found. [" << requested_source << "]\n";
					result = new IncludeResult(std::string(""), "", 0, nullptr);
				}

				return result;
			}

			virtual void releaseInclude(IncludeResult* result) override
			{
				if (result->user_data != nullptr)
				{
					std::string* data = (std::string*)result->user_data;
					delete data;
				}
				delete result;
			}
		private:
			const GLSLInclude&	m_include;
		};
	}

	bool PreprocessGLSL(
		std::string*		outCode,
		GLSLShaderLang		lang,
		const std::string&	inCode,
		const GLSLDefine&	define,
		const GLSLInclude&	include,
		std::string*		outMessage
	)
	{
		if (!Singleton<GLSLangInitializer>::Get()->IsInitialized())
		{
			return false;
		}

		if (outCode == nullptr)
		{
			return false;
		}

		EShLanguage glslLang;
		switch (lang)
		{
		case GLSLShaderLang::Vertex:
			glslLang = EShLangVertex;
			break;
		case GLSLShaderLang::TessControl:
			glslLang = EShLangTessControl;
			break;
		case GLSLShaderLang::TessEvaluation:
			glslLang = EShLangTessEvaluation;
			break;
		case GLSLShaderLang::Geometry:
			glslLang = EShLangGeometry;
			break;
		case GLSLShaderLang::Fragment:
			glslLang = EShLangFragment;
			break;
		case GLSLShaderLang::Compute:
			glslLang = EShLangCompute;
			break;
		default:
			std::cout << "Unkown Shader Type.\n";
			return false;
		}
		glslang::TShader glslShader(glslLang);

		TBuiltInResource resource = DefaultTBuiltInResource;
		const int defaultVersion = 110;
		GLSLUtilIncluder includer(include);
		EShMessages messages = EShMsgDefault;

		std::stringstream preambleSS;
		preambleSS << "#extension GL_GOOGLE_include_directive : require\n";
		for (const auto& defPair : define.GetMap())
		{
			preambleSS << "#define " << defPair.first << " " << defPair.second << "\n";
		}

		std::string outputString;

		std::string preamble = preambleSS.str();
		glslShader.setPreamble(preamble.c_str());

		const char* codes[] = {
			inCode.c_str(),
		};
		glslShader.setStrings(codes, 1);
		bool ret = glslShader.preprocess(
			&resource,
			defaultVersion,
			ENoProfile,
			false,
			false,
			messages,
			&outputString,
			includer
		);

		if (outMessage != nullptr)
		{
			*outMessage = glslShader.getInfoLog();
		}
		else
		{
			const char* info = glslShader.getInfoLog();
			if (info != nullptr)
			{
				std::cout << "glslang info:\n";
				std::cout << info << "\n";
			}
		}

		if (!ret)
		{
			return false;
		}

		// 不要な文字列を削除
		const char* removeTexts[] =
		{
			"#extension GL_GOOGLE_include_directive : require",
			"#line",
		};
		std::stringstream inSS(outputString);
		std::stringstream outSS;
		std::string line;
		while (!inSS.eof())
		{
			std::getline(inSS, line);

			bool isRemove = false;
			for (const char* removeText : removeTexts)
			{
				if (line.compare(0, strlen(removeText), removeText) == 0)
				{
					isRemove = true;
					break;
				}
			}
			if (!isRemove)
			{
				outSS << line << "\n";
			}
			int ch = inSS.peek();
			if (std::stringstream::traits_type::eof() == ch)
			{
				break;
			}
		}

		outputString = outSS.str();
		*outCode = std::move(outputString);

		return true;
	}

	void GLSLShaderUtil::SetShaderDir(const std::string & shaderDir)
	{
		m_shaderDir = shaderDir;
	}

	void GLSLShaderUtil::SetGLSLDefine(const GLSLDefine & define)
	{
		m_define = define;
	}

	void GLSLShaderUtil::SetGLSLInclude(const GLSLInclude & include)
	{
		m_include = include;
	}

	GLProgramObject GLSLShaderUtil::CreateProgram(const char * shaderName)
	{
		std::string shaderFilePath = PathUtil::Combine(m_shaderDir, shaderName);
		std::string vsFilePath = shaderFilePath + ".vert";
		std::string fsFilePath = shaderFilePath + ".frag";

		std::string vsCode;
		{
			SABA_INFO("Vertex Shader File Open. {}", vsFilePath);
			TextFileReader glslFile(vsFilePath);
			if (!glslFile.IsOpen())
			{
				SABA_WARN("Open fail.");
				return GLProgramObject();
			}
			vsCode = glslFile.ReadAll();
		}

		std::string fsCode;
		{
			SABA_INFO("Fragment Shader File Open. {}", vsFilePath);
			TextFileReader glslFile(fsFilePath);
			if (!glslFile.IsOpen())
			{
				SABA_WARN("Open fail.");
				return GLProgramObject();
			}
			fsCode = glslFile.ReadAll();
		}

		return CreateProgram(vsCode.c_str(), fsCode.c_str());
	}

	GLProgramObject GLSLShaderUtil::CreateProgram(const char * vsCode, const char * fsCode)
	{
		GLSLInclude include = m_include;
		if (!m_shaderDir.empty())
		{
			include.AddInclude(m_shaderDir);
		}

		std::string ppMessage;
		std::string ppVsCode;
		bool ret = PreprocessGLSL(
			&ppVsCode,
			GLSLShaderLang::Vertex,
			vsCode,
			m_define,
			include,
			&ppMessage);
		if (!ret)
		{
			std::cout << "preprocess fail.\n";
			std::cout << ppMessage;
			return GLProgramObject();
		}

		std::string ppFsCode;
		ret = PreprocessGLSL(
			&ppFsCode,
			GLSLShaderLang::Fragment,
			fsCode,
			m_define,
			include,
			&ppMessage);
		if (!ret)
		{
			std::cout << "preprocess fail.\n";
			std::cout << ppMessage;
			return GLProgramObject();
		}

		return CreateShaderProgram(ppVsCode.c_str(), ppFsCode.c_str());
	}

}
