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

using namespace Halley;

Exception::Exception(const String& _msg, bool logCallStack)
{
	if (logCallStack) {
		String stack = Debug::getCallStack();
		if (stack != "") {
			msg = _msg + "\n" + stack;
			return;
		}
	}

	msg = _msg;
}

const char* Exception::what() const noexcept
{
	return msg.c_str();
}
