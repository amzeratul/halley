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

#include "halley/text/string_converter.h"

namespace Halley {

	using Time = double;

	enum class TimeLine
	{
		FixedUpdate,
		VariableUpdate,
		VariableUpdateUI,
		Render,
		NUMBER_OF_TIMELINES
	};

	template <>
	struct EnumNames<TimeLine> {
		constexpr auto operator()() const {
			return std::to_array({
				"fixedUpdate",
				"variableUpdate",
				"variableUpdateUI",
				"render"
			});
		}
	};
	
}
