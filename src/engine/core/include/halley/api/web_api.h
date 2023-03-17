#pragma once

#include <memory>
#include <map>
#include <halley/concurrency/future.h>
#include <halley/utils/utils.h>
#include "halley/text/i18n.h"
#include "halley/text/enum_names.h"

namespace Halley
{
	class ISaveData;
	class String;
	class InputKeyboard;
	class NetworkService;

	class HTTPResponse {
	public:
		virtual ~HTTPResponse() {}

		virtual int getResponseCode() const = 0;
		virtual const Bytes& getBody() const = 0;
		virtual Bytes moveBody() { return getBody(); }

		void setCancelled() { cancelled = true; }
		bool isCancelled() const { return cancelled; }

	private:
		bool cancelled = false;
	};

	class HTTPRequest {
	public:
		virtual ~HTTPRequest() {}

		virtual void setPostData(const String& contentType, const Bytes& data) = 0;
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
		constexpr std::array<const char*, 5> operator()() const {
			return{{
				"GET",
				"POST",
				"PUT",
				"DELETE",
				"PATCH"
			}};
		}
	};

	class WebAPI
	{
	public:
		virtual ~WebAPI() {}

		virtual std::unique_ptr<HTTPRequest> makeHTTPRequest(HTTPMethod method, const String& url) = 0;
	};
}
