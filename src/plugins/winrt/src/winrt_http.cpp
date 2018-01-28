#include "winrt_http.h"
using namespace Halley;

#include "winrt/Windows.Storage.Streams.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Web.Http.Headers.h"

using namespace Windows::Web::Http;
using namespace Windows::Storage::Streams;

WinRTHTTPResponse::WinRTHTTPResponse(int code, Bytes body)
	: responseCode(code)
	, body(body)
{	
}

int WinRTHTTPResponse::getResponseCode() const
{
	return responseCode;
}

const Bytes& WinRTHTTPResponse::getBody() const
{
	return body;
}

WinRTHTTPRequest::WinRTHTTPRequest(const String& method, const String& url)
	: data(std::make_shared<Data>())
{
	data->method = method;
	data->url = url;
}

void WinRTHTTPRequest::setPostData(const String& type, const Bytes& bytes)
{
	data->contentType = type;
	data->postData = bytes;
}

struct __declspec(uuid("905a0fef-bc53-11df-8c49-001e4fc686da")) IBufferByteAccess : ::IUnknown
{
    virtual HRESULT __stdcall Buffer(uint8_t** value) = 0;
};

struct CustomBuffer : implements<CustomBuffer, IBuffer, IBufferByteAccess>
{
    std::vector<uint8_t> m_buffer;
    uint32_t m_length{};

    CustomBuffer(uint32_t size) :
        m_buffer(size)
    {
    }

    uint32_t Capacity() const
    {
        return m_buffer.size();
    }

    uint32_t Length() const
    {
        return m_length;
    }

    void Length(uint32_t value)
    {
        if (value > m_buffer.size())
        {
            throw hresult_invalid_argument();
        }

        m_length = value;
    }

    HRESULT __stdcall Buffer(uint8_t** value) final
    {
        *value = m_buffer.data();
        return S_OK;
    }
};

Future<std::unique_ptr<HTTPResponse>> WinRTHTTPRequest::send()
{
	auto data = this->data;

	return Concurrent::execute([data] () mutable -> std::unique_ptr<HTTPResponse>
	{
		HttpClient client;
		HttpResponseMessage response;
		auto uri = Windows::Foundation::Uri(data->url.getUTF16().c_str());

		if (data->method == "GET") {
			response = client.GetAsync(uri).get();
		} else if (data->method == "POST") {
			auto writer = DataWriter();
			writer.WriteBytes(array_view<const uint8_t>(data->postData.data(), data->postData.data() + data->postData.size()));
			IBuffer buffer = writer.DetachBuffer();
			
			HttpBufferContent content(buffer);
			content.Headers().Append(L"Content-Type", data->contentType.getUTF16().c_str());
			response = client.PostAsync(uri, content).get();
		} else {
			throw Exception("Unknown method: " + data->method);
		}

		int code = int(response.StatusCode());
		hstring body = response.Content().ReadAsStringAsync().get();
		String bodyStr(body.c_str());
		Bytes bodyData(bodyStr.size() + 1);
		memcpy_s(bodyData.data(), bodyData.size(), bodyStr.c_str(), bodyStr.size() + 1);
		return std::make_unique<WinRTHTTPResponse>(code, std::move(bodyData));
	});
}
