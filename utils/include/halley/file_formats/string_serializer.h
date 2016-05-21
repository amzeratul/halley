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

#include "../text/halleystring.h"
#include "../maths/vector2d.h"
#include "../maths/vector3d.h"
#include "../maths/colour.h"

namespace Halley {

	class StringDeserializer {
	public:
		StringDeserializer(String value);
		operator Vector2i();
		operator Vector2f();
		operator Vector3i();
		operator Vector3f();
		operator String();
		operator int();
		operator float();
		operator bool();
		operator Colour();

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
