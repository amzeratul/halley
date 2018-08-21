#ifdef WINDOWS_STORE

#include "winrt_http.h"
using namespace Halley;

#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Web.Http.Headers.h"

#include <msxml6.h>
#include <mutex>

#pragma comment(lib, "msxml6.lib")

using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Storage::Streams;


#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif


namespace Halley {
	class XMLHTTPRequest2Callback : public IXMLHTTPRequest2Callback
	{
	public:
		XMLHTTPRequest2Callback(WinRTHTTPResponse& response)
			: response(response)
			, done(false)
		{
		}

		virtual ~XMLHTTPRequest2Callback() = default;

		HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObject) override
		{
			if (!ppvObject) {
				return E_INVALIDARG;
			}
			*ppvObject = nullptr;

			if (riid == __uuidof(IXMLHTTPRequest2Callback)) {
				AddRef();
				*ppvObject = dynamic_cast<IXMLHTTPRequest2Callback*>(this);
				return NOERROR;
			} else {
				return E_NOINTERFACE;
			}
		}

		ULONG __stdcall AddRef() override
		{
			return InterlockedIncrement(&refCount);
		}

		ULONG __stdcall Release() override
		{
			ULONG uCount = InterlockedDecrement(&refCount);
		    if (uCount == 0) {
				delete this;
		    }
		    return uCount;
		}

		HRESULT __stdcall OnRedirect(IXMLHTTPRequest2* pXHR, const WCHAR* pwszRedirectUrl) override
		{
			return S_OK;
		}

		HRESULT __stdcall OnHeadersAvailable(IXMLHTTPRequest2* pXHR, DWORD dwStatus, const WCHAR* pwszStatus) override
		{
			response.setResponseCode(dwStatus);
			return S_OK;
		}

		HRESULT __stdcall OnDataAvailable(IXMLHTTPRequest2* pXHR, ISequentialStream* pResponseStream) override
		{
			return S_OK;
		}

		HRESULT __stdcall OnResponseReceived(IXMLHTTPRequest2* pXHR, ISequentialStream* pResponseStream) override
		{
			auto& body = response.getBody();

			std::array<char, 4096> buffer;
			ULONG nRead = 0;
			do {
				pResponseStream->Read(buffer.data(), buffer.size(), &nRead);
				size_t pos = body.size();
				body.resize(pos + nRead);
				memcpy(body.data() + pos, buffer.data(), nRead);
			} while (nRead > 0);

			std::unique_lock<std::mutex> lock(mutex);
			done = true;
			condition.notify_all();

			return S_OK;
		}

		HRESULT __stdcall OnError(IXMLHTTPRequest2* pXHR, HRESULT hrError) override
		{
			return S_OK;
		}

		void wait()
		{
			std::unique_lock<std::mutex> lock(mutex);
			if (!done) {
				condition.wait(lock);
			}
		}

	private:
		WinRTHTTPResponse& response;
		long refCount = 0;
		std::mutex mutex;
		std::condition_variable condition;
		std::atomic<bool> done;
	};



	class ByteSequentialStream : public ISequentialStream {
	public:
		ByteSequentialStream(Bytes bytes)
			: bytes(std::move(bytes))
		{}

		HRESULT __stdcall QueryInterface(const IID& riid, void** ppvObject) override
		{
			if (!ppvObject) {
				return E_INVALIDARG;
			}
			*ppvObject = nullptr;

			if (riid == __uuidof(ISequentialStream)) {
				AddRef();
				*ppvObject = dynamic_cast<ISequentialStream*>(this);
				return NOERROR;
			} else {
				return E_NOINTERFACE;
			}
		}

		ULONG __stdcall AddRef() override
		{
			return InterlockedIncrement(&refCount);
		}

		ULONG __stdcall Release() override
		{
			ULONG uCount = InterlockedDecrement(&refCount);
		    if (uCount == 0) {
				delete this;
		    }
		    return uCount;
		}

		HRESULT __stdcall Read(void* pv, ULONG cb, ULONG* pcbRead) override
		{
			size_t available = bytes.size() - readPos;
			size_t desired = size_t(cb);
			size_t toRead = std::min(available, desired);
			memcpy(pv, bytes.data() + readPos, toRead);
			readPos += toRead;
			*pcbRead = toRead;

			return S_OK;
		}

		HRESULT __stdcall Write(const void* pv, ULONG cb, ULONG* pcbWritten) override
		{
			return E_NOTIMPL;
		}

	private:
		Bytes bytes;
		size_t readPos = 0;
		long refCount = 0;
	};
}


WinRTHTTPResponse::WinRTHTTPResponse()
{
	callback = new XMLHTTPRequest2Callback(*this);
	callback->AddRef();
}

WinRTHTTPResponse::~WinRTHTTPResponse()
{
	wait();
}

int WinRTHTTPResponse::getResponseCode() const
{
	return responseCode;
}

const Bytes& WinRTHTTPResponse::getBody() const
{
	return body;
}

XMLHTTPRequest2Callback* WinRTHTTPResponse::getCallback() const
{
	return callback;
}

void WinRTHTTPResponse::setResponseCode(HRESULT code)
{
	responseCode = code;
}

Bytes& WinRTHTTPResponse::getBody()
{
	return body;
}

void WinRTHTTPResponse::wait()
{
	if (callback) {
		callback->wait();
		callback->Release();
		callback = nullptr;
	}
}


WinRTHTTPRequest::WinRTHTTPRequest(const String& method, const String& url)
	: data(std::make_shared<Data>())
{
	data->method = method;
	data->url = url;
}

void WinRTHTTPRequest::setPostData(const String& type, const Bytes& bytes)
{
	data->headers["Content-Type"] = type;
	data->postData = bytes;
}

void WinRTHTTPRequest::setHeader(const String& headerName, const String& headerValue)
{
	data->headers[headerName] = headerValue;
}

Future<std::unique_ptr<HTTPResponse>> WinRTHTTPRequest::send()
{
	auto data = this->data;

	return Concurrent::execute([data] () mutable -> std::unique_ptr<HTTPResponse>
	{
		auto response = std::make_unique<WinRTHTTPResponse>();
		IXMLHTTPRequest2* request;
		auto hr = CoCreateInstance(CLSID_FreeThreadedXMLHTTP60, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&request));
		if (!SUCCEEDED(hr)) {
			throw Exception("Unable to create IXMLHTTPRequest2", HalleyExceptions::PlatformPlugin);
		}

		hr = request->Open(data->method.getUTF16().c_str(), data->url.getUTF16().c_str(), response->getCallback(), nullptr, nullptr, nullptr, nullptr);
		if (!SUCCEEDED(hr)) {
			throw Exception("Unable to open HTTP request", HalleyExceptions::PlatformPlugin);
		}

		for (auto& header: data->headers) {
			hr = request->SetRequestHeader(header.first.getUTF16().c_str(), header.second.getUTF16().c_str());
			if (!SUCCEEDED(hr)) {
				throw Exception("Unable to set HTTP header: \"" + header.first + ": " + header.second + "\"", HalleyExceptions::PlatformPlugin);
			}
		}

		const size_t postDataSize = data->postData.size(); // This will be moved, make sure to do this here
		auto src = new ByteSequentialStream(std::move(data->postData));
		src->AddRef();
		request->Send(postDataSize > 0 ? src : nullptr, postDataSize);
		response->wait();
		src->Release();

		return response;
	});
}

#endif