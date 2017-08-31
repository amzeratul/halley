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
	workerThread = system.createThread("GLLoader", [this]() { run(); });
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
	Concurrent::setThreadName("OpenGL loader");
	context->bind();
	executor.runForever();
#endif
}

void LoaderThreadOpenGL::waitForGPU()
{
	// See https://forums.libsdl.org/viewtopic.php?t=9036&sid=3374c819e18df779e17b4ce5a49fdd15
#if WITH_OPENGL
	GLsync fenceId = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	while (glClientWaitSync(fenceId, GL_SYNC_FLUSH_COMMANDS_BIT, GLuint64(5000000000)) == GL_TIMEOUT_EXPIRED) {}
	glDeleteSync(fenceId);
#endif
}
