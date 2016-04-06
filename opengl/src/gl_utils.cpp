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


#ifdef _MSC_VER
#pragma warning(disable: 6286 6235)
#endif

using namespace Halley;

static const bool CHECKED = true;

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
			curBlend = Blend::Opaque;
			hasClearCol = false;
		}

		int curTexUnit;
		int numUnits;
		std::array<int, 8> curTex;
		Blend::Type curBlend;
		Rect4i viewport;
		Colour clearCol;
		bool scissoring;
		bool hasClearCol;
	};

}

#ifndef _MSC_VER
thread_specific_ptr<GLInternals> glState;
#endif

GLInternals& getState()
{
#ifdef _MSC_VER
	static _declspec(thread) GLInternals* glState;
	if (!glState) {
		glState = new GLInternals();
		std::cout << "GL state created on thread " << Concurrent::getThreadName() << std::endl;
	}
	return *glState;
#else
	auto result = glState.get();
	if (!result) {
		glState.reset(new GLInternals());
		std::cout << "GL state created on thread " << Concurrent::getThreadName() << std::endl;
		return *glState;
	}
	return *result;
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

void GLUtils::setBlendType(Blend::Type type)
{
	assert(type == Blend::Alpha || type == Blend::Alpha_Premultiplied || type == Blend::Add || type == Blend::Opaque || type == Blend::Test || type == Blend::Multiply || type == Blend::Darken);
	glCheckError();

	const Blend::Type curType = state.curBlend;
	if (!CHECKED || curType != type) {
		bool hasBlend = curType == Blend::Alpha || curType == Blend::Alpha_Premultiplied || curType == Blend::Add || curType == Blend::Multiply || curType == Blend::Darken;
		bool needsBlend = type == Blend::Alpha || type == Blend::Alpha_Premultiplied || type == Blend::Add || type == Blend::Multiply || type == Blend::Darken;
		bool hasTest = curType == Blend::Test;
		bool needsTest = type == Blend::Test;

		// Disable current
		if (hasBlend && !needsBlend) {
			glDisable(GL_BLEND);
			glCheckError();
		}
		else if (!hasBlend && needsBlend) {
			glEnable(GL_BLEND);
			glCheckError();
		}
		if (hasTest && !needsTest) {
			glDisable(GL_ALPHA_TEST);
			glCheckError();
		}
		else if (!hasTest && needsTest) {
			glEnable(GL_ALPHA_TEST);
			glCheckError();
			glAlphaFunc(GL_GREATER, 0.1f);
			glCheckError();
		}

		if (needsBlend) {
			if (type == Blend::Alpha_Premultiplied) {
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glCheckError();
			}
			else if (type == Blend::Alpha) {
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glCheckError();
			}
			else if (type == Blend::Multiply) {
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
	assert(n >= 0);
	assert(n < 8);

	if (!CHECKED || state.curTexUnit != n) {
		glActiveTexture(GL_TEXTURE0 + n);
		glCheckError();
		state.curTexUnit = n;
	}
}

void GLUtils::bindTexture(int id)
{
	assert(id >= 0);

	if (!CHECKED || id != state.curTex[state.curTexUnit]) {
		state.curTex[state.curTexUnit] = id;
		glBindTexture(GL_TEXTURE_2D, id);
		glCheckError();
	}
}

void GLUtils::setNumberOfTextureUnits(int n)
{
	assert(n >= 1);
	assert(n < 8);

	int prevUnit = state.curTexUnit;

	if (CHECKED) {
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

void GLUtils::setViewPort(Rect4i r, bool scissor)
{
	if (state.viewport != r) {
		glViewport(r.getX(), r.getY(), r.getWidth(), r.getHeight());
		state.viewport = r;
	}

	if (scissor != state.scissoring) {
		if (scissor) {
			glScissor(r.getX(), r.getY(), r.getWidth(), r.getHeight());
			glEnable(GL_SCISSOR_TEST);
		}
		else {
			glDisable(GL_SCISSOR_TEST);
		}
		state.scissoring = scissor;
	}
}

Halley::Rect4i GLUtils::getViewPort()
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

