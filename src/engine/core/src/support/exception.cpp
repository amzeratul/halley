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

#include "halley/support/exception.h"
#include "halley/support/debug.h"
#include <sstream>

using namespace Halley;

Exception::Exception(String msg, int errorCode) noexcept
	: msg(std::move(msg))
	, errorCode(errorCode)
{
	if (collectStackTrace) {
		stackTrace = Debug::getCallStack(4);
	}

	std::stringstream ss;
	ss << this->msg << "\n" << stackTrace;
	fullMsg = ss.str();
}

const char* Exception::what() const noexcept
{
	return fullMsg.c_str();
}

const String& Exception::getMessage() const
{
	return msg;
}

const String& Exception::getStackTrace() const
{
	return stackTrace;
}

const String& Exception::getFullMessage() const
{
	return fullMsg;
}

int Exception::getErrorCode() const
{
	return errorCode;
}

void Exception::setCollectStackTrace(bool collect)
{
	collectStackTrace = collect;
}

bool Exception::collectStackTrace = true;
