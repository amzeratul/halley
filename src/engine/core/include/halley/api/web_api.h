#pragma once

#include <memory>
#include <map>
#include <halley/concurrency/future.h>
#include <halley/utils/utils.h>
#include "halley/text/i18n.h"
#include "halley/text/enum_names.h"
#include "halley/data_structures/vector.h"
#include "halley/file_formats/json_convert.h"

namespace Halley
{
	class ISaveData;
	class String;
	class InputKeyboard;
	class NetworkService;

	class HTTPResponse {
	public:
		virtual ~HTTPResponse() {}

		virtual int getResponseCode() const { return 0; }
		virtual const Bytes& getBody() const { const static Bytes bytes = Bytes(); return bytes; }
		virtual Bytes moveBody() { return getBody(); }
		virtual String getRedirectLocation() { return ""; }

		void setCancelled() { cancelled = true; }
		bool isCancelled() const { return cancelled; }

	private:
		bool cancelled = false;
	};

	class HTTPRequest {
	public:
		virtual ~HTTPRequest() {}

		[[deprecated("Use setBody instead")]] void setPostData(const String& contentType, const Bytes& data) { setBody(contentType, data); }

		virtual void setBody(const String& contentType, const Bytes& data) = 0;
		virtual void setJsonBody(const ConfigNode& data)
		{
			setBody("application/json", JSONConvert::generateJSON(data).toBytes());
		}

		virtual void setHeader(const String& headerName, const String& headerValue) = 0;
		virtual void setProgressCallback(std::function<bool(uint64_t, uint64_t)> callback) {}

		virtual Future<std::unique_ptr<HTTPResponse>> send() = 0;
	};

	// Thanks, Windows.h
	#ifdef DELETE
	#undef DELETE
	#endif
	
	enum class HTTPMethod {
		GET,
		POST,
		PUT,
		DELETE,
		PATCH
	};

	template <>
	struct EnumNames<HTTPMethod> {
		constexpr auto operator()() const {
			return std::to_array({
				"GET",
				"POST",
				"PUT",
				"DELETE",
				"PATCH"
			});
		}
	};

	class HTTPServerRequest	{
	public:

	};

	class HTTPServerDataSink {
	public:
		virtual ~HTTPServerDataSink() = default;

		virtual void write(gsl::span<const std::byte> bytes) = 0;
		virtual void done() = 0;
	};

	class HTTPServerResponse {
	public:
		using Callback = std::function<bool(HTTPServerDataSink& sink)>;

		virtual ~HTTPServerResponse() = default;

		virtual void setContent(const String& data, const String& dataType)
		{
			setContent(gsl::as_bytes(data.asSpan()), dataType);
		}

		virtual void setContent(gsl::span<const std::byte> data, const String& dataType) = 0;
		virtual void setChunkedContentProvider(const String& dataType, Callback callback) = 0;
	};

	class HTTPServer {
	public:
		using Handler = std::function<void(const HTTPServerRequest& request, HTTPServerResponse& response)>;

		virtual ~HTTPServer() = default;

		virtual void endpointGet(const String& endpoint, Handler handler) = 0;
		virtual void listen(const String& address, int port) = 0;
	};

	class WebAPI
	{
	public:
		virtual ~WebAPI() {}

		virtual std::unique_ptr<HTTPRequest> makeHTTPRequest(HTTPMethod method, const String& url) = 0;
	};

	class WebServerAPI
	{
	public:
		virtual ~WebServerAPI() {}

		virtual std::unique_ptr<HTTPServer> makeHTTPServer() { return {}; }
	};
}
