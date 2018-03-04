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
	class ResourceDataByteStream final : public IMFByteStream
	{
	public:
		ResourceDataByteStream(std::shared_ptr<ResourceDataStream> data);

		HRESULT QueryInterface(const IID& riid, void** ppvObject) override;
		ULONG AddRef() override;
		ULONG Release() override;
		HRESULT GetCapabilities(DWORD* pdwCapabilities) override;
		HRESULT GetLength(QWORD* pqwLength) override;
		HRESULT SetLength(QWORD qwLength) override;
		HRESULT GetCurrentPosition(QWORD* pqwPosition) override;
		HRESULT SetCurrentPosition(QWORD qwPosition) override;
		HRESULT IsEndOfStream(BOOL* pfEndOfStream) override;
		HRESULT Read(BYTE* pb, ULONG cb, ULONG* pcbRead) override;
		HRESULT BeginRead(BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState) override;
		HRESULT EndRead(IMFAsyncResult* pResult, ULONG* pcbRead) override;
		HRESULT Write(const BYTE* pb, ULONG cb, ULONG* pcbWritten) override;
		HRESULT BeginWrite(const BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState) override;
		HRESULT EndWrite(IMFAsyncResult* pResult, ULONG* pcbWritten) override;
		HRESULT Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD* pqwCurrentPosition) override;
		HRESULT Flush() override;
		HRESULT Close() override;

	private:
		std::unique_ptr<ResourceDataReader> reader;
		size_t len;
		long refCount = 0;

		std::map<IMFAsyncResult*, ULONG> asyncNumRead;
	};
}
