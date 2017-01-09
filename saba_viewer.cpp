//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <Saba/Base/Log.h>

#include <Saba/Viewer/Viewer.h>

#include <picojson.h>
#include <fstream>

int main()
{
	SABA_INFO("Start");

	bool msaaEnable = false;
	int msaaCount = 4;
	{
		std::ifstream initJsonFile;
		initJsonFile.open("init.json");
		if (initJsonFile.is_open())
		{
			picojson::value val;
			initJsonFile >> val;
			initJsonFile.close();

			auto& init = val.get<picojson::object>();
			if (val.contains("MSAAEnable"))
			{
				msaaEnable = init["MSAAEnable"].get<bool>();
			}
			if (val.contains("MSAACount"))
			{
				double count = init["MSAACount"].get<double>();
				msaaCount = (int)(count + 0.5);
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

	int ret = viewer.Run();

	viewer.Uninitislize();

	SABA_INFO("Exit");

	saba::SingletonFinalizer::Finalize();

	return ret;
}
