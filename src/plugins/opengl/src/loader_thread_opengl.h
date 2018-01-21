#pragma once
#include <thread>
#include "halley/concurrency/executor.h"

namespace Halley
{
	class SystemAPI;
	class GLContext;

	class LoaderThreadOpenGL
	{
	public:
		explicit LoaderThreadOpenGL(SystemAPI& system, GLContext& context);
		~LoaderThreadOpenGL();
		std::thread::id getThreadId();

	private:
		std::thread workerThread;
		Executor executor;
		
		std::unique_ptr<GLContext> context;
		
		void run();
		void waitForGPU();
	};
}
