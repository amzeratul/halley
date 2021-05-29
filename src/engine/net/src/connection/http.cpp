/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2012 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "connection/http.h"
#include <halley/support/exception.h>


#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/utils.h"

using namespace Halley;

#ifdef _MSC_VER
#pragma warning(disable: 6386 6258 6309 6387 4913)
#endif

#define BOOST_SYSTEM_NO_DEPRECATED
#include <boost/asio.hpp>

Bytes HTTP::request(const String& host, const String& path, const String& method, const String& content, const std::map<String, String>& headers)
{
	// Code adapted from http://www.boost.org/doc/libs/1_49_0_beta1/doc/html/boost_asio/example/http/client/sync_client.cpp
	using boost::asio::ip::tcp;
	boost::asio::io_service io_service;

	// Get a list of endpoints corresponding to the server name.
	const auto hostParts = host.split(':');
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(hostParts[0].c_str(), hostParts.size() >= 2 ? hostParts[1] : "http");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf requestBuffer;
    //std::ostream request(&requestBuffer);
	std::stringstream request;
    request << method.c_str() << " " << path.c_str() << " HTTP/1.1\r\n";
    request << "Host: " << hostParts[0].c_str() << "\r\n";
	for (const auto& header: headers) {
		request << header.first.c_str() << ": " << header.second.c_str() << "\r\n";
	}
	if (!std_ex::contains(headers, "Accept")) {
		request << "Accept: */*\r\n";
	}
    request << "Connection: close\r\n";
	request << "\r\n";
	if (!content.isEmpty()) {
		request << content;
	}
	
	//Logger::logDev("Sending:\n\n" + request.str());
	std::ostream reqStream(&requestBuffer);
	reqStream << request.str();

	// Send the request.
	write(socket, requestBuffer);

	// Read the reply
	Bytes result;
	size_t pos = 0;
	while (true) {
		std::array<char, 2048> buf;
		boost::system::error_code error;

		size_t len = socket.read_some(boost::asio::buffer(buf), error);
		if (error == boost::asio::error::eof) {
			break;
		} else if (error) {
			throw Exception("Error reading from socket: " + error.message(), HalleyExceptions::Network);
		}

		result.resize(result.size() + len);
		memcpy(&result[pos], &buf[0], len);
		pos += len;
	}
	return result;
}
