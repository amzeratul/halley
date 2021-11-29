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

#pragma once

#include "halleystring.h"
#include <gsl/span>

namespace Halley {
	namespace Encode {
		String encodeBase16(gsl::span<const Byte> in);
		void decodeBase16(std::string_view in, gsl::span<Byte> bytes);
		String encodeBase16(const Bytes& in);

		String encodeBase64(const Bytes& in);
		Bytes decodeBase64(const String& in);

		Vector<char> encodeRLE(const Vector<char>& in);
		Vector<char> decodeRLE(const Vector<char>& in);
	}
}