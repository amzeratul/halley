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

void WinRTHTTPRequest::setHeader(const String& headerName, const String& headerValue)
{
	// TODO
}

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
