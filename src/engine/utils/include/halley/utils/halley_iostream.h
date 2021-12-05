#pragma once

#include <ostream>
#include <halley/maths/vector2.h>
#include <halley/maths/vector3.h>
#include <halley/maths/vector4.h>

#include "halley/file/path.h"

namespace Halley {
	template <typename T,class U>
	std::ostream& operator<< (std::ostream& ostream, const Vector2D<T, U>& v)
	{
		ostream << "(" << v.x << "," << v.y << ")" ; return ostream;
	}

	template <typename T >
	std::ostream& operator<< (std::ostream& ostream, const Vector4D<T>& v)
	{
		ostream << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")" ; return ostream;
	}

	std::ostream& operator<<(std::ostream& os, const Path& p)
	{
		return (os << p.string());
	}
}
