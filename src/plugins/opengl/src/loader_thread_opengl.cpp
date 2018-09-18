#include "loader_thread_opengl.h"
#include "halley/core/api/system_api.h"
#include "halley_gl.h"
#include "halley/core/halley_core.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;

LoaderThreadOpenGL::LoaderThreadOpenGL(SystemAPI& system, GLContext& context)
	: executor(Executors::getVideoAux())
	, context(context.createSharedContext())
{
#if HAS_THREADS
	workerThread = system.createThread("OpenGL Loader", ThreadPriority::Normal, [this]() { run(); });
#endif
}

LoaderThreadOpenGL::~LoaderThreadOpenGL()
{	
#if HAS_THREADS
	executor.stop();
	workerThread.join();
	context.reset();
#endif
}

std::thread::id LoaderThreadOpenGL::getThreadId()
{
#if HAS_THREADS
	return workerThread.get_id();
#else
	return std::thread::id();
#endif
}

void LoaderThreadOpenGL::run()
{
#if HAS_THREADS
	context->bind();
	executor.runForever();
#endif
}
