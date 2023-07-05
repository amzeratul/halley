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
#include "halley_gl.h"

using namespace Halley;

void GLUtils::doGlCheckError(const char* file, long line)
{
#ifdef _DEBUG
	const bool alwaysCheck = false;
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
			throw Exception(msg, HalleyExceptions::VideoPlugin);
		}
	}
}


#ifdef _MSC_VER
#pragma warning(disable: 6286 6235)
#endif

using namespace Halley;

#ifdef _DEBUG
static constexpr bool checked = false;
#else
static constexpr bool checked = true;
#endif

namespace Halley {
	class GLInternals {
	public:
		GLInternals()
		{
			for (int i = 0; i < 16; i++) {
				curTex[i] = 0;
			}
			numUnits = 0;
			curTexUnit = 0;
			curBlend = BlendType();
			viewport = {0, 0, 0, 0};
			scissoring = false;
		}

		int curTexUnit;
		int numUnits;
		std::array<unsigned int, 16> curTex;
		BlendType curBlend;
		Rect4i viewport;
		bool scissoring;
	};

}

GLInternals& getState()
{
	thread_local GLInternals state;
	return state;
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
	glCheckError();

	const BlendType curType = state.curBlend;
	if (!checked || curType != type) {
		bool hasBlend = curType.hasBlend();
		bool needsBlend = type.hasBlend();

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
			switch (type.mode) {
			case BlendMode::Alpha:
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(type.premultiplied ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BlendMode::Add:
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(type.premultiplied ? GL_ONE : GL_SRC_ALPHA, GL_ONE);
				break;
			case BlendMode::Multiply:
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BlendMode::Invert:
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BlendMode::Max:
				glBlendEquation(GL_MAX);
				glBlendFunc(type.premultiplied ? GL_ONE : GL_SRC_ALPHA, GL_ONE);
				break;
			case BlendMode::Min:
				glBlendEquation(GL_MIN);
				glBlendFunc(type.premultiplied ? GL_ONE : GL_SRC_ALPHA, GL_ONE);
				break;
			default:
				break;
			}
			glCheckError();
		}

		// Update current
		state.curBlend = type;
	}
}

static GLenum getDepthComparison(DepthStencilComparisonFunction func)
{
	switch (func) {
	case DepthStencilComparisonFunction::Always:
		return GL_ALWAYS;
	case DepthStencilComparisonFunction::Never:
		return GL_NEVER;
	case DepthStencilComparisonFunction::Less:
		return GL_LESS;
	case DepthStencilComparisonFunction::LessEqual:
		return GL_LEQUAL;
	case DepthStencilComparisonFunction::Greater:
		return GL_GREATER;
	case DepthStencilComparisonFunction::GreaterEqual:
		return GL_GEQUAL;
	case DepthStencilComparisonFunction::Equal:
		return GL_EQUAL;
	case DepthStencilComparisonFunction::NotEqual:
		return GL_NOTEQUAL;
	}
	return GL_ALWAYS;
}

void GLUtils::setDepthStencil(const MaterialDepthStencil& depthStencil)
{
	if (depthStencil.isDepthTestEnabled()) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
	glCheckError();

	glDepthMask(depthStencil.isDepthWriteEnabled() ? GL_TRUE : GL_FALSE);
	glCheckError();

	glDepthFunc(getDepthComparison(depthStencil.getDepthComparisonFunction()));
	glCheckError();
}

void GLUtils::setTextureUnit(int n)
{
	Expects(n >= 0 && n < 16);

	if (!checked || (state.curTexUnit != n)) {
		glActiveTexture(GL_TEXTURE0 + n);
		glCheckError();
		state.curTexUnit = n;
		state.numUnits = std::max(state.numUnits, n + 1);
	}
}

void GLUtils::bindTexture(unsigned int id)
{
	if (!checked || (id != state.curTex[state.curTexUnit])) {
		state.curTex[state.curTexUnit] = id;
		glBindTexture(GL_TEXTURE_2D, id);
		glCheckError();
	}
}

void GLUtils::resetTextureUnits()
{
	for (int i = state.numUnits - 1; i >= 0; i--) {
		setTextureUnit(i);
		bindTexture(0);
	}

	state.numUnits = 0;
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

void GLUtils::resetDefaultGLState()
{
    glDisable(GL_BLEND);
    glClearColor(0, 0, 0, 0);
    glDisable(GL_SCISSOR_TEST);

	glDisable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	glDepthMask(GL_TRUE);
	glClearDepth(1.0);
	glDepthRange(0.0, 1.0);
}
