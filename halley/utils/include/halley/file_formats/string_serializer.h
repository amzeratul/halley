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

  Copyright (c) 2007-2014 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "halley/text/halleystring.h"
#include "halley/maths/vector2d.h"
#include "halley/maths/vector3d.h"
#include "halley/maths/colour.h"

namespace Halley {

	class StringDeserializer {
	public:
		StringDeserializer(String value);
		operator Vector2i() const;
		operator Vector2f() const;
		operator Vector3i() const;
		operator Vector3f() const;
		operator String() const;
		operator int() const;
		operator float() const;
		operator bool() const;
		operator Colour() const;

	private:
		String value;
	};

	class StringSerializer {
	public:
		static String encode(Vector2i v);
		static String encode(Vector2f v);
		static String encode(Vector3i v);
		static String encode(Vector3f v);
		static String encode(String v);
		static String encode(int v);
		static String encode(float v);
		static String encode(bool v);
		static String encode(Colour c);

		static StringDeserializer decode(String v);
	};

}
