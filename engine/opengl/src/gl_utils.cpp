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

#include "gl_utils.h"
#include <set>
#include "halley/support/exception.h"
#include "halley/concurrency/concurrent.h"
#include <gsl/gsl_assert>
#include "halley/text/string_converter.h"

#ifdef __APPLE__
#include <pthread.h>
#endif


using namespace Halley;

void GLUtils::doGlCheckError(const char* file, long line)
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
		String key = String(file) + ":" + toString(line);
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
#ifdef WITH_OPENGL
			case GL_STACK_OVERFLOW: msg += "GL_STACK_OVERFLOW"; break;
			case GL_STACK_UNDERFLOW: msg += "GL_STACK_UNDERFLOW"; break;
#endif
			case GL_OUT_OF_MEMORY: msg += "GL_OUT_OF_MEMORY"; break;
			default: msg += "?";
			}
			if (String(file) != "") {
				msg += " at " + String(file) + ":" + toString(line);
			}
			throw Exception(msg);
		}
	}
}


#ifdef _MSC_VER
#pragma warning(disable: 6286 6235)
#endif

using namespace Halley;

static const bool checked = true;

namespace Halley {
	class GLInternals {
	public:
		GLInternals()
		{
			for (int i = 0; i < 8; i++) {
				curTex[i] = 0;
			}
			numUnits = 0;
			curTexUnit = 0;
			scissoring = false;
			curBlend = BlendType::Opaque;
			hasClearCol = false;
		}

		int curTexUnit;
		int numUnits;
		std::array<int, 8> curTex;
		BlendType curBlend;
		Rect4i viewport;
		Colour clearCol;
		bool scissoring;
		bool hasClearCol;
	};

}

#ifdef __APPLE__
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static pthread_key_t key;

static void destroyState(void* value)
{
	GLInternals* state = reinterpret_cast<GLInternals*>(value);
	delete state;
}

static void makeState()
{
	pthread_key_create(&key, destroyState);
}
#endif

GLInternals& getState()
{
#ifdef __APPLE__
	pthread_once(&key_once, makeState);
	GLInternals* ptr = reinterpret_cast<GLInternals*>(pthread_getspecific(key));
    if (!ptr) {
        ptr = new GLInternals();
        pthread_setspecific(key, ptr);
    }

	return *ptr;
#else
	thread_local GLInternals state;
	return state;
#endif
}

////////////////////

GLUtils::GLUtils()
	: state(getState())
{
	glCheckError();
}

GLUtils::GLUtils(GLUtils& other)
	: state(other.state)
{
	glCheckError();
}

void GLUtils::setBlendType(BlendType type)
{
	Expects(type == BlendType::Alpha || type == BlendType::AlphaPremultiplied || type == BlendType::Add || type == BlendType::Opaque || type == BlendType::Multiply || type == BlendType::Darken);
	glCheckError();

	const BlendType curType = state.curBlend;
	if (!checked || curType != type) {
		bool hasBlend = curType == BlendType::Alpha || curType == BlendType::AlphaPremultiplied || curType == BlendType::Add || curType == BlendType::Multiply || curType == BlendType::Darken;
		bool needsBlend = type == BlendType::Alpha || type == BlendType::AlphaPremultiplied || type == BlendType::Add || type == BlendType::Multiply || type == BlendType::Darken;

		// Disable current
		if (hasBlend && !needsBlend) {
			glDisable(GL_BLEND);
			glCheckError();
		}
		else if (!hasBlend && needsBlend) {
			glEnable(GL_BLEND);
			glCheckError();
		}

		if (needsBlend) {
			if (type == BlendType::AlphaPremultiplied) {
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glCheckError();
			}
			else if (type == BlendType::Alpha) {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glCheckError();
			}
			else if (type == BlendType::Multiply) {
				glBlendFunc(GL_ZERO, GL_SRC_COLOR);
				glCheckError();
			}
			else {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				glCheckError();
			}
		}

		// Update current
		state.curBlend = type;
	}
}

void GLUtils::setTextureUnit(int n)
{
	Expects(n >= 0);
	Expects(n < 8);

	if (!checked || state.curTexUnit != n) {
		glActiveTexture(GL_TEXTURE0 + n);
		glCheckError();
		state.curTexUnit = n;
	}
}

void GLUtils::bindTexture(int id)
{
	Expects(id >= 0);

	if (true || !checked || id != state.curTex[state.curTexUnit]) {
		state.curTex[state.curTexUnit] = id;
		glBindTexture(GL_TEXTURE_2D, id);
		glCheckError();
	}
}

void GLUtils::setNumberOfTextureUnits(int n)
{
	Expects(n >= 1);
	Expects(n < 8);

	int prevUnit = state.curTexUnit;

	if (checked) {
		// Enable units
		if (n > state.numUnits) {
			for (int i = state.numUnits; i<n; i++) {
				setTextureUnit(i);
				glCheckError();
			}
		}

		// Disable units
		else {
			for (int i = n; i<state.numUnits; i++) {
				setTextureUnit(i);
				bindTexture(0);
				glCheckError();
			}
		}
	}

	else {
		for (int i = 0; i<std::max(n, state.numUnits); i++) {
			setTextureUnit(i);
		}
	}

	setTextureUnit(prevUnit < n ? prevUnit : 0);
	state.numUnits = n;
}

void GLUtils::resetState()
{
}

void GLUtils::setViewPort(Rect4i r)
{
	if (state.viewport != r) {
		glViewport(r.getX(), r.getY(), r.getWidth(), r.getHeight());
		state.viewport = r;
	}
}

void GLUtils::setScissor(Rect4i r, bool enable)
{
	if (enable != state.scissoring) {
		if (enable) {
			glEnable(GL_SCISSOR_TEST);
		} else {
			glDisable(GL_SCISSOR_TEST);
		}
		state.scissoring = enable;
	}
	if (enable) {
		glScissor(r.getX(), r.getY(), r.getWidth(), r.getHeight());
	}
}

Halley::Rect4i GLUtils::getViewPort() const
{
	return state.viewport;
}

void GLUtils::clear(Colour col)
{
	if (!state.hasClearCol || col != state.clearCol) {
		glClearColor(col.r, col.g, col.b, col.a);
		state.clearCol = col;
		state.hasClearCol = true;
	}
	glClear(GL_COLOR_BUFFER_BIT);
}

