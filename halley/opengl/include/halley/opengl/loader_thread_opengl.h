#pragma once
#include <thread>
#include "halley/concurrency/executor.h"

namespace Halley
{
	class GLContext;

	class LoaderThreadOpenGL
	{
	public:
		explicit LoaderThreadOpenGL(GLContext& context);
		~LoaderThreadOpenGL();
		std::thread::id getThreadId();

	private:
		GLContext& parentContext;
		std::thread workerThread;
		Executor executor;
		
		std::unique_ptr<GLContext> context;
		
		void run();
		void initializeContext();
		void waitForGPU();
	};
}
