#pragma once
#include "halley/resources/resource_data.h"
#include <memory>
#include <map>

#include <Mfapi.h>
#include <Mfidl.h>
#include <Mfreadwrite.h>
#undef min
#undef max

namespace Halley {
	class ResourceDataByteStream final : public IMFByteStream, public IMFAsyncCallback 
	{
	public:
		ResourceDataByteStream(std::shared_ptr<ResourceDataStream> data);

		HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObject) override;
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;
		HRESULT __stdcall GetCapabilities(DWORD* pdwCapabilities) override;
		HRESULT __stdcall GetLength(QWORD* pqwLength) override;
		HRESULT __stdcall SetLength(QWORD qwLength) override;
		HRESULT __stdcall GetCurrentPosition(QWORD* pqwPosition) override;
		HRESULT __stdcall SetCurrentPosition(QWORD qwPosition) override;
		HRESULT __stdcall IsEndOfStream(BOOL* pfEndOfStream) override;
		HRESULT __stdcall Read(BYTE* pb, ULONG cb, ULONG* pcbRead) override;
		HRESULT __stdcall BeginRead(BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState) override;
		HRESULT __stdcall EndRead(IMFAsyncResult* pResult, ULONG* pcbRead) override;
		HRESULT __stdcall Write(const BYTE* pb, ULONG cb, ULONG* pcbWritten) override;
		HRESULT __stdcall BeginWrite(const BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState) override;
		HRESULT __stdcall EndWrite(IMFAsyncResult* pResult, ULONG* pcbWritten) override;
		HRESULT __stdcall Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD* pqwCurrentPosition) override;
		HRESULT __stdcall Flush() override;
		HRESULT __stdcall Close() override;
		HRESULT __stdcall GetParameters(DWORD* pdwFlags, DWORD* pdwQueue) override;
		HRESULT __stdcall Invoke(IMFAsyncResult* pAsyncResult) override;

	private:
		std::unique_ptr<ResourceDataReader> reader;
		std::mutex mutex;
		size_t len;
		size_t pos = 0;
		long refCount = 0;
	};
}
