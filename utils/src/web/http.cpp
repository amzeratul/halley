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

#include "halley/web/http.h"

#ifdef WITH_BOOST_ASIO

using namespace Halley;

#ifdef _MSC_VER
#pragma warning(disable: 6386 6258 6309 6387 4913)
#endif

#include <boost/asio.hpp>
#include "exception.h"

std::vector<char> HTTP::get(String host, String path)
{
	String content = "";
	return request(host, path, false, content, "");
}

std::vector<char> Halley::HTTP::post(String host, String path, std::vector<HTTPPostEntry>& entries)
{
	String boundary = "=.AaB03xBOunDaRyyy--";

	std::stringstream c;
	for (size_t i=0; i<entries.size(); i++) {
		c << "--" << boundary << "\r\n";
		if (entries[i].filename == "") {
			c << "content-disposition: form-data; name=\"" << entries[i].name << "\"\r\n\r\n";
		} else {
			c << "content-disposition: form-data; name=\"" << entries[i].name << "\"; filename=\"" << entries[i].filename << "\"\r\n";
			c << "Content-Type: application/unknown\r\n";
			c << "Content-Transfer-Encoding: binary\r\n\r\n";
		}
		c.write(entries[i].data.data(), entries[i].data.size());
		if (i != entries.size() - 1) c << "\r\n";
	}
	String content = c.str();

	return request(host, path, true, content, boundary);
}

std::vector<char> Halley::HTTP::request(String host, String path, bool isPost, String& content, String boundary)
{
	// Code adapted from http://www.boost.org/doc/libs/1_49_0_beta1/doc/html/boost_asio/example/http/client/sync_client.cpp
	
	using boost::asio::ip::tcp;
	boost::asio::io_service io_service;

	// Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(host.c_str(), "http");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << (isPost ? "POST " : "GET ") << path.c_str() << " HTTP/1.0\r\n";
    request_stream << "Host: " << host.c_str() << "\r\n";
    request_stream << "Accept: */*\r\n";
	if (isPost) {
		request_stream << "Content-Length: " << content.length() << "\r\n";
		request_stream << "Content-Type: multipart/form-data; boundary=" << boundary << "\r\n";
	}
    request_stream << "Connection: close\r\n\r\n";
	if (isPost) {
		request_stream << content;
	}

	// Send the request.
	boost::asio::write(socket, request);

	// Read the reply
	std::vector<char> result;
	size_t pos = 0;
	while (true) {
		array<char, 128> buf;
		boost::system::error_code error;

		size_t len = socket.read_some(boost::asio::buffer(buf), error);
		if (error == boost::asio::error::eof) {
			break;
		} else if (error) {
			throw Exception("Error reading from socket: " + error.message());
		}

		result.resize(result.size() + len);
		memcpy(&result[pos], &buf[0], len);
		pos += len;
	}
	return result;
}

#endif
