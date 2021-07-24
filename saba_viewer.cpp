//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <Saba/Base/Log.h>

#include <Saba/Viewer/Viewer.h>
#include <Saba/Viewer/ViewerCommand.h>

#include <nlohmann/json.hpp>
#include <sol.hpp>
#include <fstream>
#include <vector>

namespace
{
	/*
	@brief	Load initial setting from "init.json".
	*/
	void ReadInitParameterFromJson(
		saba::Viewer::InitializeParameter&	viewerInitParam,
		std::vector<saba::ViewerCommand>&	viewerCommands
	)
	{
		bool msaaEnable = false;
		int msaaCount = 4;
		std::ifstream initJsonFile;
		initJsonFile.open("init.json");
		if (initJsonFile.is_open())
		{
			nlohmann::json initJ;
			initJsonFile >> initJ;
			initJsonFile.close();

			if (initJ["MSAAEnable"].is_boolean())
			{
				viewerInitParam.m_msaaEnable = initJ["MSAAEnable"].get<bool>();
			}

			if (initJ["MSAACount"].is_number_integer())
			{
				viewerInitParam.m_msaaCount = initJ["MSAACount"].get<int>();
			}

			if (initJ["Commands"].is_array())
			{
				viewerCommands.clear();
				for (auto& commandJ : initJ["Commands"])
				{
					if (commandJ.is_object())
					{
						saba::ViewerCommand viewerCmd;
						if (commandJ["Cmd"].is_string())
						{
							viewerCmd.SetCommand(commandJ["Cmd"].get<std::string>());
						}
						if (commandJ["Args"].is_array())
						{
							for (auto& argJ : commandJ["Args"])
							{
								if (argJ.is_string())
								{
									viewerCmd.AddArg(argJ.get<std::string>());
								}
							}
						}
					}
				}
			}
		}
	}

	/*
	@brief	Load initial setting from "init.lua".
	*/
	void ReadInitParameterFromLua(
		const std::vector<std::string>&		args,
		saba::Viewer::InitializeParameter&	viewerInitParam,
		std::vector<saba::ViewerCommand>&	viewerCommands
	)
	{

		try
		{
			sol::state lua;
			lua.open_libraries(sol::lib::base, sol::lib::package);
			auto result = lua.load_file("init.lua");
			if (result)
			{
				sol::table argsTable = lua.create_table(args.size(), 0);
				for (const auto& arg : args)
				{
					argsTable.add(arg.c_str());
				}
				lua["Args"] = argsTable;

				result();

				auto msaa = lua["MSAA"];
				if (msaa)
				{
					viewerInitParam.m_msaaEnable = msaa["Enable"].get_or(false);
					viewerInitParam.m_msaaCount = msaa["Count"].get_or(4);
				}

				auto initCamera = lua["InitCamera"];
				if (initCamera)
				{
					bool useInitCamera = true;
					auto center = initCamera["Center"];
					auto eye = initCamera["Eye"];
					auto nearClip = initCamera["NearClip"];
					auto farClip = initCamera["FarClip"];
					auto radius = initCamera["Radius"];
					if (center.valid() &&
						eye.valid() &&
						nearClip.valid() &&
						farClip.valid() &&
						radius.valid()
						)
					{
						if (center["x"].valid() && center["y"].valid() && center["z"].valid())
						{
							viewerInitParam.m_initCameraCenter.x = center["x"].get<float>();
							viewerInitParam.m_initCameraCenter.y = center["y"].get<float>();
							viewerInitParam.m_initCameraCenter.z = center["z"].get<float>();
						}
						else
						{
							useInitCamera = false;
						}

						if (eye["x"].valid() && eye["y"].valid() && eye["z"].valid())
						{
							viewerInitParam.m_initCameraEye.x = eye["x"].get<float>();
							viewerInitParam.m_initCameraEye.y = eye["y"].get<float>();
							viewerInitParam.m_initCameraEye.z = eye["z"].get<float>();
						}
						else
						{
							useInitCamera = false;
						}

						viewerInitParam.m_initCameraNearClip = nearClip.get<float>();
						viewerInitParam.m_initCameraFarClip = farClip.get<float>();

						viewerInitParam.m_initCameraRadius = radius.get<float>();

						viewerInitParam.m_initCamera = useInitCamera;
					}

					auto initScene = lua["InitScene"];
					if (initScene)
					{
						auto unitScale = initScene["UnitScale"];
						if (unitScale.valid())
						{
							viewerInitParam.m_initSceneUnitScale = unitScale.get<float>();

							viewerInitParam.m_initScene = true;
						}
					}
				}

				sol::object commandsObj = lua["Commands"];
				if (commandsObj.is<sol::table>())
				{
					sol::table commands = commandsObj;
					viewerCommands.clear();
					for (auto cmdIt = commands.begin(); cmdIt != commands.end(); ++cmdIt)
					{
						sol::table cmd = (*cmdIt).second;
						saba::ViewerCommand viewerCmd;
						std::string cmdText = cmd["Cmd"].get_or(std::string(""));
						viewerCmd.SetCommand(cmdText);
						sol::object argsObj = cmd["Args"];
						if (argsObj.is<sol::table>())
						{
							sol::table args = argsObj;
							for (auto argIt = args.begin(); argIt != args.end(); ++argIt)
							{
								std::string argText = (*argIt).second.as<std::string>();
								viewerCmd.AddArg(argText);
							}
						}
						viewerCommands.emplace_back(viewerCmd);
					}
				}
			}
		}
		catch (sol::error e)
		{
			SABA_ERROR("Failed to load init.lua.\n{}", e.what());
		}
	}
} // namespace

int SabaViewerMain(const std::vector<std::string>& args)
{
	SABA_INFO("Start");
	saba::Viewer viewer;
	saba::Viewer::InitializeParameter	viewerInitParam;
	std::vector<saba::ViewerCommand>	viewerCommands;

	ReadInitParameterFromJson(viewerInitParam, viewerCommands);
	ReadInitParameterFromLua(args, viewerInitParam, viewerCommands);

	if (viewerInitParam.m_msaaEnable)
	{
		SABA_INFO("Enable MSAA");

		if (viewerInitParam.m_msaaCount != 2 &&
			viewerInitParam.m_msaaCount != 4 &&
			viewerInitParam.m_msaaCount != 8
			)
		{
			SABA_WARN("MSAA Count Force Change : 4");
			viewerInitParam.m_msaaCount = 4;
		}

		SABA_INFO("MSAA Count : {}", viewerInitParam.m_msaaCount);
	}
	else
	{
		SABA_INFO("Disable MSAA");
	}

	if (!viewer.Initialize(viewerInitParam))
	{
		return -1;
	}

	for (const auto& viewerCommand : viewerCommands)
	{
		viewer.ExecuteCommand(viewerCommand);
	}

	int ret = viewer.Run();

	viewer.Uninitislize();

	SABA_INFO("Exit");

	return ret;
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

	auto ret = SabaViewerMain(args);
	saba::SingletonFinalizer::Finalize();
	return ret;
}

