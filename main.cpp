//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include <Saba/Base/Log.h>

#include <Saba/Viewer/Viewer.h>

int main()
{
	SABA_INFO("Start");

	saba::Viewer viewer;

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
