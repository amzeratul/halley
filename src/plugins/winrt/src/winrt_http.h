#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "winrt/Windows.Web.Http.h"
#include "winrt/Windows.Foundation.h"

using namespace winrt;

namespace Halley
{
	class XMLHTTPRequest2Callback;

	class WinRTHTTPResponse : public HTTPResponse
	{
	public:
		WinRTHTTPResponse();
		~WinRTHTTPResponse();

		int getResponseCode() const override;
		const Bytes& getBody() const override;

		XMLHTTPRequest2Callback* getCallback() const;
		void setResponseCode(HRESULT code);
		Bytes& getBody();
		
		void wait();

	private:
		int responseCode = 0;
		Bytes body;
		XMLHTTPRequest2Callback* callback;
	};

	class WinRTHTTPRequest : public HTTPRequest
	{
		struct Data
		{
			String method;
			String url;
			Bytes postData;
			std::map<String, String> headers;
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
