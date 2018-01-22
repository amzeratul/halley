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

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

#pragma comment(lib, "windowsapp")


class WinRTDataReader : public ResourceDataReader {
public:

	WinRTDataReader(StorageFile&& _file)
		: file(std::move(_file))
	{
		stream = file.OpenReadAsync().get();
		open = true;
	}

	size_t size() const override
	{
		//return file.GetBasicPropertiesAsync().GetResults().Size();
		return stream.Size();
	}
	
	int read(gsl::span<gsl::byte> dst) override
	{
		Streams::DataReader reader(stream);
		size_t bytesLoaded = size_t(reader.LoadAsync(uint32_t(dst.size_bytes())).get());
		const auto dstData = reinterpret_cast<uint8_t*>(dst.data());
		reader.ReadBytes(array_view<uint8_t>(dstData, dstData + dst.size_bytes()));
		return int(bytesLoaded);
	}
	
	void seek(int64_t pos, int whence) override
	{
		int64_t basePos = 0;
		if (whence == SEEK_CUR) {
			basePos = tell();
		} else if (whence == SEEK_END) {
			basePos = size();
		}
		stream.Seek(pos);
	}
	
	size_t tell() const override
	{
		return stream.Position();
	}
	
	void close() override
	{
		open = false;
	}

private:
	StorageFile file;
	Streams::IRandomAccessStreamWithContentType stream;
	bool open = false;
};

class WinRTWindow : public Window
{
public:
	WinRTWindow(CoreWindow window, const WindowDefinition& definition)
		: window(window)
		, definition(definition)
	{}

	void update(const WindowDefinition& def) override
	{
		definition = def;
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
		return static_cast<winrt::Windows::Foundation::IUnknown*>(&window);
	}

	String getNativeHandleType() override
	{
		return "CoreWindow";
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
	return "Assets\\";
}

std::unique_ptr<ResourceDataReader> WinRTSystem::getDataReader(String path, int64_t start, int64_t end)
{
	try {
		StorageFolder assetFolder = Windows::ApplicationModel::Package::Current().InstalledLocation();
		StorageFile file = assetFolder.GetFileAsync(param::hstring(path.replaceAll("/", "\\").getUTF16().c_str())).get();
		if (file) {
			return std::make_unique<WinRTDataReader>(std::move(file));
		} else {
			return {};
		}
	} catch (hresult_error& e) {
		OutputDebugString(e.message().c_str());
		return {};
	} catch (...) {
		return {};
	}
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
