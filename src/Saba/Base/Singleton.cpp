//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "Singleton.h"

#include <cstdlib>

namespace saba
{
	namespace
	{
		const size_t MaxFinalizerNum = 256;
		size_t g_finalizerSize = 0;

		SingletonFinalizer::FinalizerFunc g_finalizers[MaxFinalizerNum];
	}

	void SingletonFinalizer::AddFinalizer(FinalizerFunc finalizer)
	{
		if (g_finalizerSize >= MaxFinalizerNum)
		{
			exit(-1);
		}

		g_finalizers[g_finalizerSize] = finalizer;
		g_finalizerSize += 1;
	}

	void SingletonFinalizer::Finalize()
	{
		for (size_t i = 0; i < g_finalizerSize; i++)
		{
			size_t idx = g_finalizerSize - i - 1;
			(g_finalizers[idx])();
		}
		g_finalizerSize = 0;
	}

}
