#include "resource_data_byte_stream.h"
using namespace Halley;

Halley::ResourceDataByteStream::ResourceDataByteStream(std::shared_ptr<ResourceDataStream> data)
	: reader(data->getReader())
{
	len = reader->size();
}

HRESULT ResourceDataByteStream::QueryInterface(const IID& riid, void** ppvObject)
{
	if (!ppvObject) {
		return E_INVALIDARG;
	}
	*ppvObject = nullptr;

	if (riid == __uuidof(IMFByteStream)) {
		AddRef();
		*ppvObject = static_cast<IMFByteStream*>(this);
		return NOERROR;
	} else {
		return E_NOINTERFACE;
	}
}

ULONG ResourceDataByteStream::AddRef()
{
	return InterlockedIncrement(&refCount);
}

ULONG ResourceDataByteStream::Release()
{
	ULONG uCount = InterlockedDecrement(&refCount);
    if (uCount == 0) {
		delete this;
    }
    return uCount;
}

HRESULT ResourceDataByteStream::GetCapabilities(DWORD* pdwCapabilities)
{
	*pdwCapabilities = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE;
	return S_OK;
}

HRESULT ResourceDataByteStream::GetLength(QWORD* pqwLength)
{
	*pqwLength = len;
	return S_OK;
}

HRESULT ResourceDataByteStream::SetLength(QWORD qwLength)
{
	return E_NOTIMPL;
}

HRESULT ResourceDataByteStream::GetCurrentPosition(QWORD* pqwPosition)
{
	*pqwPosition = reader->tell();
	return S_OK;
}

HRESULT ResourceDataByteStream::SetCurrentPosition(QWORD qwPosition)
{
	reader->seek(qwPosition, SEEK_SET);
	return S_OK;
}

HRESULT ResourceDataByteStream::IsEndOfStream(BOOL* pfEndOfStream)
{
	*pfEndOfStream = reader->tell() == len;
	return S_OK;
}

HRESULT ResourceDataByteStream::Read(BYTE* pb, ULONG cb, ULONG* pcbRead)
{
	*pcbRead = reader->read(gsl::as_writeable_bytes(gsl::span<BYTE>(pb, cb)));
	return S_OK;
}

HRESULT ResourceDataByteStream::BeginRead(BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	ULONG nRead;
	Read(pb, cb, &nRead);

	IMFAsyncResult* result;
	MFCreateAsyncResult(nullptr, pCallback, punkState, &result);

	asyncNumRead[result] = nRead;

	pCallback->Invoke(result);

	return S_OK;
}

HRESULT ResourceDataByteStream::EndRead(IMFAsyncResult* pResult, ULONG* pcbRead)
{
	auto iter = asyncNumRead.find(pResult);
	if (iter == asyncNumRead.end()) {
		throw Exception("Invalid EndRead");
	}
	*pcbRead = iter->second;
	asyncNumRead.erase(iter);

	pResult->Release();
	return S_OK;
}

HRESULT ResourceDataByteStream::Write(const BYTE* pb, ULONG cb, ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

HRESULT ResourceDataByteStream::BeginWrite(const BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	return E_NOTIMPL;
}

HRESULT ResourceDataByteStream::EndWrite(IMFAsyncResult* pResult, ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

HRESULT ResourceDataByteStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD* pqwCurrentPosition)
{
	int whence = SEEK_SET;
	if (SeekOrigin == msoCurrent) {
		whence = SEEK_CUR;
	}

	reader->seek(llSeekOffset, whence);
	*pqwCurrentPosition = reader->tell();

	return S_OK;
}

HRESULT ResourceDataByteStream::Flush()
{
	return S_OK;
}

HRESULT ResourceDataByteStream::Close()
{
	reader->close();
	return S_OK;
}
