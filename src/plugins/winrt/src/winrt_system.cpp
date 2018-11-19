#include "winrt_platform.h"
#include "xbl_manager.h"
#ifdef WINDOWS_STORE

#include "winrt_system.h"
using namespace Halley;

#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Diagnostics.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.FileProperties.h"
#include "winrt/Windows.Storage.Search.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.System.Diagnostics.h"
#include "winrt/Windows.UI.Core.h"
#include "winrt/Windows.UI.ViewManagement.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;

#pragma comment(lib, "windowsapp")

class StdioDataReader : public ResourceDataReader {
public:

	StdioDataReader(const String& path)
	{
		_wfopen_s(&fp, path.getUTF16().c_str(), L"rb");
		if (!fp) {
			throw Exception("Unable to load " + path, HalleyExceptions::File);
		}
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
		, definition(definition.withSize(Vector2i(Vector2f(window.Bounds().Width, window.Bounds().Height))))
	{
		window.SizeChanged([=] (CoreWindow win, WindowSizeChangedEventArgs args)
		{
			notifySizeChange(Vector2i(Vector2f(args.Size().Width, args.Size().Height)));
		});

		window.KeyDown([=] (CoreWindow win, KeyEventArgs args)
		{
			args.Handled(true);
		});

		window.KeyUp([=] (CoreWindow win, KeyEventArgs args)
		{
			args.Handled(true);
		});
	}

	void update(const WindowDefinition& def) override
	{
		auto curView = ApplicationView::GetForCurrentView();
		bool currentlyFullscreen = curView.IsFullScreenMode();
		bool wantsFullscreen = def.getWindowType() == WindowType::Fullscreen;
		if (currentlyFullscreen != wantsFullscreen) {
			if (wantsFullscreen) {
				curView.TryEnterFullScreenMode();
			} else {
				curView.ExitFullScreenMode();
			}
		}
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
		//return Rect4i(Vector2i(), Vector2i(1920, 1080));
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
		//definition = definition.withSize(Vector2i(1920, 1080));
	}

private:
	CoreWindow window;
	WindowDefinition definition;
};


void WinRTSystem::init()
{
	// Setup logging
	/*
	using namespace winrt::Windows::Foundation::Diagnostics;
	LoggingChannel channel(L"halleyLog", LoggingChannelOptions());
	FileLoggingSession session(L"halleyLog");
	session.AddLoggingChannel(channel);

	channel.LogMessage(L"Hello world!");
	session.CloseAndSaveToFileAsync();
	*/

	Logger::addSink(*this);
}

void WinRTSystem::deInit()
{
	Logger::removeSink(*this);
}

void WinRTSystem::log(LoggerLevel level, const String& msg)
{
#ifdef _DEBUG
	OutputDebugStringW((msg + "\n").getUTF16().c_str());
#endif
}

Path WinRTSystem::getAssetsPath(const Path& gamePath) const
{
	static auto path = String(winrt::Windows::ApplicationModel::Package::Current().InstalledLocation().Path().data()) + String("/Assets/");
	return path;
}

Path WinRTSystem::getUnpackedAssetsPath(const Path& gamePath) const
{
	static auto path = String(winrt::Windows::ApplicationModel::Package::Current().InstalledLocation().Path().data()) + String("/Assets/");
	return path;
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
	return Vector2i(1920, 1080);
}

Rect4i WinRTSystem::getDisplayRect(int screen) const
{
	return Rect4i(0, 0, 1920, 1080);
}

void WinRTSystem::showCursor(bool show)
{
	// TODO
}

std::shared_ptr<ISaveData> WinRTSystem::getStorageContainer(SaveDataType type, const String& containerName)
{
	String prefix = toString(type);
	if (!containerName.isEmpty()) {
		prefix = prefix + "/" + containerName;
	}
	return std::make_shared<WinRTLocalSave>(prefix);
}

bool WinRTSystem::generateEvents(VideoAPI* video, InputAPI* input)
{
	CoreWindow window = CoreWindow::GetForCurrentThread();
	CoreDispatcher dispatcher = window.Dispatcher();
	dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
	
	return true;
}



struct View : winrt::implements<View, IFrameworkView>
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

	void Load(winrt::hstring entryPoint)
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

struct Source : winrt::implements<Source, IFrameworkViewSource>
{
	Source(WinRTSystem& system, std::function<void()>&& runnable)
		: system(system)
		, runnable(std::move(runnable))
	{}

	IFrameworkView CreateView()
	{
		return winrt::make<View>(system, std::move(runnable));
	}

private:
	WinRTSystem& system;
	std::function<void()> runnable;
};

void WinRTSystem::runGame(std::function<void()> runnable)
{
	winrt::init_apartment();
	ApplicationViewScaling::TrySetDisableLayoutScaling(true);
	CoreApplication::Run(winrt::make<Source>(*this, std::move(runnable)));
}

WinRTLocalSave::WinRTLocalSave(String prefix)
	: folder(makeFolder(prefix))
{
}

bool WinRTLocalSave::isReady() const
{
	return true;
}

Bytes WinRTLocalSave::getData(const String& path)
{
	return Concurrent::execute([&] () -> Bytes {
		try {
			auto file = folder.GetFileAsync(path.getUTF16().c_str()).get();
			auto buffer = FileIO::ReadBufferAsync(file).get();

			auto size = buffer.Length();
			Bytes result(size);
			auto dataReader = Streams::DataReader::FromBuffer(buffer);
			dataReader.ReadBytes(winrt::array_view<uint8_t>(result));

			return result;
		} catch (...) {
			return {};
		}
	}).get();
}

std::vector<String> WinRTLocalSave::enumerate(const String& root)
{
	return Concurrent::execute([&] () -> std::vector<String> {
		std::vector<String> result;
		try {
			auto files = folder.GetFilesAsync(winrt::Windows::Storage::Search::CommonFileQuery::OrderByName, 0, 256).get();
			for (auto& f: files) {
				String name = String(f.Name().c_str());
				if (name.startsWith(root)) {
					result.push_back(name);
				}
			}
		} catch (...) {}
		return result;
	}).get();
}

void WinRTLocalSave::setData(const String& path, const Bytes& data, bool commit)
{
	Concurrent::execute([&] () {
		auto file = folder.CreateFileAsync(path.getUTF16().c_str(), CreationCollisionOption::ReplaceExisting).get();
		FileIO::WriteBytesAsync(file, data).get();
	}).get(); // This .get() is not necessary, but gives me peace of mind
}

void WinRTLocalSave::removeData(const String& path)
{
	Concurrent::execute([&]() {
		try {
			auto file = folder.GetFileAsync(path.getUTF16().c_str()).get();
			file.DeleteAsync().get();
		}
		catch (...) {}
	}).get();
}

void WinRTLocalSave::commit()
{
}

StorageFolder WinRTLocalSave::makeFolder(String prefix)
{
	return Concurrent::execute([&] () -> StorageFolder {
		auto localFolder = ApplicationData::Current().LocalFolder();
		return localFolder.CreateFolderAsync(prefix.getUTF16().c_str(), CreationCollisionOption::OpenIfExists).get();
	}).get();
}


#endif
