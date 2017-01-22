//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

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
#include <Saba/Model/MMD/SjisToUnicode.h>

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

#include <array>
#include <deque>
#include <sstream>
#include <iomanip>
#include <string>

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
		: m_msaaEnable(false)
		, m_msaaCount(4)
		, m_glfwInitialized(false)
		, m_window(nullptr)
		, m_uColor1(-1)
		, m_uColor2(-2)
		, m_prevTime(0)
		, m_frameBufferWidth(0)
		, m_frameBufferHeight(0)
		, m_modelNameID(1)
		, m_enableInfoUI(true)
		, m_enableLogUI(true)
		, m_enableCommandUI(true)
		, m_enableModelListUI(true)
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

	void Viewer::EnableMSAA(bool enable)
	{
		m_msaaEnable = enable;
	}

	void Viewer::SetMSAACount(int msaaCount)
	{
		m_msaaCount = msaaCount;
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
		if (m_msaaEnable)
		{
			m_context.EnableMSAA(true);
			glfwWindowHint(GLFW_SAMPLES, m_msaaCount);
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

		// MSAA の設定
		if (m_msaaEnable)
		{
			glEnable(GL_MULTISAMPLE);
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
			m_frameBufferWidth = w;
			m_frameBufferHeight = h;
			m_context.SetFrameBufferSize(w, h);
			int windowW, windowH;
			glfwGetWindowSize(m_window, &windowW, &windowH);
			m_context.SetWindowSize(windowW, windowH);

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
						ImGui::MenuItem("Model List", nullptr, &m_enableModelListUI);
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
				DrawUI();
				ImGui::Render();
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
			wcharTable.push_back((ImWchar)ConvertSjisToUnicode(ch));
		}
		for (int sjisFirst = SjisFirstBegin1; sjisFirst <= SjisFirstEnd1; sjisFirst++)
		{
			for (int sjisSecond = SjisSecondBegin; sjisSecond <= SjisSecondEnd; sjisSecond++)
			{
				int ch = (sjisFirst << 8) | sjisSecond;
				wcharTable.push_back((ImWchar)ConvertSjisToUnicode(ch));\
			}
		}
		for (int sjisFirst = SjisFirstBegin2; sjisFirst <= SjisFirstEnd2; sjisFirst++)
		{
			for (int sjisSecond = SjisSecondBegin; sjisSecond <= SjisSecondEnd; sjisSecond++)
			{
				int ch = (sjisFirst << 8) | sjisSecond;
				wcharTable.push_back((ImWchar)ConvertSjisToUnicode(ch));
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

		for (auto& modelDrawer : m_modelDrawers)
		{
			// DrawUI
			modelDrawer->DrawUI(&m_context);

			// Update
			modelDrawer->Update(&m_context);

			// Draw
			modelDrawer->Draw(&m_context);
		}
	}

	void Viewer::DrawUI()
	{
		DrawInfoUI();
		DrawLogUI();
		DrawCommandUI();
		DrawModelListUI();
	}

	void Viewer::DrawInfoUI()
	{
		if (!m_enableCommandUI)
		{
			return;
		}
		ImGui::SetNextWindowPos(ImVec2(10, 30));
		if (!ImGui::Begin("Info", &m_enableCommandUI, ImVec2(0, 0), 0.3f, ImGuiWindowFlags_NoTitleBar /*| ImGuiWindowFlags_NoResize*/ | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::End();
			return;
		}
		ImGui::Text("Info");
		ImGui::Separator();
		ImGui::Text("Time %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (m_selectedModelDrawer != nullptr && m_selectedModelDrawer->GetType() == ModelDrawerType::MMDModelDrawer)
		{
			auto mmdModelDrawer = reinterpret_cast<GLMMDModelDrawer*>(m_selectedModelDrawer.get());
			auto mmdModel = mmdModelDrawer->GetModel();
			if (mmdModel != nullptr)
			{
				ImGui::Text("MMD Model Update Time %.3f ms", mmdModel->GetUpdateTime() * 1000.0);
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

		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowPos(
			ImVec2(m_frameBufferWidth - width, m_frameBufferHeight - height - 80),
			ImGuiSetCond_FirstUseEver
		);
		ImGui::Begin("Log", &m_enableLogUI);
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

		float width = 500;
		float height = 0;
		std::array<char, 256> inputBuffer;
		inputBuffer.fill('\0');

		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiSetCond_FirstUseEver);
		ImGui::SetNextWindowPos(
			ImVec2(m_frameBufferWidth - width, m_frameBufferHeight - height - 60),
			ImGuiSetCond_FirstUseEver
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

	void Viewer::DrawModelListUI()
	{
		if (!m_enableModelListUI)
		{
			return;
		}

		float width = 200;
		float height = 100;

		ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiSetCond_Once);
		ImGui::SetNextWindowPos(ImVec2((float)m_frameBufferWidth - width, 20), ImGuiSetCond_Once);
		ImGui::Begin("Model List", &m_enableModelListUI);
		ImGui::BeginChild("models");

		for (const auto& modelDrawer : m_modelDrawers)
		{
			bool selected = modelDrawer == m_selectedModelDrawer;
			if (ImGui::Selectable(modelDrawer->GetName().c_str(), selected))
			{
				m_selectedModelDrawer = modelDrawer;
			}
		}

		ImGui::EndChild();
		ImGui::End();
	}

	bool Viewer::ExecuteCommand(const ViewerCommand & cmd)
	{
		if (strcmp("open", cmd.GetCommand().c_str()) == 0)
		{
			return CmdOpen(cmd.GetArgs());
		}
		else if (strcmp("clear", cmd.GetCommand().c_str()) == 0)
		{
			return CmdClear(cmd.GetArgs());
		}
		else if (strcmp("play", cmd.GetCommand().c_str()) == 0)
		{
			return CmdPlay(cmd.GetArgs());
		}
		else if (strcmp("stop", cmd.GetCommand().c_str()) == 0)
		{
			return CmdStop(cmd.GetArgs());
		}
		else if (strcmp("select", cmd.GetCommand().c_str()) == 0)
		{
			return CmdSelect(cmd.GetArgs());
		}
		else if (strcmp("translate", cmd.GetCommand().c_str()) == 0)
		{
			return CmdTranslate(cmd.GetArgs());
		}
		else if (strcmp("rotate", cmd.GetCommand().c_str()) == 0)
		{
			return CmdRotate(cmd.GetArgs());
		}
		else if (strcmp("scale", cmd.GetCommand().c_str()) == 0)
		{
			return CmdScale(cmd.GetArgs());
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

	bool Viewer::CmdClear(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Clear Execute.");

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

				AdjustSceneScale();
			}
		}

		SABA_INFO("Cmd Clear Succeeded.");

		return true;
	}

	bool Viewer::CmdPlay(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Play Execute.");

		if (args.empty())
		{
			// 引数が空の場合は選択中のモデルを再生
			if (m_selectedModelDrawer == nullptr)
			{
				SABA_INFO("Cmd Play : Selected model is null.");
				return false;
			}
			else
			{
				m_selectedModelDrawer->Play();
			}
		}
		else
		{
			if (args[0] == "-all")
			{
				for (auto& modelDrawer : m_modelDrawers)
				{
					modelDrawer->Play();
				}
			}
		}

		SABA_INFO("Cmd Play Succeeded.");

		return true;
	}

	bool Viewer::CmdStop(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Stop Execute.");

		if (args.empty())
		{
			// 引数が空の場合は選択中のモデルを再生
			if (m_selectedModelDrawer == nullptr)
			{
				SABA_INFO("Cmd Stop : Selected model is null.");
				return false;
			}
			else
			{
				m_selectedModelDrawer->Stop();
			}
		}
		else
		{
			if (args[0] == "-all")
			{
				for (auto& modelDrawer : m_modelDrawers)
				{
					modelDrawer->Stop();
				}
			}
		}

		SABA_INFO("Cmd Stop Succeeded.");

		return true;
	}

	bool Viewer::CmdSelect(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Select Execute.");

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

		SABA_INFO("Cmd Select Succeeded.");
		return true;
	}

	namespace
	{
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
	}

	bool Viewer::CmdTranslate(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Select Execute.");

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

		SABA_INFO("Cmd Translate Succeeded.");

		return true;
	}

	bool Viewer::CmdRotate(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Rotate Execute.");

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

		SABA_INFO("Cmd Rotate Succeeded.");

		return true;
	}

	bool Viewer::CmdScale(const std::vector<std::string>& args)
	{
		SABA_INFO("Cmd Scale Execute.");

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

		SABA_INFO("Cmd Scale Succeeded.");

		return false;
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

		AdjustSceneScale();

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
		m_modelDrawers.emplace_back(std::move(mmdDrawer));
		m_selectedModelDrawer = m_modelDrawers[m_modelDrawers.size() - 1];
		m_selectedModelDrawer->SetName(GetNewModelName());
		m_selectedModelDrawer->SetBBox(pmdModel->GetBBoxMin(), pmdModel->GetBBoxMax());

		AdjustSceneScale();

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

		AdjustSceneScale();

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

		return mmdModel->LoadAnimation(vmd);
	}

	bool Viewer::AdjustSceneScale()
	{
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

		return true;
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
