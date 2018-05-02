#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "winrt/Windows.Web.Http.h"
#include "winrt/Windows.Foundation.h"

using namespace winrt;

namespace Halley
{
	class WinRTHTTPResponse : public HTTPResponse
	{
	public:
		WinRTHTTPResponse(int code, Bytes body);
		int getResponseCode() const override;
		const Bytes& getBody() const override;

	private:
		int responseCode;
		Bytes body;
	};

	class WinRTHTTPRequest : public HTTPRequest
	{
		struct Data
		{
			String method;
			String url;
			String contentType;
			Bytes postData;
		};

	public:
		WinRTHTTPRequest(const String& method, const String& url);
		void setPostData(const String& contentType, const Bytes& data) override;
		void setHeader(const String& headerName, const String& headerValue) override;
		Future<std::unique_ptr<HTTPResponse>> send() override;

	private:
		std::shared_ptr<Data> data;
	};
}
