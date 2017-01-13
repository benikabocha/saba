//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <Saba/Base/Log.h>

#include <Saba/Viewer/Viewer.h>
#include <Saba/Viewer/ViewerCommand.h>

#include <picojson.h>
#include <fstream>
#include <vector>

int main()
{
	SABA_INFO("Start");

	bool msaaEnable = false;
	int msaaCount = 4;
	std::vector<saba::ViewerCommand> viewerCommands;
	{
		std::ifstream initJsonFile;
		initJsonFile.open("init.json");
		if (initJsonFile.is_open())
		{
			picojson::value val;
			initJsonFile >> val;
			initJsonFile.close();

			auto& init = val.get<picojson::object>();
			if (init["MSAAEnable"].is<bool>())
			{
				msaaEnable = init["MSAAEnable"].get<bool>();
			}
			if (init["MSAACount"].is<double>())
			{
				double count = init["MSAACount"].get<double>();
				msaaCount = (int)(count + 0.5);
			}
			if (init["Commands"].is<picojson::array>())
			{
				for (auto& command : init["Commands"].get<picojson::array>())
				{
					if (command.is<picojson::object>())
					{
						saba::ViewerCommand viewerCmd;
						auto& cmdObj = command.get<picojson::object>();
						if (cmdObj["Cmd"].is<std::string>())
						{
							viewerCmd.SetCommand(cmdObj["Cmd"].get<std::string>());
						}
						if (cmdObj["Args"].is<picojson::array>())
						{
							for (auto& arg : cmdObj["Args"].get<picojson::array>())
							{
								if (arg.is<std::string>())
								{
									viewerCmd.AddArg(arg.get<std::string>());
								}
							}
						}
						viewerCommands.emplace_back(std::move(viewerCmd));
					}
				}
			}
		}
	}

	saba::Viewer viewer;
	if (msaaEnable)
	{
		SABA_INFO("Enable MSAA");
		viewer.EnableMSAA(true);

		if (msaaCount != 2 && msaaCount != 4 && msaaCount != 8)
		{
			SABA_WARN("MSAA Count Force Change : 4");
			msaaCount = 4;
		}

		SABA_INFO("MSAA Count : {}", msaaCount);
		viewer.SetMSAACount(msaaCount);
	}

	if (!viewer.Initialize())
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

	saba::SingletonFinalizer::Finalize();

	return ret;
}
