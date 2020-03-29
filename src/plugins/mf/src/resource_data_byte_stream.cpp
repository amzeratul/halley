#include "resource_data_byte_stream.h"
#include "halley/concurrency/concurrent.h"
#include <atlbase.h>
using namespace Halley;

class AsyncReadOp final : public IUnknown
{
public:
	size_t from = 0;
	gsl::span<gsl::byte> dst;
	ULONG nRead = 0;

    AsyncReadOp(size_t from, gsl::span<gsl::byte> dst) 
		: from(from)
		, dst(dst)
	{}

    HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject)
    {
		if (!ppvObject) {
			return E_INVALIDARG;
		}
		*ppvObject = nullptr;

		if (riid == __uuidof(IUnknown)) {
			AddRef();
			*ppvObject = static_cast<AsyncReadOp*>(this);
			return NOERROR;
		} else {
			return E_NOINTERFACE;
		}
    }

    ULONG __stdcall AddRef()
    {
        return InterlockedIncrement(&refCount);
    }

    ULONG __stdcall Release()
    {
        long ref = InterlockedDecrement(&refCount);
        if (ref == 0) {
            delete this;
        }
        return ref;
    }

private:
	long refCount = 0;
};

Halley::ResourceDataByteStream::ResourceDataByteStream(std::shared_ptr<ResourceDataStream> data)
	: reader(data->getReader())
{
	len = reader->size();
}

HRESULT __stdcall ResourceDataByteStream::QueryInterface(const IID& riid, void** ppvObject)
{
	if (!ppvObject) {
		return E_INVALIDARG;
	}
	*ppvObject = nullptr;

	if (riid == __uuidof(IMFByteStream)) {
		AddRef();
		*ppvObject = static_cast<IMFByteStream*>(this);
		return NOERROR;
	} else if (riid == __uuidof(IMFAsyncCallback)) {
		AddRef();
		*ppvObject = static_cast<IMFAsyncCallback*>(this);
		return NOERROR;
	} else {
		return E_NOINTERFACE;
	}
}

ULONG __stdcall ResourceDataByteStream::AddRef()
{
	return InterlockedIncrement(&refCount);
}

ULONG __stdcall ResourceDataByteStream::Release()
{
	ULONG uCount = InterlockedDecrement(&refCount);
    if (uCount == 0) {
		delete this;
    }
    return uCount;
}

HRESULT __stdcall ResourceDataByteStream::GetCapabilities(DWORD* pdwCapabilities)
{
	*pdwCapabilities = MFBYTESTREAM_IS_READABLE | MFBYTESTREAM_IS_SEEKABLE;
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::GetLength(QWORD* pqwLength)
{
	*pqwLength = len;
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::SetLength(QWORD qwLength)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ResourceDataByteStream::GetCurrentPosition(QWORD* pqwPosition)
{
	std::unique_lock<std::mutex> lock(mutex);
	*pqwPosition = QWORD(pos);
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::SetCurrentPosition(QWORD qwPosition)
{
	std::unique_lock<std::mutex> lock(mutex);
	pos = size_t(qwPosition);
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::IsEndOfStream(BOOL* pfEndOfStream)
{
	std::unique_lock<std::mutex> lock(mutex);
	*pfEndOfStream = pos >= len;
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::Read(BYTE* pb, ULONG cb, ULONG* pcbRead)
{
	std::unique_lock<std::mutex> lock(mutex);
	reader->seek(pos, SEEK_SET);
	const auto nRead = reader->read(gsl::as_writeable_bytes(gsl::span<BYTE>(pb, cb)));
	pos += nRead;
	*pcbRead = nRead;
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::BeginRead(BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	/*
	punkState->AddRef();
	pCallback->AddRef();
	Concurrent::execute([=] () {
		IMFAsyncResult* result;
		MFCreateAsyncResult(nullptr, pCallback, punkState, &result);
		{
			std::unique_lock<std::mutex> lock(mutex);
			ULONG nRead = reader->read(gsl::as_writeable_bytes(gsl::span<BYTE>(pb, cb)));
			asyncNumRead[result] = nRead;
		}
		pCallback->Invoke(result);
		punkState->Release();
		pCallback->Release();
	});
	*/

	auto op = new AsyncReadOp(pos, gsl::as_writeable_bytes(gsl::span<BYTE>(pb, cb)));
	pos += cb;
	if (pos > len) {
		pos = len;
	}

	IMFAsyncResult* result;
	auto hr = MFCreateAsyncResult(op, pCallback, punkState, &result);
	if (!SUCCEEDED(hr)) {
		throw Exception("Error in ResourceDataByteStream", HalleyExceptions::MoviePlugin);
	}

	IMFAsyncResult* invokeResult;
	hr = MFCreateAsyncResult(nullptr, this, result, &invokeResult);
	if (!SUCCEEDED(hr)) {
		throw Exception("Error in ResourceDataByteStream", HalleyExceptions::MoviePlugin);
	}
	hr = MFInvokeCallback(invokeResult);
	//hr = MFPutWorkItem(MFASYNC_CALLBACK_QUEUE_STANDARD, this, result);
	//result->Release();

	return hr;
}

HRESULT ResourceDataByteStream::Invoke(IMFAsyncResult* pAsyncResult)
{
	IUnknown* state;
	IUnknown* unknown;
	IMFAsyncResult* callerResult = nullptr;
	auto hr = pAsyncResult->GetState(&state);
	if (!SUCCEEDED(hr)) {
		throw Exception("Error in ResourceDataByteStream", HalleyExceptions::MoviePlugin);
	}
	hr = state->QueryInterface(IID_PPV_ARGS(&callerResult));
	if (!SUCCEEDED(hr)) {
		throw Exception("Error in ResourceDataByteStream", HalleyExceptions::MoviePlugin);
	}
	hr = callerResult->GetObject(&unknown);
	if (!SUCCEEDED(hr)) {
		throw Exception("Error in ResourceDataByteStream", HalleyExceptions::MoviePlugin);
	}

	auto readOp = static_cast<AsyncReadOp*>(unknown);

	{
		std::unique_lock<std::mutex> lock(mutex);
		reader->seek(readOp->from, SEEK_SET);
		readOp->nRead = reader->read(readOp->dst);
	}

	if (callerResult) {
		callerResult->SetStatus(S_OK);
		hr = MFInvokeCallback(callerResult);
		if (!SUCCEEDED(hr)) {
			throw Exception("Error in ResourceDataByteStream", HalleyExceptions::MoviePlugin);
		}
	}

	if (state) {
		state->Release();
	}
	if (unknown) {
		unknown->Release();
	}
	if (callerResult) {
		callerResult->Release();
	}

	//EndRead(pAsyncResult, nullptr);
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::EndRead(IMFAsyncResult* pResult, ULONG* pcbRead)
{
	IUnknown* unknown;
	pResult->GetObject(&unknown);
	auto readOp = static_cast<AsyncReadOp*>(unknown);
	*pcbRead = readOp->nRead;

	pResult->Release();
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::Write(const BYTE* pb, ULONG cb, ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ResourceDataByteStream::BeginWrite(const BYTE* pb, ULONG cb, IMFAsyncCallback* pCallback, IUnknown* punkState)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ResourceDataByteStream::EndWrite(IMFAsyncResult* pResult, ULONG* pcbWritten)
{
	return E_NOTIMPL;
}

HRESULT __stdcall ResourceDataByteStream::Seek(MFBYTESTREAM_SEEK_ORIGIN SeekOrigin, LONGLONG llSeekOffset, DWORD dwSeekFlags, QWORD* pqwCurrentPosition)
{
	std::unique_lock<std::mutex> lock(mutex);
	if (SeekOrigin == msoCurrent) {
		pos += size_t(llSeekOffset);
	} else {
		pos = size_t(llSeekOffset);
	}
	*pqwCurrentPosition = QWORD(pos);

	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::Flush()
{
	return S_OK;
}

HRESULT __stdcall ResourceDataByteStream::Close()
{
	reader->close();
	return S_OK;
}

HRESULT ResourceDataByteStream::GetParameters(DWORD* pdwFlags, DWORD* pdwQueue)
{
	return E_NOTIMPL;
}
