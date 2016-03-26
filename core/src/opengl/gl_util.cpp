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

#include "gl_util.h"
#include <set>

namespace Halley {
	void doGlCheckError(const char* file, long line)
	{
#ifdef _DEBUG
		const bool alwaysCheck = true;
#else
		const bool alwaysCheck = false;
#endif
		const bool checkOnce = false;
		bool check = false;

		if (alwaysCheck) {
			check = true;
		} else if (checkOnce) {
			static std::set<String> checked;
			String key = String(file) + ":" + String::integerToString(line);
			if (checked.find(key) == checked.end()) {
				// Not checked yet, go ahead
				check = true;
				checked.insert(key);
			}
		}

		if (check) {
			int error = glGetError();
			if (error != 0) {
				String msg = "OpenGL error: ";
				switch (error) {
				case GL_INVALID_ENUM: msg += "GL_INVALID_ENUM"; break;
				case GL_INVALID_VALUE: msg += "GL_INVALID_VALUE"; break;
				case GL_INVALID_OPERATION: msg += "GL_INVALID_OPERATION"; break;
				case GL_STACK_OVERFLOW: msg += "GL_STACK_OVERFLOW"; break;
				case GL_STACK_UNDERFLOW: msg += "GL_STACK_UNDERFLOW"; break;
				case GL_OUT_OF_MEMORY: msg += "GL_OUT_OF_MEMORY"; break;
				default: msg += "?";
				}
				if (String(file) != "") {
					msg += " at " + String(file) + ":" + String::integerToString(line);
				}
				throw Exception(msg);
			}
		}
	}
}
