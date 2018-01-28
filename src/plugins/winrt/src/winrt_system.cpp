#include "winrt_system.h"
#include <ppltasks.h>
using namespace Halley;

#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Storage.FileProperties.h"
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.UI.Core.h"
#include "winrt/Windows.System.Diagnostics.h"

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;

#pragma comment(lib, "windowsapp")

class StdioDataReader : public ResourceDataReader {
public:

	StdioDataReader(const String& path)
	{
		_wfopen_s(&fp, path.getUTF16().c_str(), L"rb");
	}

	~StdioDataReader()
	{
		StdioDataReader::close();
	}

	size_t size() const override
	{
		const long startPos = ftell(fp);
		fseek(fp, 0, SEEK_END);
		const size_t size = size_t(ftell(fp));
		fseek(fp, startPos, SEEK_SET);
		return size;
	}
	
	int read(gsl::span<gsl::byte> dst) override
	{
		return int(fread(dst.data(), 1, dst.size_bytes(), fp));
	}
	
	void seek(int64_t pos, int whence) override
	{
		fseek(fp, long(pos), whence);
	}
	
	size_t tell() const override
	{
		return size_t(ftell(fp));
	}
	
	void close() override
	{
		if (fp) {
			fclose(fp);
			fp = nullptr;
		}
	}

private:
	FILE* fp = nullptr;
};


class WinRTWindow : public Window
{
public:
	WinRTWindow(CoreWindow window, const WindowDefinition& definition)
		: window(window)
		, definition(definition.withSize(Vector2i(window.Bounds().Width, window.Bounds().Height)))
	{
		window.SizeChanged([=] (CoreWindow win, WindowSizeChangedEventArgs args)
		{
			notifySizeChange(Vector2i(args.Size().Width, args.Size().Height));
		});
	}

	void update(const WindowDefinition& def) override
	{
		//definition = def;
	}

	void show() override
	{
	}

	void hide() override
	{
	}

	void setVsync(bool vsync) override
	{
	}

	void swap() override
	{
	}

	Rect4i getWindowRect() const override
	{
		auto bounds = window.Bounds();
		return Rect4i(Rect4f(bounds.X, bounds.Y, bounds.Width, bounds.Height));
	}

	const WindowDefinition& getDefinition() const override
	{
		return definition;
	}

	void* getNativeHandle() override
	{
		return winrt::get_abi(window);
	}

	String getNativeHandleType() override
	{
		return "CoreWindow";
	}

	void notifySizeChange(Vector2i size)
	{
		definition = definition.withSize(size);
	}

private:
	CoreWindow window;
	WindowDefinition definition;
};


void WinRTSystem::init()
{
}

void WinRTSystem::deInit()
{
}

String WinRTSystem::getResourcesBasePath(const String& gamePath) const
{
	static auto path = String(Windows::ApplicationModel::Package::Current().InstalledLocation().Path().data()) + String("/Assets/");
	return path;
	//return "Assets\\";
}

std::unique_ptr<ResourceDataReader> WinRTSystem::getDataReader(String path, int64_t start, int64_t end)
{
	return std::make_unique<StdioDataReader>(path);
}

std::unique_ptr<GLContext> WinRTSystem::createGLContext()
{
	// Not supported
	return {};
}

std::shared_ptr<Window> WinRTSystem::createWindow(const WindowDefinition& window)
{
	return std::make_shared<WinRTWindow>(CoreWindow::GetForCurrentThread(), window);
}

void WinRTSystem::destroyWindow(std::shared_ptr<Window> window)
{
}

Vector2i WinRTSystem::getScreenSize(int n) const
{
	// TODO
	return Vector2i();
}

Rect4i WinRTSystem::getDisplayRect(int screen) const
{
	// TODO
	return Rect4i();
}

void WinRTSystem::showCursor(bool show)
{
	// TODO
}

Bytes WinRTSystem::getSaveData(SaveDataType type, const String& path)
{
	// TODO
	return {};
}

void WinRTSystem::setSaveData(SaveDataType type, const String& path, const Bytes& data)
{
	// TODO
}

std::vector<String> WinRTSystem::enumerateSaveData(SaveDataType type, const String& root)
{
	// TODO
	return {};
}

bool WinRTSystem::generateEvents(VideoAPI* video, InputAPI* input)
{
	CoreWindow window = CoreWindow::GetForCurrentThread();
	CoreDispatcher dispatcher = window.Dispatcher();
	dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
	
	return true;
}



struct View : implements<View, IFrameworkView>
{
	View(WinRTSystem& system, std::function<void()>&& runnable)
		: system(system)
		, runnable(std::move(runnable))
	{}

	~View()
	{
		OutputDebugString(L"Bye");
	}

	void Initialize(CoreApplicationView const &view)
	{
		OutputDebugString(L"Initialized");
	}

	void Load(hstring entryPoint)
	{
	}

	void Uninitialize()
	{
		OutputDebugString(L"Uninitialized");
	}

	void Run()
	{
		CoreWindow window = CoreWindow::GetForCurrentThread();
		window.Activate();
		CoreDispatcher dispatcher = window.Dispatcher();
		dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

		runnable();
	}

	void SetWindow(CoreWindow const & window)
	{

	}

private:
	WinRTSystem& system;
	std::function<void()> runnable;
};

struct Source : implements<Source, IFrameworkViewSource>
{
	Source(WinRTSystem& system, std::function<void()>&& runnable)
		: system(system)
		, runnable(std::move(runnable))
	{}

	IFrameworkView CreateView()
	{
		return make<View>(system, std::move(runnable));
	}

private:
	WinRTSystem& system;
	std::function<void()> runnable;
};

void WinRTSystem::runGame(std::function<void()> runnable)
{
	CoreApplication::Run(make<Source>(*this, std::move(runnable)));
}
