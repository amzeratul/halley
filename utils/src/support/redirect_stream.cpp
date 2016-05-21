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

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "../../include/halley/support/redirect_stream.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif

using namespace Halley;

RedirectStream::RedirectStream(std::ostream& stream, bool _exclusive)
	: orgstream(stream)
	, exclusive(_exclusive)
{
	// Swap the the old buffer in ostream with this buffer.
	orgbuf = orgstream.rdbuf(this);

	// Create a new ostream that we set the old buffer in
	newstream = std::shared_ptr<std::ostream>(new std::ostream(orgbuf));
}

RedirectStream::~RedirectStream(){
	//Restore old buffer
	orgstream.rdbuf(orgbuf);
}

std::streamsize RedirectStream::xsputn(const char *msg, std::streamsize count){
	onText(msg, count);

#ifdef __ANDROID__
	// On android, also redirect to log
	static std::string logBuf;
	logBuf += std::string(msg, count);
	size_t lineBreak;
	while ((lineBreak = logBuf.find('\n')) != std::string::npos) {
		std::string msg2 = logBuf.substr(0, lineBreak);
		logBuf = logBuf.substr(lineBreak+1);
		if (msg2 != "") {
			__android_log_write(ANDROID_LOG_INFO, "Halley", msg2.c_str());
		}
	}
#else
	// Output to original stream
	if (!exclusive) newstream->write(msg, count);
#endif

	return count;
}

int RedirectStream::overflow(int c)
{
	char tmp = char(c);
	xsputn(&tmp, 1);

	if (!exclusive) {
		newstream->flush();
	}
	onFlush();

	return 0;
}

RedirectStreamToStream::RedirectStreamToStream(std::ostream& source, std::shared_ptr<std::ostream> _dst, bool exclusive)
	: RedirectStream(source, exclusive)
	, dst(_dst)
{
}

RedirectStreamToStream::~RedirectStreamToStream()
{
}

void RedirectStreamToStream::onText(const char *msg, std::streamsize count)
{
	dst->write(msg, count);
}

void RedirectStreamToStream::onFlush()
{
	dst->flush();
}

