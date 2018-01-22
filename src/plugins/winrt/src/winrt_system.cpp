#include "winrt_system.h"
#include <ppltasks.h>
using namespace Halley;

#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Storage.FileProperties.h"
#include "winrt/Windows.ApplicationModel.h"
using namespace winrt;

#pragma comment(lib, "windowsapp")


class WinRTDataReader : public ResourceDataReader {
public:

	WinRTDataReader(Windows::Storage::StorageFile&& _file)
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
		Windows::Storage::Streams::DataReader reader(stream);
		size_t bytesLoaded = size_t(reader.LoadAsync(dst.size_bytes()).get());
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
	Windows::Storage::StorageFile file;
	Windows::Storage::Streams::IRandomAccessStreamWithContentType stream;
	bool open = false;
};


void WinRTSystem::init()
{
	init_apartment();
}

void WinRTSystem::deInit()
{
	uninit_apartment();
}

String WinRTSystem::getResourcesBasePath(const String& gamePath) const
{
	return "Assets\\";
}

std::unique_ptr<ResourceDataReader> WinRTSystem::getDataReader(String path, int64_t start, int64_t end)
{
	try {
		Windows::Storage::StorageFolder assetFolder = Windows::ApplicationModel::Package::Current().InstalledLocation();
		Windows::Storage::StorageFile file = assetFolder.GetFileAsync(param::hstring(path.replaceAll("/", "\\").getUTF16().c_str())).get();
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
	// TODO
	return {};
}

void WinRTSystem::destroyWindow(std::shared_ptr<Window> window)
{
	// TODO
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
	// TODO
	return true;
}
