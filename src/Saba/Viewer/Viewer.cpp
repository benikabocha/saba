#include "Viewer.h"

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
#include <Saba/Model/MMD/PMXModel.h>
#include <Saba/GL/Model/MMD/GLMMDModel.h>
#include <Saba/GL/Model/MMD/GLMMDModelDrawer.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include <array>
#include <deque>

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

			m_buffer.push_back(msg.formatted.str());
			m_added = true;
		}

		void flush() override
		{
		}

		const std::deque<std::string>& GetBuffer() const { return m_buffer; }

		bool IsAdded() const { return m_added; }
		void ClearAddedFlag() { m_added = false; }

	private:
		size_t					m_maxBufferSize;
		std::deque<std::string> m_buffer;
		bool					m_added;
	};

	Viewer::Viewer()
		: m_glfwInitialized(false)
		, m_enableInfoUI(true)
		, m_enableLogUI(true)
		, m_enableCommandUI(true)
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

	bool Viewer::Initialize()
	{
		auto logger = Singleton<saba::Logger>::Get();
		m_imguiLogSink = logger->AddSink<ImGUILogSink>();

		SABA_INFO("CurDir = {}", m_context.GetWorkDir());
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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
		ImGui_ImplGlfwGL3_Init(m_window, false);
		ImGuiIO& io = ImGui::GetIO();
		std::string fontDir = PathUtil::Combine(
			m_context.GetResourceDir(),
			"font"
		);
		std::string fontPath = PathUtil::Combine(
			fontDir,
			"mgenplus-1mn-bold.ttf"
		);
		io.Fonts->AddFontFromFileTTF(
			fontPath.c_str(),
			14.0f,
			nullptr,
			io.Fonts->GetGlyphRangesJapanese()
		);

		// gl3wの初期化
		if (gl3wInit() != 0)
		{
			SABA_ERROR("gl3w Init Fail.");
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
		m_context.GetCamera()->Initialize(glm::vec3(0), 10.0f);
		if (!m_grid.Initialize(m_context, 0.5f, 10, 5))
		{
			SABA_ERROR("grid Init Fail.");
			return false;
		}

		m_objModelDrawContext = std::make_unique<GLOBJModelDrawContext>(&m_context);
		m_mmdModelDrawContext = std::make_unique<GLMMDModelDrawContext>(&m_context);

		m_prevTime = GetTime();

		return true;
	}

	void Viewer::Uninitislize()
	{
		auto logger = Singleton<saba::Logger>::Get();
		logger->RemoveSink(m_imguiLogSink.get());
		m_imguiLogSink.reset();

		ImGui_ImplGlfwGL3_Shutdown();
	}

	int Viewer::Run()
	{

		while (!glfwWindowShouldClose(m_window))
		{
			ImGui_ImplGlfwGL3_NewFrame();

			m_mouse.Update(m_window);
			if (!ImGui::GetIO().WantCaptureMouse)
			{
				if (m_cameraMode == CameraMode::Orbit)
				{
					m_context.GetCamera()->Orbit((float)m_mouse.m_dx, (float)m_mouse.m_dy);
				}
				if (m_cameraMode == CameraMode::Dolly)
				{
					m_context.GetCamera()->Dolly((float)m_mouse.m_dx + (float)m_mouse.m_dy);
				}
				if (m_cameraMode == CameraMode::Pan)
				{
					m_context.GetCamera()->Pan((float)m_mouse.m_dx, (float)m_mouse.m_dy);
				}

				if (m_mouse.m_scrollY != 0)
				{
					m_context.GetCamera()->Dolly((float)m_mouse.m_scrollY * 0.1f);
				}
			}

			int w, h;
			glfwGetFramebufferSize(m_window, &w, &h);
			m_context.GetCamera()->SetSize((float)w, (float)h);
			m_context.GetCamera()->UpdateMatrix();

			glViewport(0, 0, w, h);

			if (m_context.IsUIEnabled())
			{
				if (ImGui::BeginMainMenuBar())
				{
					if (ImGui::BeginMenu("Window"))
					{
						ImGui::MenuItem("Info", nullptr, &m_enableInfoUI);
						ImGui::MenuItem("Log", nullptr, &m_enableLogUI);
						ImGui::MenuItem("Command", nullptr, &m_enableCommandUI);
						ImGui::EndMenu();
					}
					ImGui::EndMainMenuBar();
				}
			}

			double time = GetTime();
			double elapsed = time - m_prevTime;
			m_prevTime = time;
			m_context.SetElapsedTime(elapsed);
			Draw();

			if (m_context.IsUIEnabled())
			{
				DrawInfoUI();
				DrawLogUI();
				DrawCommandUI();
				ImGui::Render();
			}

			glfwSwapBuffers(m_window);
			glfwPollEvents();
		}

		return 0;
	}

	void Viewer::Draw()
	{
		glClear(GL_DEPTH_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(m_bgVAO);
		glUseProgram(m_bgProg);
		SetUniform(m_uColor1, glm::vec3(0.2f, 0.2f, 0.2f));
		SetUniform(m_uColor2, glm::vec3(0.4f, 0.4f, 0.4f));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glUseProgram(0);
		glBindVertexArray(0);

		glEnable(GL_DEPTH_TEST);

		auto world = glm::mat4(1.0);
		auto view = m_context.GetCamera()->GetViewMatrix();
		auto proj = m_context.GetCamera()->GetProjectionMatrix();
		auto wv = view * world;
		auto wvp = proj * view * world;
		auto wvit = glm::mat3(view * world);
		wvit = glm::inverse(wvit);
		wvit = glm::transpose(wvit);
		m_grid.SetWVPMatrix(wvp);
		m_grid.Draw();

		if (m_modelDrawer != nullptr)
		{
			m_modelDrawer->Draw(&m_context);
		}
	}

	void Viewer::DrawInfoUI()
	{
		if (!m_enableCommandUI)
		{
			return;
		}
		ImGui::SetNextWindowPos(ImVec2(10, 30));
		if (!ImGui::Begin("Info", &m_enableCommandUI, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar /*| ImGuiWindowFlags_NoResize*/ | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::End();
			return;
		}
		ImGui::Text("Info");
		ImGui::Separator();
		ImGui::Text("Time %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (m_mmdModel != nullptr)
		{
			ImGui::Text("MMD Model Update Time %.3f ms", m_mmdModel->GetUpdateTime() * 1000.0);
		}
		ImGui::End();
	}

	void Viewer::DrawLogUI()
	{
		if (!m_enableLogUI)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Log");
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		for (const auto& log : m_imguiLogSink->GetBuffer())
		{
			ImGui::TextUnformatted(log.c_str());
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

		std::array<char, 256> inputBuffer;
		inputBuffer.fill('\0');
		ImGui::Begin("Command");
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

	bool Viewer::ExecuteCommand(const ViewerCommand & cmd)
	{
		if (strcmp("open", cmd.GetCommand().c_str()) == 0)
		{
			return CmdOpen(cmd.GetArgs());
		}
		else
		{
			SABA_INFO("Unknown Command. [{}]", cmd.GetCommand());
			return false;
		}

		return true;
	}

	bool Viewer::CmdOpen(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Open Execute.");
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
			if (!LoadPMDFile(filepath))
			{
				return false;
			}
		}
		else if (ext == "vmd")
		{
			if (!LoadVMDFile(filepath))
			{
				return false;
			}
		}
		else if (ext == "pmx")
		{
			if (!LoadPMXFile(filepath))
			{
				return false;
			}
		}
		else
		{
			SABA_INFO("Unknown File Ext [{}]", ext);
			return false;
		}

		SABA_INFO("Cmd Open Succeeded.");

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
		m_modelDrawer = std::move(objDrawer);

		auto bboxMin = objModel.GetBBoxMin();
		auto bboxMax = objModel.GetBBoxMax();
		auto center = (bboxMax + bboxMin) * 0.5f;
		auto radius = glm::length(bboxMax - center);
		m_context.GetCamera()->Initialize(center, radius);

		// グリッドのスケールを調節する
		float gridSize = 1.0f;
		if (radius < 1.0f)
		{
			while (!(gridSize <= radius && radius <= gridSize * 10.0f))
			{
				gridSize /= 10.0f;
			}
		}
		else
		{
			while (!(gridSize <= radius && radius <= gridSize * 10.0f))
			{
				gridSize *= 10.0f;
			}
		}
		if (!m_grid.Initialize(m_context, gridSize, 10, 5))
		{
			SABA_ERROR("grid Init Fail.");
			return false;
		}

		SABA_INFO("radisu [{}] grid [{}]", radius, gridSize);

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
		m_modelDrawer = std::move(mmdDrawer);

		auto bboxMin = pmdModel->GetBBoxMin();
		auto bboxMax = pmdModel->GetBBoxMax();
		auto center = (bboxMax + bboxMin) * 0.5f;
		auto radius = glm::length(bboxMax - center);
		m_context.GetCamera()->Initialize(center, radius);

		// グリッドのスケールを調節する
		float gridSize = 1.0f;
		if (radius < 1.0f)
		{
			while (!(gridSize <= radius && radius <= gridSize * 10.0f))
			{
				gridSize /= 10.0f;
			}
		}
		else
		{
			while (!(gridSize <= radius && radius <= gridSize * 10.0f))
			{
				gridSize *= 10.0f;
			}
		}
		if (!m_grid.Initialize(m_context, gridSize, 10, 5))
		{
			SABA_ERROR("grid Init Fail.");
			return false;
		}

		SABA_INFO("radisu [{}] grid [{}]", radius, gridSize);

		m_mmdModel = glMMDModel;

		m_prevTime = GetTime();

		return false;
	}

	bool Viewer::LoadPMXFile(const std::string & filename)
	{
		std::shared_ptr<PMXModel> pmxModel = std::make_shared<PMXModel>();
		std::string mmdDataDir = PathUtil::Combine(
			m_context.GetResourceDir(),
			"mmd"
		);
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
		m_modelDrawer = std::move(mmdDrawer);

		auto bboxMin = pmxModel->GetBBoxMin();
		auto bboxMax = pmxModel->GetBBoxMax();
		auto center = (bboxMax + bboxMin) * 0.5f;
		auto radius = glm::length(bboxMax - center);
		m_context.GetCamera()->Initialize(center, radius);

		// グリッドのスケールを調節する
		float gridSize = 1.0f;
		if (radius < 1.0f)
		{
			while (!(gridSize <= radius && radius <= gridSize * 10.0f))
			{
				gridSize /= 10.0f;
			}
		}
		else
		{
			while (!(gridSize <= radius && radius <= gridSize * 10.0f))
			{
				gridSize *= 10.0f;
			}
		}
		if (!m_grid.Initialize(m_context, gridSize, 10, 5))
		{
			SABA_ERROR("grid Init Fail.");
			return false;
		}

		SABA_INFO("radisu [{}] grid [{}]", radius, gridSize);

		m_mmdModel = glMMDModel;

		m_prevTime = GetTime();

		return false;
	}

	bool Viewer::LoadVMDFile(const std::string & filename)
	{
		if (m_mmdModel == nullptr)
		{
			SABA_INFO("MMD Model is null.");
			return false;
		}

		VMDFile vmd;
		if (!ReadVMDFile(&vmd, filename.c_str()))
		{
			return false;
		}

		return m_mmdModel->LoadAnimation(vmd);
	}

	void Viewer::OnMouseButtonStub(GLFWwindow * window, int button, int action, int mods)
	{
		ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);

		Viewer* viewer = (Viewer*)glfwGetWindowUserPointer(window);
		if (viewer != nullptr)
		{
			viewer->OnMouseButton(button, action, mods);
		}
	}

	void Viewer::OnMouseButton(int button, int action, int mods)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			m_cameraMode = CameraMode::Orbit;
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
			}
		}
	}

	void Viewer::OnScrollStub(GLFWwindow * window, double offsetx, double offsety)
	{
		ImGui_ImplGlfwGL3_ScrollCallback(window, offsetx, offsety);

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
		ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);

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
		ImGui_ImplGlfwGL3_CharCallback(window, codepoint);

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

	Viewer::Mouse::Mouse()
		: m_dx(0)
		, m_dy(0)
	{
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
