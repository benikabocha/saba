#include <Saba/Base/Log.h>

#include "Viewer.h"


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
