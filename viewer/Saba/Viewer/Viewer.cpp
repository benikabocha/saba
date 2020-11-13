//
// Copyright(c) 2016-2019 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Viewer.h"
#include "VMDCameraOverrider.h"
#include "ShadowMap.h"

#include <Saba/Base/Singleton.h>
#include <Saba/Base/Log.h>
#include <Saba/Base/Path.h>
#include <Saba/Base/Time.h>
#include <Saba/GL/GLSLUtil.h>
#include <Saba/GL/GLShaderUtil.h>

#include <Saba/Model/OBJ/OBJModel.h>
#include <Saba/GL/Model/OBJ/GLOBJModel.h>
#include <Saba/GL/Model/OBJ/GLOBJModelDrawer.h>

#include <Saba/Model/MMD/PMDModel.h>
#include <Saba/Model/MMD/VMDFile.h>
#include <Saba/Model/MMD/VPDFile.h>
#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/GL/Model/MMD/GLMMDModel.h>
#include <Saba/GL/Model/MMD/GLMMDModelDrawer.h>
#include <Saba/Model/MMD/SjisToUnicode.h>

#include <Saba/Model/XFile/XFileModel.h>
#include <Saba/GL/Model/XFile/GLXFileModel.h>
#include <Saba/GL/Model/XFile/GLXFileModelDrawer.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuizmo.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <deque>
#include <sstream>
#include <iomanip>
#include <string>
#include <thread>

namespace saba
{
	class ImGUILogSink : public spdlog::sinks::sink
	{
	public:
		explicit ImGUILogSink(size_t maxBufferSize = 100)
			: m_maxBufferSize(maxBufferSize)
			, m_added(false)
		{
		}

		void log(const spdlog::details::log_msg& msg) override
		{
			while (m_buffer.size() >= m_maxBufferSize)
			{
				if (m_buffer.empty())
				{
					break;
				}
				m_buffer.pop_front();
			}

			m_buffer.emplace_back(LogMessage{msg.level, msg.formatted.str()});
			m_added = true;
		}

		void flush() override
		{
		}

		struct LogMessage
		{
			spdlog::level::level_enum	m_level;
			std::string					m_message;
		};
		const std::deque<LogMessage>& GetBuffer() const { return m_buffer; }

		bool IsAdded() const { return m_added; }
		void ClearAddedFlag() { m_added = false; }

	private:
		size_t					m_maxBufferSize;
		std::deque<LogMessage>	m_buffer;
		bool					m_added;
	};

	Viewer::InitializeParameter::InitializeParameter()
		: m_msaaEnable(false)
		, m_msaaCount(4)
		, m_initCamera(false)
		, m_initCameraCenter(0, 0, 0)
		, m_initCameraEye(0, 0, 10)
		, m_initCameraNearClip(0.1f)
		, m_initCameraFarClip(1000.0f)
		, m_initCameraRadius(10.0f)
		, m_initScene(false)
		, m_initSceneUnitScale(1.0f)
	{
	}

	Viewer::MMDModelConfig::MMDModelConfig()
		: m_parallelUpdateCount(0)
	{
	}

	const glm::vec3 Viewer::DefaultBGColor1 = glm::vec3(0.2f);
	const glm::vec3 Viewer::DefaultBGColor2 = glm::vec3(0.4f);

	Viewer::Viewer()
		: m_glfwInitialized(false)
		, m_window(nullptr)
		, m_uColor1(-1)
		, m_uColor2(-1)
		, m_cameraMode(CameraMode::None)
		, m_gridEnabled(true)
		, m_mouseLockMode(MouseLockMode::None)
		, m_sceneUnitScale(1)
		, m_prevTime(0)
		, m_modelNameID(1)
		, m_enableInfoUI(true)
		, m_enableMoreInfoUI(false)
		, m_enableLogUI(true)
		, m_enableCommandUI(true)
		, m_enableManip(false)
		, m_currentManipOp(ImGuizmo::TRANSLATE)
		, m_currentManipMode(ImGuizmo::LOCAL)
		, m_animCtrlEditFPS(30.0f)
		, m_animCtrlFPSMode(FPSMode::FPS30)
		, m_animFixedUpdate(false)
		, m_enableCtrlUI(true)
		, m_enableLightManip(false)
		, m_enableLightGuide(false)
		, m_lightManipOp(ImGuizmo::ROTATE)
		, m_bgColor1(DefaultBGColor1)
		, m_bgColor2(DefaultBGColor2)
		, m_cameraOverride(true)
		, m_clipElapsed(true)
		, m_currentFrameBufferWidth(-1)
		, m_currentFrameBufferHeight(-1)
		, m_currentMSAAEnable(false)
		, m_currentMSAACount(0)
	{
		if (!glfwInit())
		{
			m_glfwInitialized = false;
		}
		m_glfwInitialized = true;
	}

	Viewer::~Viewer()
	{
		if (m_glfwInitialized)
		{
			glfwTerminate();
		}
	}

	bool Viewer::Initialize(const InitializeParameter& initParam)
	{
		SABA_INFO("Execute Path = {}", PathUtil::GetExecutablePath());

		m_initParam = initParam;

		auto logger = Singleton<saba::Logger>::Get();
		m_imguiLogSink = logger->AddSink<ImGUILogSink>();

		SABA_INFO("CurDir = {}", m_context.GetWorkDir());
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		if (m_initParam.m_msaaEnable)
		{
			m_context.EnableMSAA(true);
			m_context.SetMSAACount(m_initParam.m_msaaCount);
		}
		m_window = glfwCreateWindow(1280, 800, "Saba Viewer", nullptr, nullptr);

		if (m_window == nullptr)
		{
			SABA_ERROR("Window Create Fail.");
			return false;
		}

		// glfwコールバックの登録
		glfwSetWindowUserPointer(m_window, this);
		glfwSetMouseButtonCallback(m_window, OnMouseButtonStub);
		glfwSetScrollCallback(m_window, OnScrollStub);
		glfwSetKeyCallback(m_window, OnKeyStub);
		glfwSetCharCallback(m_window, OnCharStub);
		glfwSetDropCallback(m_window, OnDropStub);

		glfwMakeContextCurrent(m_window);

		// imguiの初期化
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(m_window, false);
		ImGui_ImplOpenGL3_Init("#version 150");

		std::string fontDir = PathUtil::Combine(
			m_context.GetResourceDir(),
			"font"
		);
		std::string fontPath = PathUtil::Combine(
			fontDir,
			"mgenplus-1mn-bold.ttf"
		);
		SetupSjisGryphRanges();
		io.Fonts->AddFontFromFileTTF(
			fontPath.c_str(),
			14.0f,
			nullptr,
			m_gryphRanges.data()//io.Fonts->GetGlyphRangesJapanese()
		);

		// gl3wの初期化
		if (gl3wInit() != 0)
		{
			SABA_ERROR("gl3w Init Fail.");
			return false;
		}

		if (!m_context.Initialize())
		{
			SABA_ERROR("Failed to initialize ViewerContext.");
			return false;
		}

		GLSLShaderUtil glslShaderUtil;
		glslShaderUtil.SetShaderDir(m_context.GetShaderDir());

		m_bgProg = glslShaderUtil.CreateProgram("bg");
		if (m_bgProg.Get() == 0)
		{
			SABA_ERROR("'bg' Shader Create Fail.");
			return false;
		}
		m_uColor1 = glGetUniformLocation(m_bgProg, "u_Color1");
		m_uColor2 = glGetUniformLocation(m_bgProg, "u_Color2");
		m_bgVAO.Create();

		m_mouse.Initialize(m_window);
		m_context.m_camera.Initialize(glm::vec3(0), 10.0f);
		if (!m_grid.Initialize(m_context, 0.5f, 10, 5))
		{
			SABA_ERROR("grid Init Fail.");
			return false;
		}

		if (!m_context.m_shadowmap.InitializeShader(&m_context))
		{
			SABA_ERROR("shadowmap InitializeShader Fail.");
			return false;
		}
		if (!m_context.m_shadowmap.Setup(1024, 1024, 4))
		{
			SABA_ERROR("shadowmap Setup Fail.");
			return false;
		}

		m_objModelDrawContext = std::make_unique<GLOBJModelDrawContext>(&m_context);
		m_mmdModelDrawContext = std::make_unique<GLMMDModelDrawContext>(&m_context);
		m_xfileModelDrawContext = std::make_unique<GLXFileModelDrawContext>(&m_context);

		RegisterCommand();
		RefreshCustomCommand();

		m_prevTime = GetTime();

		return true;
	}

	void Viewer::Uninitislize()
	{
		auto logger = Singleton<saba::Logger>::Get();
		logger->RemoveSink(m_imguiLogSink.get());
		m_imguiLogSink.reset();

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		m_context.Uninitialize();
	}

	int Viewer::Run()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::BeginFrame();

			m_mouse.Update(m_window);

			if (m_mouseLockMode == MouseLockMode::RequestLock)
			{
				if (ImGui::GetIO().WantCaptureMouse ||
					ImGuizmo::IsOver())
				{
					m_mouseLockMode = MouseLockMode::None;
					m_cameraMode = CameraMode::None;
				}
				else
				{
					m_mouseLockMode = MouseLockMode::Lock;
				}
			}

			if (m_mouseLockMode == MouseLockMode::Lock && m_cameraMode != CameraMode::None)
			{
				if (m_cameraMode == CameraMode::Orbit)
				{
					m_context.m_camera.Orbit((float)m_mouse.m_dx, (float)m_mouse.m_dy);
				}
				if (m_cameraMode == CameraMode::Dolly)
				{
					m_context.m_camera.Dolly((float)m_mouse.m_dx + (float)m_mouse.m_dy);
				}
				if (m_cameraMode == CameraMode::Pan)
				{
					m_context.m_camera.Pan((float)m_mouse.m_dx, (float)m_mouse.m_dy);
				}
			}

			if (m_mouse.m_scrollY != 0 && (!ImGui::GetIO().WantCaptureMouse))
			{
				m_context.m_camera.Dolly((float)m_mouse.m_scrollY * 0.1f);
			}

			ImGuizmo::Enable(m_cameraMode == CameraMode::None);

			int w, h;
			glfwGetFramebufferSize(m_window, &w, &h);
			m_context.m_camera.SetSize((float)w, (float)h);
			m_context.SetFrameBufferSize(w, h);
			int windowW, windowH;
			glfwGetWindowSize(m_window, &windowW, &windowH);
			m_context.SetWindowSize(windowW, windowH);

			if (m_context.IsUIEnabled())
			{
				DrawMenuBar();
			}

			Update();

			if (m_context.IsShadowEnabled())
			{
				DrawShadowMap();
			}
			Draw();

			if (m_context.IsUIEnabled())
			{
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}

			glfwSwapBuffers(m_window);
			glfwPollEvents();
		}

		return 0;
	}

	void Viewer::SetupSjisGryphRanges()
	{
		const int ASCIIBegin = 0x20;
		const int ASCIIEnd = 0x7E;

		const int HankakuBegin = 0xA1;
		const int HankakuEnd = 0xDF;

		const int SjisFirstBegin1 = 0x81;
		const int SjisFirstEnd1 = 0x9F;

		const int SjisFirstBegin2 = 0xE0;
		const int SjisFirstEnd2 = 0xEF;

		const int SjisSecondBegin = 0x40;
		const int SjisSecondEnd = 0xFC;
		std::vector<ImWchar> wcharTable;
		for (int ch = ASCIIBegin; ch <= ASCIIEnd; ch++)
		{
			wcharTable.push_back((ImWchar)ch);
		}
		for (int ch = HankakuBegin; ch <= HankakuEnd; ch++)
		{
			wcharTable.push_back((ImWchar)saba::ConvertSjisToU16Char(ch));
		}
		for (int sjisFirst = SjisFirstBegin1; sjisFirst <= SjisFirstEnd1; sjisFirst++)
		{
			for (int sjisSecond = SjisSecondBegin; sjisSecond <= SjisSecondEnd; sjisSecond++)
			{
				int ch = (sjisFirst << 8) | sjisSecond;
				wcharTable.push_back((ImWchar)saba::ConvertSjisToU16Char(ch));
			}
		}
		for (int sjisFirst = SjisFirstBegin2; sjisFirst <= SjisFirstEnd2; sjisFirst++)
		{
			for (int sjisSecond = SjisSecondBegin; sjisSecond <= SjisSecondEnd; sjisSecond++)
			{
				int ch = (sjisFirst << 8) | sjisSecond;
				wcharTable.push_back((ImWchar)saba::ConvertSjisToU16Char(ch));
			}
		}
		std::sort(wcharTable.begin(), wcharTable.end());
		auto removeIt = std::unique(wcharTable.begin(), wcharTable.end());
		wcharTable.erase(removeIt, wcharTable.end());

		m_gryphRanges.clear();
		if (!wcharTable.empty())
		{
			auto begin = wcharTable.begin();
			auto end = wcharTable.end();
			auto prevCh = (*begin);
			auto it = begin + 1;
			while (it != end)
			{
				if ((prevCh + 1) != (*it))
				{
					m_gryphRanges.push_back((*begin));
					m_gryphRanges.push_back(prevCh);

					begin = it;
				}
				prevCh = (*it);
				++it;
			}
			m_gryphRanges.push_back((*begin));
			m_gryphRanges.push_back(prevCh);
		}
		m_gryphRanges.push_back(0);
	}

	void Viewer::Update()
	{
		if (m_context.IsUIEnabled())
		{
			DrawUI();
		}

		m_context.SetClipElapsed(m_clipElapsed);
		m_context.EnableCameraOverride(m_cameraOverride);
		bool update = true;
		double time = GetTime();
		double elapsed = time - m_prevTime;
		if (m_animFixedUpdate)
		{
			double fixedEpalsed = 1.0 / double(m_animCtrlEditFPS);
			if (elapsed > fixedEpalsed)
			{
				double offsetElapsed = elapsed - fixedEpalsed;
				if (offsetElapsed < fixedEpalsed)
				{
					m_prevTime = time - offsetElapsed;
				}
				else
				{
					m_prevTime = time;
				}
				m_context.SetElapsedTime(fixedEpalsed);
			}
			else
			{
				update = false;
			}
		}
		else
		{
			m_prevTime = time;
			m_context.SetElapsedTime(elapsed);
		}

		if (update)
		{
			UpdateAnimation();
		}

		if (m_cameraOverride && m_cameraOverrider)
		{
			Camera overrideCam = *m_context.GetCamera();
			m_cameraOverrider->Override(&m_context, &overrideCam);
			m_context.SetCamera(overrideCam);
		}
		m_context.m_camera.UpdateMatrix();

		if (update)
		{
			for (auto& modelDrawer : m_modelDrawers)
			{
				// Update
				modelDrawer->Update(&m_context);
			}
		}

		if (m_context.IsShadowEnabled())
		{
			m_context.m_shadowmap.CalcShadowMap(m_context.GetCamera(), m_context.GetLight());
		}

		if (m_context.GetPlayMode() == ViewerContext::PlayMode::Update)
		{
			m_context.SetPlayMode(ViewerContext::PlayMode::Stop);
		}
	}

	void Viewer::DrawShadowMap()
	{
		auto shadowMap = m_context.GetShadowMap();
		glDisable(GL_MULTISAMPLE);
		glViewport(0, 0, shadowMap->GetWidth(), shadowMap->GetHeight());
		size_t csmCount = shadowMap->GetClipSpaceCount();
		for (size_t i = 0; i < csmCount; i++)
		{
			const auto& clipSpace = m_context.GetShadowMap()->GetClipSpace(i);
			glBindFramebuffer(GL_FRAMEBUFFER, clipSpace.m_shadowmapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);

			for (auto& modelDrawer : m_modelDrawers)
			{
				// Shadow
				modelDrawer->DrawShadowMap(&m_context, i);
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Viewer::Draw()
	{
		DrawBegin();

		glViewport(0, 0, m_context.GetFrameBufferWidth(), m_context.GetFrameBufferHeight());
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(m_bgVAO);
		glUseProgram(m_bgProg);
		SetUniform(m_uColor1, m_bgColor1);
		SetUniform(m_uColor2, m_bgColor2);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUseProgram(0);
		glBindVertexArray(0);

		glEnable(GL_DEPTH_TEST);

		if (m_gridEnabled)
		{
			// Draw grid
			const auto world = glm::mat4(1.0);
			const auto& view = m_context.GetCamera()->GetViewMatrix();
			const auto& proj = m_context.GetCamera()->GetProjectionMatrix();
			const auto wv = view * world;
			const auto wvp = proj * view * world;
			m_grid.SetWVPMatrix(wvp);
			m_grid.Draw();
		}

		for (auto& modelDrawer : m_modelDrawers)
		{
			// Draw
			modelDrawer->Draw(&m_context);
		}

		DrawEnd();
	}

	void Viewer::DrawBegin()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (m_context.GetFrameBufferWidth() != m_currentFrameBufferWidth ||
			m_context.GetFrameBufferHeight() != m_currentFrameBufferHeight ||
			m_context.IsMSAAEnabled() != m_currentMSAAEnable ||
			m_context.GetMSAACount() != m_currentMSAACount
			)
		{
			SABA_INFO(
				"Create Framebuffer : ({}, {})",
				m_context.GetFrameBufferWidth(), m_context.GetFrameBufferHeight()
			);
			m_currentFrameBuffer.Destroy();
			m_currentMSAAFrameBuffer.Destroy();
			m_currentColorTarget.Destroy();
			m_currentMSAAColorTarget.Destroy();
			m_currentDepthTarget.Destroy();

			m_currentColorTarget.Create();
			glBindTexture(GL_TEXTURE_2D, m_currentColorTarget);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA,
				m_context.GetFrameBufferWidth(),
				m_context.GetFrameBufferHeight(),
				0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
			);
			glBindTexture(GL_TEXTURE_2D, 0);

			if (!m_context.ResizeCaptureTexture())
			{
				SABA_ERROR("Failed to resize capture texture.");
			}
			// Setup capture framebuffer
			{
				m_captureFrameBuffer.Create();
				glBindFramebuffer(GL_FRAMEBUFFER, m_captureFrameBuffer);
				glFramebufferTexture2D(
					GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_2D, m_context.GetCaptureTexture(), 0
				);
				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
					GL_RENDERBUFFER, 0
				);
				auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (GL_FRAMEBUFFER_COMPLETE != status)
				{
					SABA_WARN("Framebuffer Status : {}", status);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					return;
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

			}

			if (m_context.IsMSAAEnabled())
			{
				// Setup MSAA Target
				m_currentMSAAColorTarget.Create();
				glBindRenderbuffer(GL_RENDERBUFFER, m_currentMSAAColorTarget);
				glRenderbufferStorageMultisample(
					GL_RENDERBUFFER, m_context.GetMSAACount(), GL_RGBA,
					m_context.GetFrameBufferWidth(),
					m_context.GetFrameBufferHeight()
				);
				glBindRenderbuffer(GL_RENDERBUFFER, 0);

				// Setup Depth Target
				m_currentDepthTarget.Create();
				glBindRenderbuffer(GL_RENDERBUFFER, m_currentDepthTarget);
				glRenderbufferStorageMultisample(
					GL_RENDERBUFFER, m_context.GetMSAACount(), GL_DEPTH24_STENCIL8,
					m_context.GetFrameBufferWidth(),
					m_context.GetFrameBufferHeight()
				);
				glBindRenderbuffer(GL_RENDERBUFFER, 0);


				// Setup MSAA Framebuffer
				m_currentMSAAFrameBuffer.Create();
				glBindFramebuffer(GL_FRAMEBUFFER, m_currentMSAAFrameBuffer);
				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_currentMSAAColorTarget
				);
				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_currentDepthTarget
				);
				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_currentDepthTarget
				);
				auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (GL_FRAMEBUFFER_COMPLETE != status)
				{
					SABA_WARN("MSAA Framebuffer Status : {}", status);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					return;
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				// Setup Framebuffer (depth target off)
				m_currentFrameBuffer.Create();
				glBindFramebuffer(GL_FRAMEBUFFER, m_currentFrameBuffer);
				glFramebufferTexture2D(
					GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_2D, m_currentColorTarget, 0
				);
				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
					GL_RENDERBUFFER, 0
				);
				status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (GL_FRAMEBUFFER_COMPLETE != status)
				{
					SABA_WARN("Framebuffer Status : {}", status);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					return;
				}
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}
			else
			{
				// Setup Depth Target
				m_currentDepthTarget.Create();
				glBindRenderbuffer(GL_RENDERBUFFER, m_currentDepthTarget);
				glRenderbufferStorage(
					GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
					m_context.GetFrameBufferWidth(),
					m_context.GetFrameBufferHeight()
				);

				// Setup Framebuffer (depth target off)
				m_currentFrameBuffer.Create();
				glBindFramebuffer(GL_FRAMEBUFFER, m_currentFrameBuffer);
				glFramebufferTexture2D(
					GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_2D, m_currentColorTarget, 0
				);
				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
					GL_RENDERBUFFER, m_currentDepthTarget
				);
				glFramebufferRenderbuffer(
					GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
					GL_RENDERBUFFER, m_currentDepthTarget
				);
				auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (GL_FRAMEBUFFER_COMPLETE != status)
				{
					SABA_WARN("Framebuffer Status : {}", status);
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					return;
				}
				glBindRenderbuffer(GL_RENDERBUFFER, 0);
			}

			m_currentFrameBufferWidth = m_context.GetFrameBufferWidth();
			m_currentFrameBufferHeight = m_context.GetFrameBufferHeight();
			m_currentMSAAEnable = m_context.IsMSAAEnabled();
			m_currentMSAACount = m_context.GetMSAACount();
		}

		if (m_currentMSAAEnable)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_currentMSAAFrameBuffer);
			glEnable(GL_MULTISAMPLE);
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_currentFrameBuffer);
		}
	}

	void Viewer::DrawEnd()
	{
		if (m_currentMSAAEnable)
		{
			glDisable(GL_MULTISAMPLE);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_currentFrameBuffer);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_currentMSAAFrameBuffer);
			glBlitFramebuffer(
				0, 0, m_currentFrameBufferWidth, m_currentFrameBufferHeight,
				0, 0, m_currentFrameBufferWidth, m_currentFrameBufferHeight,
				GL_COLOR_BUFFER_BIT, GL_NEAREST
			);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		if (m_captureFrameBuffer != 0)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_captureFrameBuffer);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_currentFrameBuffer);
			glBlitFramebuffer(
				0, 0, m_currentFrameBufferWidth, m_currentFrameBufferHeight,
				0, 0, m_currentFrameBufferWidth, m_currentFrameBufferHeight,
				GL_COLOR_BUFFER_BIT, GL_NEAREST
			);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		if (m_currentFrameBuffer != 0)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_currentFrameBuffer);
			glDrawBuffer(GL_BACK);
			glBlitFramebuffer(
				0, 0, m_currentFrameBufferWidth, m_currentFrameBufferHeight,
				0, 0, m_currentFrameBufferWidth, m_currentFrameBufferHeight,
				GL_COLOR_BUFFER_BIT, GL_NEAREST
			);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
	}

	void Viewer::DrawMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Window"))
			{
				ImGui::MenuItem("Info", nullptr, &m_enableInfoUI);
				ImGui::MenuItem("More Info", nullptr, &m_enableMoreInfoUI);
				ImGui::MenuItem("Log", nullptr, &m_enableLogUI);
				ImGui::MenuItem("Command", nullptr, &m_enableCommandUI);
				ImGui::MenuItem("Control", nullptr, &m_enableCtrlUI);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Manipulater", nullptr, &m_enableManip))
				{
					if (m_enableManip)
					{
						m_enableLightManip = false;
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Viewer"))
			{
				bool cameraOverride = m_cameraOverride;
				bool cameraOverrideEnable = true;
				if (m_cameraOverrider == nullptr)
				{
					cameraOverride = false;
					cameraOverrideEnable = false;
				}
				ImGui::MenuItem("CameraOverride", nullptr, &cameraOverride, cameraOverrideEnable);
				if (m_cameraOverrider != nullptr)
				{
					m_cameraOverride = cameraOverride;
				}
				ImGui::MenuItem("ClipElapsed", nullptr, &m_clipElapsed);
				ImGui::MenuItem("Grid", nullptr, &m_gridEnabled);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Animation"))
			{
				if (ImGui::MenuItem("30 FPS", nullptr, m_animCtrlFPSMode == FPSMode::FPS30))
				{
					m_animCtrlFPSMode = FPSMode::FPS30;
					m_animCtrlEditFPS = 30.0f;
				}
				if (ImGui::MenuItem("60 FPS", nullptr, m_animCtrlFPSMode == FPSMode::FPS60))
				{
					m_animCtrlFPSMode = FPSMode::FPS60;
					m_animCtrlEditFPS = 60.0f;
				}
				if (ImGui::MenuItem("Fixed Animation", nullptr, m_animFixedUpdate))
				{
					m_animFixedUpdate = !m_animFixedUpdate;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("CustomCommand"))
			{
				if (ImGui::MenuItem("RefreshCustomCommand"))
				{
					ViewerCommand cmd;
					cmd.SetCommand("refreshCustomCommand");
					ExecuteCommand(cmd);
				}
				ImGui::Separator();

				DrawCustomMenu(&m_customCommandMenuItemRoot);
			}
			ImGui::EndMainMenuBar();
		}
	}

	void Viewer::DrawCustomMenu(CustomCommandMenuItem* parentItem)
	{
		for (auto& menuItemPair : (*parentItem).m_items)
		{
			if (menuItemPair.second.m_command == nullptr)
			{
				if (ImGui::BeginMenu(menuItemPair.first.c_str()))
				{
					DrawCustomMenu(&menuItemPair.second);
				}
			}
			else
			{
				if (ImGui::MenuItem(menuItemPair.first.c_str()))
				{
					ViewerCommand cmd;
					cmd.SetCommand(menuItemPair.second.m_command->m_name);
					this->ExecuteCommand(cmd);
				}
			}
		}
		ImGui::EndMenu();
	}

	void Viewer::DrawUI()
	{
		DrawInfoUI();
		DrawLogUI();
		DrawCommandUI();
		if (m_enableManip)
		{
			DrawManip();
		}
		DrawCtrlUI();

		DrawLightGuide();
	}

	namespace
	{
		template <typename T>
		void PushPerfLap(std::deque<T>& lap, const T& val)
		{
			if (lap.size() >= 100)
			{
				lap.pop_front();
			}
			lap.push_back(val);
		}

		template <typename T>
		T GetPerfLapMax(const std::deque<T>& lap)
		{
			auto it = std::max_element(lap.begin(), lap.end());
			if (it == lap.end())
			{
				return 0;
			}
			return (*it);
		}

		template <typename T>
		T GetPerfLapMin(const std::deque<T>& lap)
		{
			auto it = std::min_element(lap.begin(), lap.end());
			if (it == lap.end())
			{
				return 0;
			}
			return (*it);
		}

		template <typename T>
		T GetPerfLapAve(const std::deque<T>& lap)
		{
			T count = T(0);
			T sum = T(0);
			for (const auto& val : lap)
			{
				sum += val;
				count += T(1);
			}
			if (count == T(0))
			{
				return T(0);
			}
			return sum / count;
		}
	} // namespace

	void Viewer::DrawInfoUI()
	{
		if (!m_enableInfoUI)
		{
			return;
		}
		ImGui::SetNextWindowPos(ImVec2(10, 30));
		if (!ImGui::Begin("Info", &m_enableCommandUI, ImGuiWindowFlags_NoTitleBar /*| ImGuiWindowFlags_NoResize*/ | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::End();
			return;
		}
		ImGui::Text("Info");
		ImGui::Separator();
		ImGui::Text("Time %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		PushPerfLap(m_perfFramerateLap, ImGui::GetIO().Framerate);
		float aveFps = GetPerfLapAve(m_perfFramerateLap);
		float minFps = GetPerfLapMin(m_perfFramerateLap);
		if (m_enableMoreInfoUI)
		{
			ImGui::Text("FPS ave:%.2f min:%.2f max time:%.2f[ms]", aveFps, minFps, 1000.0f / minFps);
		}

		if (m_selectedModelDrawer != nullptr && m_selectedModelDrawer->GetType() == ModelDrawerType::MMDModelDrawer)
		{
			auto mmdModelDrawer = reinterpret_cast<GLMMDModelDrawer*>(m_selectedModelDrawer.get());
			auto mmdModel = mmdModelDrawer->GetModel();
			if (mmdModel != nullptr)
			{
				ImGui::Text("MMD Model Update Time %.3f ms", mmdModel->GetUpdateTime() * 1000.0);
				const auto& perfInfo = mmdModel->GetPerfInfo();

				PushPerfLap(m_perfMMDSetupAnimTimeLap, perfInfo.m_setupAnimTime);
				PushPerfLap(m_perfMMDUpdateMorphAnimTimeLap, perfInfo.m_updateMorphAnimTime);
				PushPerfLap(m_perfMMDUpdateNodeAnimTimeLap, perfInfo.m_updateNodeAnimTime);
				PushPerfLap(m_perfMMDUpdatePhysicsAnimTimeLap, perfInfo.m_updatePhysicsAnimTime);
				PushPerfLap(m_perfMMDUpdateModelTimeLap, perfInfo.m_updateModelTime);
				PushPerfLap(m_perfMMDUpdateGLBufferTimeLap, perfInfo.m_updateGLBufferTime);

				if (m_enableMoreInfoUI)
				{
					// Setup animation
					ImGui::Text("Setup  ave:%.2f max:%.2f now:%.2f [ms]",
						float(GetPerfLapAve(m_perfMMDSetupAnimTimeLap) * 1000.0),
						float(GetPerfLapMax(m_perfMMDSetupAnimTimeLap) * 1000.0),
						float(perfInfo.m_setupAnimTime * 1000.0)
					);

					// Morph animation
					ImGui::Text("Morph  ave:%.2f max:%.2f now:%.2f [ms]",
						float(GetPerfLapAve(m_perfMMDUpdateMorphAnimTimeLap) * 1000.0),
						float(GetPerfLapMax(m_perfMMDUpdateMorphAnimTimeLap) * 1000.0),
						float(perfInfo.m_updateMorphAnimTime * 1000.0)
					);

					// Node animation
					ImGui::Text("Node   ave:%.2f max:%.2f now:%.2f [ms]",
						float(GetPerfLapAve(m_perfMMDUpdateNodeAnimTimeLap) * 1000.0),
						float(GetPerfLapMax(m_perfMMDUpdateNodeAnimTimeLap) * 1000.0),
						float(perfInfo.m_updateNodeAnimTime * 1000.0)
					);

					// Physics animation
					ImGui::Text("Phyics ave:%.2f max:%.2f now:%.2f [ms]",
						float(GetPerfLapAve(m_perfMMDUpdatePhysicsAnimTimeLap) * 1000.0),
						float(GetPerfLapMax(m_perfMMDUpdatePhysicsAnimTimeLap) * 1000.0),
						float(perfInfo.m_updatePhysicsAnimTime * 1000.0)
					);

					// Update model
					ImGui::Text("Model  ave:%.2f max:%.2f now:%.2f [ms]",
						float(GetPerfLapAve(m_perfMMDUpdateModelTimeLap) * 1000.0),
						float(GetPerfLapMax(m_perfMMDUpdateModelTimeLap) * 1000.0),
						float(perfInfo.m_updateModelTime * 1000.0)
					);

					// Update GLBuffer
					ImGui::Text("Buffer ave:%.2f max:%.2f now:%.2f [ms]",
						float(GetPerfLapAve(m_perfMMDUpdateGLBufferTimeLap) * 1000.0),
						float(GetPerfLapMax(m_perfMMDUpdateGLBufferTimeLap) * 1000.0),
						float(perfInfo.m_updateGLBufferTime * 1000.0)
					);
				}
			}
		}
		ImGui::End();
	}

	void Viewer::DrawLogUI()
	{
		if (!m_enableLogUI)
		{
			return;
		}

		float width = 500;
		float height = 400;

		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(
			ImVec2((float)(m_context.GetWindowWidth()) - width, (float)(m_context.GetWindowHeight()) - height - 80),
			ImGuiCond_FirstUseEver
		);
		ImGui::Begin("Log", &m_enableLogUI);
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		for (const auto& log : m_imguiLogSink->GetBuffer())
		{
			ImVec4 col = ImColor(255, 255, 255, 255);
			switch (log.m_level)
			{
			case spdlog::level::trace:
				col = ImColor(73, 248, 255, 255);
				break;
			case spdlog::level::debug:
				col = ImColor(184, 255, 160, 255);
				break;
			case spdlog::level::info:
				col = ImColor(255, 255, 255, 255);
				break;
			case spdlog::level::warn:
				col = ImColor(255, 238, 7, 255);
				break;
			case spdlog::level::err:
				col = ImColor(255, 0, 67, 255);
				break;
			case spdlog::level::critical:
				col = ImColor(198, 35, 67, 255);
				break;
			case spdlog::level::off:
				col = ImColor(128, 128, 128, 255);
				break;
			default:
				col = ImColor(255, 255, 255, 255);
				break;
			}
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TextUnformatted(log.m_message.c_str());
			ImGui::PopStyleColor();
		}
		// スクロールする場合、最後の行が見えなくなるため、ダミーの行を追加
		ImGui::TextUnformatted("");

		if (m_imguiLogSink->IsAdded())
		{
			ImGui::SetScrollHere(1.0f);
			m_imguiLogSink->ClearAddedFlag();
		}

		ImGui::EndChild();
		ImGui::End();
	}

	void Viewer::DrawCommandUI()
	{
		if (!m_enableCommandUI)
		{
			return;
		}

		float width = 500;
		float height = 0;
		std::array<char, 256> inputBuffer;
		inputBuffer.fill('\0');

		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(
			ImVec2((float)m_context.GetWindowWidth() - width, (float)m_context.GetWindowHeight() - height - 60),
			ImGuiCond_FirstUseEver
		);
		ImGui::Begin("Command", &m_enableCommandUI);
		if (ImGui::InputText("Input", &inputBuffer[0], inputBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue, nullptr, nullptr))
		{
			const char* cmdLine = &inputBuffer[0];
			ViewerCommand cmd;
			if (cmd.Parse(&cmdLine[0]))
			{
				if (!ExecuteCommand(cmd))
				{
					SABA_INFO("Command Execute Error. [{}]", cmdLine);
				}
			}
			else
			{
				SABA_INFO("Command Parse Error. [{}]", cmdLine);
			}

		}
		ImGui::End();
	}

	void Viewer::DrawManip()
	{
		if (m_selectedModelDrawer != nullptr)
		{
			const auto& view = m_context.GetCamera()->GetViewMatrix();
			const auto& proj = m_context.GetCamera()->GetProjectionMatrix();
			auto world = m_selectedModelDrawer->GetTransform();

			ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
			ImGuizmo::Manipulate(
				&view[0][0],
				&proj[0][0],
				m_currentManipOp,
				m_currentManipMode,
				&world[0][0]
			);

			glm::vec3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&world[0][0], &t[0], &r[0], &s[0]);
			switch (m_currentManipOp)
			{
			case ImGuizmo::TRANSLATE:
				m_selectedModelDrawer->SetTranslate(t);
				break;
			case ImGuizmo::ROTATE:
				m_selectedModelDrawer->SetRotate(glm::radians(r));
				break;
			case ImGuizmo::SCALE:
				m_selectedModelDrawer->SetScale(s);
				break;
			}
		}
	}

	void Viewer::DrawCtrlUI()
	{
		if (!m_enableCtrlUI)
		{
			return;
		}


		float width = 300;
		float height = 250;

		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);
		ImGui::SetNextWindowPos(ImVec2(0, 100 + 20), ImGuiCond_Once);
		ImGui::Begin("Control", &m_enableCtrlUI);

		ImGui::PushID("Control UI");

		if (ImGui::CollapsingHeader("Animation"))
		{
			DrawAnimCtrl();
		}
		if (ImGui::CollapsingHeader("Camera"))
		{
			DrawCameraCtrl();
		}
		if (ImGui::CollapsingHeader("Shadow"))
		{
			DrawShadowCtrl();
		}
		if (ImGui::CollapsingHeader("Light"))
		{
			DrawLightCtrl();
		}
		if (ImGui::CollapsingHeader("Model"))
		{
			if (ImGui::TreeNode("Model List"))
			{
				DrawModelListCrtl();
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Transform"))
			{
				DrawTransformCtrl();
				ImGui::TreePop();
			}
			if (m_selectedModelDrawer != nullptr)
			{
				DrawModelCtrl();
			}
		}
		if (ImGui::CollapsingHeader("BG"))
		{
			DrawBGCtrl();
		}

		ImGui::PopID();

		ImGui::End();
	}

	void Viewer::DrawModelListCrtl()
	{
		ImGui::PushID("Model List Control");

		ImGui::BeginChild("models", ImVec2(0, 80), true);

		for (const auto& modelDrawer : m_modelDrawers)
		{
			bool selected = modelDrawer == m_selectedModelDrawer;
			if (ImGui::Selectable(modelDrawer->GetName().c_str(), selected))
			{
				m_selectedModelDrawer = modelDrawer;
			}
		}
		ImGui::EndChild();

		if (ImGui::Button("up"))
		{
			if (m_selectedModelDrawer != nullptr)
			{
				auto findIt = std::find(m_modelDrawers.begin(), m_modelDrawers.end(), m_selectedModelDrawer);
				if (findIt != m_modelDrawers.end())
				{
					if (m_modelDrawers.begin() != findIt)
					{
						std::iter_swap((findIt - 1), findIt);
					}
				}
			}
		}
		if (ImGui::Button("down"))
		{
			if (m_selectedModelDrawer != nullptr)
			{
				auto findIt = std::find(m_modelDrawers.begin(), m_modelDrawers.end(), m_selectedModelDrawer);
				if (findIt != m_modelDrawers.end())
				{
					if (m_modelDrawers.end() != (findIt + 1))
					{
						std::iter_swap((findIt + 1), findIt);
					}
				}
			}
		}

		ImGui::PopID();
	}

	void Viewer::DrawTransformCtrl()
	{
		ImGui::PushID("Transform Control");

		if (ImGui::Checkbox("Manipulator", &m_enableManip))
		{
			if (m_enableManip)
			{
				m_enableLightManip = false;
			}
		}

		if (ImGui::RadioButton("Translate", m_currentManipOp == ImGuizmo::TRANSLATE))
		{
			m_currentManipOp = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", m_currentManipOp == ImGuizmo::ROTATE))
		{
			m_currentManipOp = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", m_currentManipOp == ImGuizmo::SCALE))
		{
			m_currentManipOp = ImGuizmo::SCALE;
		}

		glm::vec3 t, r, s(1.0f);
		if (m_selectedModelDrawer != nullptr)
		{
			t = m_selectedModelDrawer->GetTranslate();
			r = glm::degrees(m_selectedModelDrawer->GetRotate());
			s = m_selectedModelDrawer->GetScale();
		}

		ImGui::InputFloat3("T", &t[0], 3);
		ImGui::InputFloat3("R", &r[0], 3);
		ImGui::InputFloat3("S", &s[0], 3);
		if (m_selectedModelDrawer != nullptr)
		{
			m_selectedModelDrawer->SetTranslate(t);
			m_selectedModelDrawer->SetRotate(glm::radians(r));
			m_selectedModelDrawer->SetScale(s);
		}

		ImGui::PopID();
	}

	void Viewer::DrawAnimCtrl()
	{
		ImGui::PushID("Animation Control");

		float animFrame = float(m_context.GetAnimationTime() * m_animCtrlEditFPS);
		if (ImGui::InputFloat("Frame", &animFrame))
		{
			m_context.SetAnimationTime(animFrame / m_animCtrlEditFPS);
			m_context.SetPlayMode(ViewerContext::PlayMode::Update);
		}

		if (m_context.GetPlayMode() == ViewerContext::PlayMode::Play)
		{
			if (ImGui::Button("Stop"))
			{
				m_context.SetPlayMode(ViewerContext::PlayMode::Stop);
			}
		}
		else
		{
			if (ImGui::Button("Play"))
			{
				m_context.SetPlayMode(ViewerContext::PlayMode::PlayStart);
			}
		}
		if (ImGui::Button("Next Frame"))
		{
			m_context.SetPlayMode(ViewerContext::PlayMode::NextFrame);
		}
		if (ImGui::Button("Prev Frame"))
		{
			m_context.SetPlayMode(ViewerContext::PlayMode::PrevFrame);
		}
		if (ImGui::Button("Reset Anim"))
		{
			ResetAnimation();
		}
		if (ImGui::Button("Init Anim"))
		{
			InitializeAnimation();
		}
		if (ImGui::Button("Clear Animation"))
		{
			if (m_selectedModelDrawer != nullptr)
			{
				ClearAnimation(m_selectedModelDrawer.get());
			}
		}
		if (ImGui::Button("Clear All Animation"))
		{
			ClearSceneAnimation();
			for (auto& modelDrawer : m_modelDrawers)
			{
				ClearAnimation(modelDrawer.get());
			}
			InitializeAnimation();
			InitializeScene();
		}

		ImGui::PopID();
	}

	void Viewer::DrawCameraCtrl()
	{
		ImGui::PushID("Camera Control");

		auto cam = &m_context.m_camera;
		auto eyePos = cam->GetEyePostion();
		auto up = cam->GetUp();
		auto forward = cam->GetForward();
		auto fov = glm::degrees(cam->GetFovY());
		auto nearClip = cam->GetNearClip();
		auto farClip = cam->GetFarClip();

		ImGui::InputFloat3("Position", &eyePos[0], -1, ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Up", &up[0], -1, ImGuiInputTextFlags_ReadOnly);
		ImGui::InputFloat3("Forward", &forward[0], -1, ImGuiInputTextFlags_ReadOnly);
		if (ImGui::SliderFloat("Fov", &fov, 0.1f, 180.0f))
		{
			cam->SetFovY(glm::radians(fov));
		}
		if (ImGui::InputFloat("Near Clip", &nearClip))
		{
			cam->SetClip(nearClip, farClip);
		}
		if (ImGui::InputFloat("Far Clip", &farClip))
		{
			cam->SetClip(nearClip, farClip);
		}

		ImGui::PopID();
	}

	void Viewer::DrawShadowCtrl()
	{
		ImGui::PushID("Shadow Control");

		bool shadowEnable = m_context.IsShadowEnabled();
		if (ImGui::Checkbox("Use Shadow", &shadowEnable))
		{
			m_context.EnableShadow(shadowEnable);
		}

		auto shadowMap = &m_context.m_shadowmap;
		auto cam = &m_context.m_camera;

		float nearClip = shadowMap->GetNearClip();
		float farClip = shadowMap->GetFarClip();
		if (ImGui::InputFloat("Near Clip", &nearClip, cam->GetNearClip(), cam->GetFarClip()))
		{
			if (nearClip < farClip)
			{
				shadowMap->SetClip(nearClip, farClip);
			}
		}
		if (ImGui::InputFloat("Far Clip", &farClip, cam->GetNearClip(), cam->GetFarClip()))
		{
			if (nearClip < farClip)
			{
				shadowMap->SetClip(nearClip, farClip);
			}
		}

		float bias = shadowMap->GetBias();
		if (ImGui::SliderFloat("Bias", &bias, 0.0f, 1.0f))
		{
			shadowMap->SetBias(bias);
		}

		int width = shadowMap->GetWidth();
		int height = shadowMap->GetHeight();
		int size[2] = { width, height };
		if (ImGui::InputInt2("Texture Size", size))
		{
			glm::ivec2 texSize(size[0], size[1]);
			texSize = glm::max(glm::ivec2(1), texSize);
			texSize = glm::min(glm::ivec2(8192), texSize);
			shadowMap->Setup(texSize.x, texSize.y, 4);
		}

		auto mmdShadowColor = m_context.GetMMDGroundShadowColor();
		if (ImGui::ColorEdit4("MMD Ground Shadow Color", &mmdShadowColor[0]))
		{
			m_context.SetMMDGroundShadowColor(mmdShadowColor);
		}

		ImGui::PopID();
	}

	namespace
	{
		ImVec2 WorldToScreen(const glm::vec3& pos, const glm::mat4& m)
		{
			ImGuiIO& io = ImGui::GetIO();

			auto screenPos = m * glm::vec4(pos, 1.0f);
			screenPos *= 0.5f / screenPos.w;
			screenPos += glm::vec4(0.5f, 0.5f, 0.0f, 0.0f);
			screenPos.y = 1.0f - screenPos.y;
			screenPos.x *= io.DisplaySize.x;
			screenPos.y *= io.DisplaySize.y;
			return ImVec2(screenPos.x, screenPos.y);
		}
	}

	void Viewer::DrawLightCtrl()
	{
		ImGui::PushID("Light Control");

		glm::vec3 lightDir = m_context.GetLight()->GetLightDirection();
		glm::vec3 lightColor = m_context.GetLight()->GetLightColor();
		if (ImGui::InputFloat3("Direction", &lightDir[0]))
		{
			glm::quat rot(glm::vec3(0, 0, -1), glm::normalize(lightDir));
			m_lgihtManipMat = glm::translate(glm::mat4(1), m_lightManipPos) * glm::mat4_cast(rot);
		}
		ImGui::ColorEdit3("Color", &lightColor[0]);
		if (ImGui::Checkbox("Use Manip", &m_enableLightManip))
		{
			if (m_enableLightManip)
			{
				// 別のマニピュレーターが動いていたら OFF
				m_enableManip = false;

				glm::quat rot(glm::vec3(0, 0, -1), glm::normalize(lightDir));
				m_lgihtManipMat = glm::translate(glm::mat4(1), m_lightManipPos) * glm::mat4_cast(rot);
			}
			else
			{
				m_enableLightGuide = false;
			}
		}
		if (ImGui::Checkbox("Light Guide", &m_enableLightGuide))
		{
			glm::quat rot(glm::vec3(0, 0, -1), glm::normalize(lightDir));
			m_lgihtManipMat = glm::translate(glm::mat4(1), m_lightManipPos) * glm::mat4_cast(rot);
		}

		if (m_enableLightManip)
		{
			m_enableLightGuide = true;

			if (ImGui::RadioButton("Translate", m_lightManipOp == ImGuizmo::TRANSLATE))
			{
				m_lightManipOp = ImGuizmo::TRANSLATE;
			}
			if (ImGui::RadioButton("Rotate", m_lightManipOp == ImGuizmo::ROTATE))
			{
				m_lightManipOp = ImGuizmo::ROTATE;
			}
			const auto& view = m_context.GetCamera()->GetViewMatrix();
			const auto& proj = m_context.GetCamera()->GetProjectionMatrix();

			ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
			ImGuizmo::Manipulate(
				&view[0][0],
				&proj[0][0],
				m_lightManipOp,
				ImGuizmo::LOCAL,
				&m_lgihtManipMat[0][0]
			);

			glm::vec3 t, r, s;
			ImGuizmo::DecomposeMatrixToComponents(&m_lgihtManipMat[0][0], &t[0], &r[0], &s[0]);

			lightDir = glm::mat3(m_lgihtManipMat) * glm::vec3(0, 0, -1);
			m_lightManipPos = t;
		}

		Light light;
		light.SetLightDirection(lightDir);
		light.SetLightColor(lightColor);
		m_context.SetLight(light);

		ImGui::PopID();
	}

	void Viewer::DrawLightGuide()
	{
		if (m_enableLightGuide)
		{
			glm::vec3 lightDir = m_context.GetLight()->GetLightDirection();
			const auto& view = m_context.GetCamera()->GetViewMatrix();
			const auto& proj = m_context.GetCamera()->GetProjectionMatrix();

			const ImU32 flags = ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoInputs |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoBringToFrontOnFocus;
			ImGui::Begin("LightDir", nullptr, flags);
			auto drawList = ImGui::GetWindowDrawList();
			ImGui::End();
			auto wvp = proj * view * m_lgihtManipMat;
			auto startPos = WorldToScreen(glm::vec3(0), wvp);
			auto endPos = WorldToScreen(glm::vec3(0, 0, -m_sceneUnitScale), wvp);
			auto axEndoPos = WorldToScreen(glm::vec3(m_sceneUnitScale * 0.1f, 0, 0), wvp);
			auto ayEndoPos = WorldToScreen(glm::vec3(0, m_sceneUnitScale * 0.1f, 0), wvp);
			auto azEndoPos = WorldToScreen(glm::vec3(0, 0, m_sceneUnitScale * 0.1f), wvp);

			drawList->AddLine(startPos, endPos, ImColor(1.0f, 0.9f, 0.1f));
			drawList->AddLine(startPos, axEndoPos, ImColor(1.0f, 0.0f, 0.0f));
			drawList->AddLine(startPos, ayEndoPos, ImColor(0.0f, 1.0f, 0.0f));
			drawList->AddLine(startPos, azEndoPos, ImColor(0.0f, 0.0f, 1.8f));
		}
	}

	void Viewer::DrawModelCtrl()
	{
		ImGui::PushID("Model Control");

		if (m_selectedModelDrawer != nullptr)
		{
			m_selectedModelDrawer->DrawUI(&m_context);
		}

		ImGui::PopID();
	}

	void Viewer::DrawBGCtrl()
	{
		ImGui::PushID("BG Control");

		ImGui::ColorEdit3("BG Color1", &m_bgColor1[0]);
		ImGui::ColorEdit3("BG Color2", &m_bgColor2[0]);
		if (ImGui::Button("Reset BG Color"))
		{
			m_bgColor1 = DefaultBGColor1;
			m_bgColor2 = DefaultBGColor2;
		}

		ImGui::PopID();
	}

	void Viewer::UpdateAnimation()
	{
		double animTime = m_context.GetAnimationTime();
		switch (m_context.GetPlayMode())
		{
		case ViewerContext::PlayMode::None:
			break;
		case ViewerContext::PlayMode::PlayStart:
			m_context.SetAnimationTime(animTime);
			m_context.SetPlayMode(ViewerContext::PlayMode::Play);
			break;
		case ViewerContext::PlayMode::Play:
			m_context.SetAnimationTime(animTime + m_context.GetElapsed());
			break;
		case ViewerContext::PlayMode::Stop:
			m_context.SetAnimationTime(animTime);
			break;
		case ViewerContext::PlayMode::Update:
			m_context.SetAnimationTime(animTime);
			break;
		case ViewerContext::PlayMode::NextFrame:
			m_context.SetAnimationTime(animTime + 1.0f / m_animCtrlEditFPS);
			m_context.SetPlayMode(ViewerContext::PlayMode::Update);
			break;
		case ViewerContext::PlayMode::PrevFrame:
			m_context.SetAnimationTime(animTime - 1.0f / m_animCtrlEditFPS);
			m_context.SetPlayMode(ViewerContext::PlayMode::Update);
			break;
		default:
			break;
		}
	}

	void Viewer::InitializeAnimation()
	{
		m_context.SetAnimationTime(0);
		ResetAnimation();
	}

	void Viewer::ResetAnimation()
	{
		m_context.SetPlayMode(ViewerContext::PlayMode::None);
		for (auto& modelDrawer : m_modelDrawers)
		{
			modelDrawer->ResetAnimation(&m_context);
		}
	}

	void Viewer::RegisterCommand()
	{
		using Args = std::vector<std::string>;
		m_commands.emplace_back(Command{ "open", [this](const Args& args) { return CmdOpen(args); } });
		m_commands.emplace_back(Command{ "clear", [this](const Args& args) { return CmdClear(args); } });
		m_commands.emplace_back(Command{ "play", [this](const Args& args) { return CmdPlay(args); } });
		m_commands.emplace_back(Command{ "stop", [this](const Args& args) { return CmdStop(args); } });
		m_commands.emplace_back(Command{ "select", [this](const Args& args) { return CmdSelect(args); } });
		m_commands.emplace_back(Command{ "translate", [this](const Args& args) { return CmdTranslate(args); } });
		m_commands.emplace_back(Command{ "rotate", [this](const Args& args) { return CmdRotate(args); } });
		m_commands.emplace_back(Command{ "scale", [this](const Args& args) { return CmdScale(args); } });
		m_commands.emplace_back(Command{ "refreshCustomCommand", [this](const Args& args) { return CmdRefreshCustomCommand(args); } });
		m_commands.emplace_back(Command{ "enableUI", [this](const Args& args) { return CmdEnableUI(args); } });
		m_commands.emplace_back(Command{ "clearAnimation", [this](const Args& args) { return CmdClearAnimation(args); } });
		m_commands.emplace_back(Command{ "clearSceneAnimation", [this](const Args& args) { return CmdClearSceneAnimation(args); } });
		m_commands.emplace_back(Command{ "setMMDConfig", [this](const Args& args) { return CmdSetMMDConfig(args); } });
		m_commands.emplace_back(Command{ "setMSAA", [this](const Args& args) {return CmdSetMSAA(args); } });
	}

	void Viewer::RefreshCustomCommand()
	{
		std::string workDir = m_context.GetWorkDir();
		std::string cmdLuaPath = PathUtil::Combine(workDir, "command.lua");
		File cmdLuaFile;
		if (cmdLuaFile.Open(cmdLuaPath))
		{
			std::vector<char> cmdText;
			if (cmdLuaFile.ReadAll(&cmdText))
			{
				m_customCommands.clear();
				m_customCommandMenuItemRoot.m_items.clear();
				m_lua = std::make_unique<sol::state>();
				m_lua->open_libraries(sol::lib::base, sol::lib::package);

				(*m_lua)["RegisterCommand"] = [this](
					const std::string& name,
					sol::object func,
					const std::string& menuName
					)
				{
					std::string cmdName = name;
					if (name.empty())
					{
						cmdName = "@unnamed_" + std::to_string(m_customCommands.size());
					}
					if (func.is<sol::function>())
					{
						// Look for a registerd command with same name.
						bool findRegCmd = m_commands.end() != std::find_if(
							m_commands.begin(),
							m_commands.end(),
							[&cmdName](const Command& regCmd) { return regCmd.m_name == cmdName; }
						);
						if (findRegCmd)
						{
							SABA_WARN("RegisterCommand : [{}] is registered command.", cmdName);
							return;
						}

						// Look for a custom command with same name.
						bool findCutomCmd = m_customCommands.end() != std::find_if(
							m_customCommands.begin(),
							m_customCommands.end(),
							[&cmdName](const CustomCommandPtr& customCmd) { return customCmd->m_name == cmdName; }
						);
						if (findCutomCmd)
						{
							SABA_WARN("RegisterCommand : [{}] is already exists.", cmdName);
							return;
						}

						// Register custom command.
						std::unique_ptr<CustomCommand> cmd = std::make_unique<CustomCommand>();
						cmd->m_name = cmdName;
						cmd->m_commandFunc = func;
						cmd->m_menuName = menuName;
						if (!cmdName.empty())
						{
							size_t offset = 0;
							auto* curMenuItem = &m_customCommandMenuItemRoot;
							while (offset < menuName.size())
							{
								auto findIdx = menuName.find('/', offset);
								std::string itemName = menuName.substr(offset, findIdx - offset);
								if (!itemName.empty())
								{
									// Add sub menu entry.
									(*curMenuItem)[itemName].m_name = itemName;
									(*curMenuItem)[itemName].m_command = nullptr;
									curMenuItem = &((*curMenuItem)[itemName]);
								}
								if (findIdx == std::string::npos)
								{
									break;
								}
								offset = findIdx + 1;
							}
							curMenuItem->m_command = cmd.get();
						}
						m_customCommands.emplace_back(std::move(cmd));
					}
					else
					{
						SABA_WARN("RegisterCommand : func is not function.");
					}
				};

				(*m_lua)["ExecuteCommand"] = [this](const std::string&	cmd, sol::object args)
				{
					ViewerCommand viewerCmd;
					viewerCmd.SetCommand(cmd);
					if (args.valid())
					{
						if (args.is<sol::table>())
						{
							sol::table argsTable = args;
							for (auto argIt = argsTable.begin(); argIt != argsTable.end(); ++argIt)
							{
								auto arg = (*argIt).second.as<std::string>();
								viewerCmd.AddArg(arg);
							}
						}
						else
						{
							viewerCmd.AddArg(args.as<std::string>());
						}
					}
					return ExecuteCommand(viewerCmd);
				};

				sol::load_result cmd = m_lua->load_buffer(cmdText.data(), cmdText.size(), "command.lua");
				if (cmd.valid())
				{
					try
					{
						cmd();
					}
					catch (sol::error e)
					{
						SABA_ERROR("command.lua execute fail.\n{}", e.what());
					}
				}
				else
				{
					std::string errorMessage = cmd;
					SABA_ERROR("command.lua load fail.\n{}", errorMessage);
				}
			}
			else
			{
				SABA_ERROR("command.lua read fail.");
			}
		}
		else
		{
			SABA_INFO("command.lua not found.");
		}
	}

	bool Viewer::ExecuteCommand(const ViewerCommand & cmd)
	{
		// Register Command
		{
			auto findIt = std::find_if(
				m_commands.begin(),
				m_commands.end(),
				[&cmd](const Command& regCmd) { return cmd.GetCommand() == regCmd.m_name; }
			);
			if (findIt != m_commands.end())
			{
				SABA_INFO("CMD {} Execute", cmd.GetCommand());
				if ((*findIt).m_commandFunc(cmd.GetArgs()))
				{
					SABA_INFO("CMD {} Succeeded.", cmd.GetCommand());
					return true;
				}
				else
				{
					SABA_INFO("CMD {} Failed.", cmd.GetCommand());
					return false;
				}
			}
		}

		// Custom Command
		{
			auto findIt = std::find_if(
				m_customCommands.begin(),
				m_customCommands.end(),
				[&cmd](const CustomCommandPtr& customCmd) { return cmd.GetCommand() == customCmd->m_name; }
			);
			if (findIt != m_customCommands.end())
			{
				const auto& args = cmd.GetArgs();
				sol::table argsTable = m_lua->create_table(args.size(), 0);
				for (const auto& arg : args)
				{
					argsTable.add(arg);
				}
				try
				{
					sol::function_result ret = (*findIt)->m_commandFunc(argsTable);
					return ret.valid();
				}
				catch (sol::error e)
				{
					SABA_WARN("lua error\n{}", e.what());
					return false;
				}
			}
		}

		SABA_INFO("Unknown Command. [{}]", cmd.GetCommand());
		return false;
	}

	bool Viewer::CmdOpen(const std::vector<std::string>& args)
	{
		if (args.empty())
		{
			SABA_INFO("Cmd Open Args Empty.");
			return false;
		}

		std::string filepath = args[0];
		std::string ext = PathUtil::GetExt(filepath);
		SABA_INFO("Open File. [{}]", filepath);
		if (ext == "obj")
		{
			if (!LoadOBJFile(filepath))
			{
				return false;
			}
		}
		else if (ext == "pmd")
		{
			InitializeAnimation();
			if (!LoadPMDFile(filepath))
			{
				return false;
			}
		}
		else if (ext == "vmd")
		{
			InitializeAnimation();
			if (!LoadVMDFile(filepath))
			{
				return false;
			}
		}
		else if (ext == "pmx")
		{
			InitializeAnimation();
			if (!LoadPMXFile(filepath))
			{
				return false;
			}
		}
		else if (ext == "vpd")
		{
			if (!LoadVPDFile(filepath))
			{
				return false;
			}
		}
		else if (ext == "x")
		{
			if (!LoadXFile(filepath))
			{
				return false;
			}
		}
		else
		{
			SABA_INFO("Unknown File Ext [{}]", ext);
			return false;
		}


		return true;
	}

	bool Viewer::CmdClear(const std::vector<std::string>& args)
	{
		if (args.empty())
		{
			// 引数が空の場合は選択中のモデルを消す
			if (m_selectedModelDrawer == nullptr)
			{
				SABA_INFO("Cmd Clear : Selected model is null.");
				return false;
			}
			else
			{
				auto removeIt = std::remove(
					m_modelDrawers.begin(),
					m_modelDrawers.end(),
					m_selectedModelDrawer
				);
				m_modelDrawers.erase(removeIt, m_modelDrawers.end());
				m_selectedModelDrawer = nullptr;
			}
		}
		else
		{
			if (args[0] == "-all")
			{
				m_selectedModelDrawer = nullptr;
				m_modelDrawers.clear();
				m_cameraOverrider.reset();

				InitializeScene();
			}
		}

		InitializeAnimation();

		return true;
	}

	bool Viewer::CmdPlay(const std::vector<std::string>& args)
	{
		for (auto& modelDrawer : m_modelDrawers)
		{
			modelDrawer->Play();
		}

		m_context.SetPlayMode(ViewerContext::PlayMode::PlayStart);

		return true;
	}

	bool Viewer::CmdStop(const std::vector<std::string>& args)
	{
		for (auto& modelDrawer : m_modelDrawers)
		{
			modelDrawer->Stop();
		}

		m_context.SetPlayMode(ViewerContext::PlayMode::Stop);

		return true;
	}

	bool Viewer::CmdSelect(const std::vector<std::string>& args)
	{
		if (args.empty())
		{
			SABA_INFO("Cmd Select : Model name is empty");
			return false;
		}

		auto findModelDrawer = FindModelDrawer(args[0]);
		if (findModelDrawer == nullptr)
		{
			SABA_INFO("Cmd Select : Model Not Found. [{}]", args[0]);
			return false;
		}
		m_selectedModelDrawer = findModelDrawer;
		return true;
	}

	namespace
	{
		bool ToInt(const std::vector<std::string>& args, size_t offset, int* outVal)
		{
			if (outVal == nullptr)
			{
				return false;
			}

			if (args.size() < offset + 1)
			{
				return false;
			}

			size_t outPos;
			int temp = std::stoi(args[offset], &outPos);
			if (outPos != args[offset].size())
			{
				return false;
			}

			*outVal = temp;

			return true;
		}

		bool ToFloat(const std::vector<std::string>& args, size_t offset, float* outVal)
		{
			if (outVal == nullptr)
			{
				return false;
			}

			if (args.size() < offset + 1)
			{
				return false;
			}

			size_t outPos;
			float temp = std::stof(args[offset], &outPos);
			if (outPos != args[offset].size())
			{
				return false;
			}

			*outVal = temp;

			return true;
		}

		bool ToVec3(const std::vector<std::string>& args, size_t offset, glm::vec3* outVec)
		{
			if (outVec == nullptr)
			{
				return false;
			}

			if (args.size() < offset + 3)
			{
				return false;
			}

			glm::vec3 temp;
			if (!ToFloat(args, offset + 0, &temp.x) ||
				!ToFloat(args, offset + 1, &temp.y) ||
				!ToFloat(args, offset + 2, &temp.z)
				)
			{
				return false;
			}

			*outVec = temp;
			return true;
		}

		bool ToBool(const std::vector<std::string>& args, size_t offset, bool* outVal)
		{
			if (outVal == nullptr)
			{
				return false;
			}

			if (args.size() < offset + 1)
			{
				return false;
			}

			const auto& arg = args[offset];
			if (arg == "true") { *outVal = true; }
			else if (arg == "false") { *outVal = false; }
			else
			{
				int intVal;
				if (!ToInt(args, offset, &intVal)) { return false; }
				*outVal = !!intVal;
			}

			return true;
		}
	}

	bool Viewer::CmdTranslate(const std::vector<std::string>& args)
	{
		if (m_selectedModelDrawer == nullptr)
		{
			SABA_INFO("Cmd Translate : Selected model is null.");
			return false;
		}

		glm::vec3 translate;
		if (!ToVec3(args, 0, &translate))
		{
			SABA_INFO("Cmd Translate : Invalid Argument.");
			return false;
		}

		m_selectedModelDrawer->SetTranslate(translate);

		return true;
	}

	bool Viewer::CmdRotate(const std::vector<std::string>& args)
	{
		if (m_selectedModelDrawer == nullptr)
		{
			SABA_INFO("Cmd Rotate : Selected model is null.");
			return false;
		}

		glm::vec3 rotate;
		if (!ToVec3(args, 0, &rotate))
		{
			SABA_INFO("Cmd Rotate : Invalid Argument.");
			return false;
		}

		m_selectedModelDrawer->SetRotate(glm::radians(rotate));

		return true;
	}

	bool Viewer::CmdScale(const std::vector<std::string>& args)
	{
		if (m_selectedModelDrawer == nullptr)
		{
			SABA_INFO("Cmd Scale : Selected model is null.");
			return false;
		}

		glm::vec3 scale;
		if (!ToVec3(args, 0, &scale))
		{
			SABA_INFO("Cmd Scale : Invalid Argument.");
			return false;
		}

		m_selectedModelDrawer->SetScale(scale);

		return false;
	}

	bool Viewer::CmdRefreshCustomCommand(const std::vector<std::string>& args)
	{
		RefreshCustomCommand();

		return true;
	}

	bool Viewer::CmdEnableUI(const std::vector<std::string>& args)
	{
		if (args.empty())
		{
			m_context.EnableUI(true);
		}
		else
		{
			std::string enable = args[0];
			if (enable == "false")
			{
				m_context.EnableUI(false);
			}
			else
			{
				SABA_INFO("Unknown arg [{}]", args[0]);
				return false;
			}
		}

		return true;
	}

	bool Viewer::CmdClearAnimation(const std::vector<std::string>& args)
	{
		if (args.empty())
		{
			return ClearAnimation(m_selectedModelDrawer.get());
		}
		else
		{
			if (args[0] == "-all")
			{
				for (auto& modelDrawer : m_modelDrawers)
				{
					ClearAnimation(modelDrawer.get());
				}
			}
		}
		return true;
	}

	bool Viewer::CmdClearSceneAnimation(const std::vector<std::string>& args)
	{
		ClearSceneAnimation();
		InitializeScene();
		return true;
	}

	bool Viewer::CmdSetMMDConfig(const std::vector<std::string>& args)
	{
		if (args.empty())
		{
			SABA_INFO("Parallel : {}", m_mmdModelConfig.m_parallelUpdateCount);
		}
		auto argIt = args.begin();
		for (; argIt != args.end(); ++argIt)
		{
			if ((*argIt) == "-pallalel" || (*argIt) == "-p")
			{
				++argIt;
				if (argIt == args.end())
				{
					return false;
				}
				try
				{
					const size_t MaxParallelCount = std::max(size_t(std::thread::hardware_concurrency()), size_t(16));
					auto parallelCount = std::stoul(*argIt);
					if (parallelCount > MaxParallelCount)
					{
						SABA_WARN("parallel : 0 - {}", MaxParallelCount);
						return false;
					}
					m_mmdModelConfig.m_parallelUpdateCount = parallelCount;
					for (auto& modelDrawer : m_modelDrawers)
					{
						if (modelDrawer->GetType() == ModelDrawerType::MMDModelDrawer)
						{
							auto mmdModelDrawer = reinterpret_cast<GLMMDModelDrawer*>(modelDrawer.get());
							auto mmdModel = mmdModelDrawer->GetModel();
							mmdModel->GetMMDModel()->SetParallelUpdateHint(parallelCount);
						}
					}
				}
				catch (std::exception e)
				{
					SABA_WARN("exception : {}", e.what());
					return false;
				}
			}
			else
			{
				SABA_WARN("unknown arg : {}", *argIt);
				return false;
			}
		}
		return true;
	}

	bool Viewer::CmdSetMSAA(const std::vector<std::string>& args)
	{
		bool msaaEnable = true;
		int msaaCount = 4;

		ToBool(args, 0, &msaaEnable);
		ToInt(args, 1, &msaaCount);

		GLint maxSamples;
		glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
		if (msaaCount < 0 || msaaCount > maxSamples)
		{
			SABA_WARN("MSAA Max Samples {}", maxSamples);
			msaaCount = 4;
		}

		SABA_INFO("Set MSAA {} {}", msaaEnable, msaaCount);


		m_context.EnableMSAA(msaaEnable);
		m_context.SetMSAACount(msaaCount);

		return true;
	}

	bool Viewer::LoadOBJFile(const std::string & filename)
	{
		OBJModel objModel;
		if (!objModel.Load(filename.c_str()))
		{
			SABA_WARN("OBJ Load Fail.");
			return false;
		}

		auto glObjModel = std::make_shared<GLOBJModel>();
		if (!glObjModel->Create(objModel))
		{
			SABA_WARN("GLOBJModel Create Fail.");
			return false;
		}

		auto objDrawer = std::make_unique<GLOBJModelDrawer>(
			m_objModelDrawContext.get(),
			glObjModel
			);
		if (!objDrawer->Create())
		{
			SABA_WARN("GLOBJModelDrawer Create Fail.");
			return false;
		}
		m_modelDrawers.emplace_back(std::move(objDrawer));
		m_selectedModelDrawer = m_modelDrawers[m_modelDrawers.size() - 1];
		m_selectedModelDrawer->SetName(GetNewModelName());
		m_selectedModelDrawer->SetBBox(objModel.GetBBoxMin(), objModel.GetBBoxMax());

		InitializeScene();

		m_prevTime = GetTime();

		return true;
	}

	bool Viewer::LoadPMDFile(const std::string & filename)
	{
		std::shared_ptr<PMDModel> pmdModel = std::make_shared<PMDModel>();
		std::string mmdDataDir = PathUtil::Combine(
			m_context.GetResourceDir(),
			"mmd"
		);
		pmdModel->SetParallelUpdateHint(m_mmdModelConfig.m_parallelUpdateCount);
		if (!pmdModel->Load(filename, mmdDataDir))
		{
			SABA_WARN("PMD Load Fail.");
			return false;
		}

		std::shared_ptr<GLMMDModel> glMMDModel = std::make_shared<GLMMDModel>();
		if (!glMMDModel->Create(pmdModel))
		{
			SABA_WARN("GLMMDModel Create Fail.");
			return false;
		}

		auto mmdDrawer = std::make_unique<GLMMDModelDrawer>(
			m_mmdModelDrawContext.get(),
			glMMDModel
			);
		if (!mmdDrawer->Create())
		{
			SABA_WARN("GLMMDModelDrawer Create Fail.");
			return false;
		}
		m_modelDrawers.emplace_back(std::move(mmdDrawer));
		m_selectedModelDrawer = m_modelDrawers[m_modelDrawers.size() - 1];
		m_selectedModelDrawer->SetName(GetNewModelName());
		m_selectedModelDrawer->SetBBox(pmdModel->GetBBoxMin(), pmdModel->GetBBoxMax());

		InitializeScene();

		m_prevTime = GetTime();

		return true;
	}

	bool Viewer::LoadPMXFile(const std::string & filename)
	{
		std::shared_ptr<PMXModel> pmxModel = std::make_shared<PMXModel>();
		std::string mmdDataDir = PathUtil::Combine(
			m_context.GetResourceDir(),
			"mmd"
		);
		pmxModel->SetParallelUpdateHint(m_mmdModelConfig.m_parallelUpdateCount);
		if (!pmxModel->Load(filename, mmdDataDir))
		{
			SABA_WARN("PMD Load Fail.");
			return false;
		}

		std::shared_ptr<GLMMDModel> glMMDModel = std::make_shared<GLMMDModel>();
		if (!glMMDModel->Create(pmxModel))
		{
			SABA_WARN("GLMMDModel Create Fail.");
			return false;
		}

		auto mmdDrawer = std::make_unique<GLMMDModelDrawer>(
			m_mmdModelDrawContext.get(),
			glMMDModel
			);
		if (!mmdDrawer->Create())
		{
			SABA_WARN("GLMMDModelDrawer Create Fail.");
			return false;
		}
		m_modelDrawers.emplace_back(std::move(mmdDrawer));
		m_selectedModelDrawer = m_modelDrawers[m_modelDrawers.size() - 1];
		m_selectedModelDrawer->SetName(GetNewModelName());
		m_selectedModelDrawer->SetBBox(pmxModel->GetBBoxMin(), pmxModel->GetBBoxMax());

		InitializeScene();

		m_prevTime = GetTime();

		return true;
	}

	bool Viewer::LoadVMDFile(const std::string & filename)
	{
		GLMMDModel* mmdModel = nullptr;
		if (m_selectedModelDrawer != nullptr && m_selectedModelDrawer->GetType() == ModelDrawerType::MMDModelDrawer)
		{
			auto mmdModelDrawer = reinterpret_cast<GLMMDModelDrawer*>(m_selectedModelDrawer.get());
			mmdModel = mmdModelDrawer->GetModel();
		}
		if (mmdModel == nullptr)
		{
			SABA_INFO("MMD Model not selected.");
			return false;
		}

		VMDFile vmd;
		if (!ReadVMDFile(&vmd, filename.c_str()))
		{
			return false;
		}

		if (!vmd.m_cameras.empty())
		{
			auto vmdCamOverrider = std::make_unique<VMDCameraOverrider>();
			if (!vmdCamOverrider->Create(vmd))
			{
				return false;
			}
			m_cameraOverrider = std::move(vmdCamOverrider);
		}

		return mmdModel->LoadAnimation(vmd);
	}

	bool Viewer::LoadVPDFile(const std::string & filename)
	{
		GLMMDModel* mmdModel = nullptr;
		if (m_selectedModelDrawer != nullptr && m_selectedModelDrawer->GetType() == ModelDrawerType::MMDModelDrawer)
		{
			auto mmdModelDrawer = reinterpret_cast<GLMMDModelDrawer*>(m_selectedModelDrawer.get());
			mmdModel = mmdModelDrawer->GetModel();
			ClearAnimation(mmdModelDrawer);
		}
		if (mmdModel == nullptr)
		{
			SABA_INFO("MMD Model not selected.");
			return false;
		}

		VPDFile vpd;
		if (!ReadVPDFile(&vpd, filename.c_str()))
		{
			return false;
		}

		mmdModel->LoadPose(vpd);

		return true;
	}

	bool Viewer::LoadXFile(const std::string & filename)
	{
		XFileModel xfileModel;
		if (!xfileModel.Load(filename.c_str()))
		{
			SABA_WARN("Failed to load XFile.");
			return false;
		}

		auto glXFileModel = std::make_shared<GLXFileModel>();
		if (!glXFileModel->Create(&m_context, xfileModel))
		{
			SABA_WARN("Failed to create GLXFileModel.");
			return false;
		}

		auto xfileDrawer = std::make_unique<GLXFileModelDrawer>(
			m_xfileModelDrawContext.get(),
			glXFileModel
			);
		if (!xfileDrawer->Create())
		{
			SABA_WARN("Failed to create GLXFileModelDrawer.");
			return false;
		}
		m_modelDrawers.emplace_back(std::move(xfileDrawer));
		m_selectedModelDrawer = m_modelDrawers[m_modelDrawers.size() - 1];
		m_selectedModelDrawer->SetName(GetNewModelName());
		m_selectedModelDrawer->SetBBox(xfileModel.GetBBoxMin(), xfileModel.GetBBoxMax());

		InitializeScene();

		m_prevTime = GetTime();

		return true;
	}

	bool Viewer::ClearAnimation(ModelDrawer * modelDrawer)
	{
		GLMMDModel* mmdModel = nullptr;
		if (modelDrawer != nullptr && modelDrawer->GetType() == ModelDrawerType::MMDModelDrawer)
		{
			auto mmdModelDrawer = reinterpret_cast<GLMMDModelDrawer*>(modelDrawer);
			mmdModel = mmdModelDrawer->GetModel();
		}

		if (mmdModel != nullptr)
		{
			mmdModel->ClearAnimation();
		}

		return true;
	}

	bool Viewer::ClearSceneAnimation()
	{
		m_cameraOverrider.reset();
		return true;
	}

	bool Viewer::InitializeScene()
	{
		// Initialize BBox
		auto bboxMin = glm::vec3(0);
		auto bboxMax = glm::vec3(0);
		if (m_modelDrawers.empty())
		{
			bboxMin = glm::vec3(-1);
			bboxMax = glm::vec3(1);
		}
		else
		{
			bboxMin = m_modelDrawers[0]->GetBBoxMin();
			bboxMax = m_modelDrawers[0]->GetBBoxMax();
		}

		for (const auto& modelDrawer : m_modelDrawers)
		{
			bboxMin = glm::min(bboxMin, modelDrawer->GetBBoxMin());
			bboxMax = glm::max(bboxMax, modelDrawer->GetBBoxMax());
		}

		m_bboxMin = bboxMin;
		m_bboxMax = bboxMax;

		auto center = (bboxMax + bboxMin) * 0.5f;
		auto radius = glm::length(bboxMax - center);

		// Initialize Scene Setting
		if (m_initParam.m_initScene)
		{
			m_sceneUnitScale = m_initParam.m_initSceneUnitScale;
		}
		else
		{
			AdjustSceneUnitScale();
		}

		// Reset Grid
		if (!m_grid.Initialize(m_context, m_sceneUnitScale, 10, 5))
		{
			SABA_ERROR("grid Init Fail.");
			return false;
		}

		SABA_INFO("bbox [{}, {}, {}] - [{}, {}, {}] radisu [{}] grid [{}]",
			bboxMin.x, bboxMin.y, bboxMin.z,
			bboxMax.x, bboxMax.y, bboxMax.z,
			radius, m_sceneUnitScale);

		// Initialize Camera
		InitializeCamera();

		// Reset Shadow
		m_context.m_shadowmap.SetClip(
			m_context.m_camera.GetNearClip() * 10.0f, m_context.m_camera.GetFarClip() * 0.1f
		);

		return true;
	}

	void Viewer::InitializeCamera()
	{
		if (m_initParam.m_initCamera)
		{
			m_context.m_camera.Initialize(
				m_initParam.m_initCameraCenter,
				m_initParam.m_initCameraEye,
				m_initParam.m_initCameraNearClip,
				m_initParam.m_initCameraFarClip,
				m_initParam.m_initCameraRadius
			);
		}
		else
		{
			auto center = (m_bboxMax + m_bboxMin) * 0.5f;
			auto radius = glm::length(m_bboxMax - center);

			m_context.m_camera.Initialize(center, radius);
		}
	}

	void Viewer::AdjustSceneUnitScale()
	{
		auto center = (m_bboxMax + m_bboxMin) * 0.5f;
		auto radius = glm::length(m_bboxMax - center);

		float unitScale = 1.0f;
		if (radius < 1.0f)
		{
			while (!(unitScale <= radius && radius <= unitScale * 10.0f))
			{
				unitScale /= 10.0f;
			}
		}
		else
		{
			while (!(unitScale <= radius && radius <= unitScale * 10.0f))
			{
				unitScale *= 10.0f;
			}
		}

		m_sceneUnitScale = unitScale;
	}

	std::string Viewer::GetNewModelName()
	{
		std::string name;
		while (true)
		{
			std::stringstream ss;
			ss << "model_" << std::setfill('0') << std::setw(3) << m_modelNameID;
			m_modelNameID++;
			name = ss.str();
			if (FindModelDrawer(name) == nullptr)
			{
				break;
			}
		}
		return name;
	}

	Viewer::ModelDrawerPtr Viewer::FindModelDrawer(const std::string & name)
	{
		auto findIt = std::find_if(
			m_modelDrawers.begin(),
			m_modelDrawers.end(),
			[name](const ModelDrawerPtr& md) {return md->GetName() == name; }
		);
		if (findIt != m_modelDrawers.end())
		{
			return (*findIt);
		}
		return nullptr;
	}

	void Viewer::OnMouseButtonStub(GLFWwindow * window, int button, int action, int mods)
	{
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

		Viewer* viewer = (Viewer*)glfwGetWindowUserPointer(window);
		if (viewer != nullptr)
		{
			viewer->OnMouseButton(button, action, mods);
		}
	}

	void Viewer::OnMouseButton(int button, int action, int mods)
	{
		auto prevCameraMode = m_cameraMode;
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			m_cameraMode = CameraMode::Orbit;
			if (glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_PRESS)
			{
				m_cameraMode = CameraMode::Orbit;
			}
			else if (glfwGetKey(m_window, GLFW_KEY_X) == GLFW_PRESS)
			{
				m_cameraMode = CameraMode::Pan;
			}
			else if (glfwGetKey(m_window, GLFW_KEY_C) == GLFW_PRESS)
			{
				m_cameraMode = CameraMode::Dolly;
			}
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		{
			m_cameraMode = CameraMode::Dolly;
		}
		else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
		{
			m_cameraMode = CameraMode::Pan;
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT ||
			button == GLFW_MOUSE_BUTTON_RIGHT ||
			button == GLFW_MOUSE_BUTTON_MIDDLE
			)
		{
			if (action == GLFW_RELEASE)
			{
				m_cameraMode = CameraMode::None;
				m_mouseLockMode = MouseLockMode::None;
			}
		}

		if (prevCameraMode == CameraMode::None && m_cameraMode != CameraMode::None)
		{
			m_mouseLockMode = MouseLockMode::RequestLock;
		}
	}

	void Viewer::OnScrollStub(GLFWwindow * window, double offsetx, double offsety)
	{
		ImGui_ImplGlfw_ScrollCallback(window, offsetx, offsety);

		Viewer* viewer = (Viewer*)glfwGetWindowUserPointer(window);
		if (viewer != nullptr)
		{
			viewer->OnScroll(offsetx, offsety);
		}
	}

	void Viewer::OnScroll(double offsetx, double offsety)
	{
		m_mouse.SetScroll(offsetx, offsety);
	}

	void Viewer::OnKeyStub(GLFWwindow * window, int key, int scancode, int action, int mods)
	{
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

		Viewer* viewer = (Viewer*)glfwGetWindowUserPointer(window);
		if (viewer != nullptr)
		{
			viewer->OnKey(key, scancode, action, mods);
		}
	}

	void Viewer::OnKey(int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
		{
			m_context.EnableUI(!m_context.IsUIEnabled());
		}
	}

	void Viewer::OnCharStub(GLFWwindow * window, unsigned int codepoint)
	{
		ImGui_ImplGlfw_CharCallback(window, codepoint);

		Viewer* viewer = (Viewer*)glfwGetWindowUserPointer(window);
		if (viewer != nullptr)
		{
			viewer->OnChar(codepoint);
		}
	}

	void Viewer::OnChar(unsigned int codepoint)
	{
	}

	void Viewer::OnDropStub(GLFWwindow * window, int count, const char ** paths)
	{
		Viewer* viewer = (Viewer*)glfwGetWindowUserPointer(window);
		if (viewer != nullptr)
		{
			viewer->OnDrop(count, paths);
		}
	}

	void Viewer::OnDrop(int count, const char ** paths)
	{
		if (count > 0)
		{
			std::vector<std::string> args;
			for (int i = 0; i < count; i++)
			{
				SABA_INFO("Drop File. {}", paths[i]);
				args.emplace_back(paths[i]);
			}
			CmdOpen(args);
		}
	}

	void Viewer::Mouse::Initialize(GLFWwindow * window)
	{
		glfwGetCursorPos(window, &m_prevX, &m_prevY);
		m_dx = 0;
		m_dy = 0;

		m_saveScrollX = 0;
		m_saveScrollY = 0;
		m_scrollX = 0;
		m_scrollY = 0;
	}

	void Viewer::Mouse::Update(GLFWwindow * window)
	{
		int w, h;
		glfwGetWindowSize(window, &w, &h);

		double curX, curY;
		glfwGetCursorPos(window, &curX, &curY);

		m_dx = (curX - m_prevX) / (double)w;
		m_dy = (curY - m_prevY) / (double)h;
		m_prevX = curX;
		m_prevY = curY;

		m_scrollX = m_saveScrollX;
		m_scrollY = m_saveScrollY;
		m_saveScrollX = 0;
		m_saveScrollY = 0;
	}

	void Viewer::Mouse::SetScroll(double x, double y)
	{
		m_saveScrollX = x;
		m_saveScrollY = y;
	}

}
