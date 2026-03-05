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

#include <exception>
#include "halley/text/halleystring.h"
#include "exception_type.h"

namespace Halley {

	class Exception final : public std::exception {
	public:
		Exception() = default;
		Exception(String msg, int errorCode) noexcept;
		Exception(const Exception& other) noexcept = default;
		Exception(Exception&& other) noexcept = default;
		~Exception() noexcept {}

		Exception& operator=(const Exception& other) = default;

		const char* what() const noexcept override;

		const String& getMessage() const;
		const String& getStackTrace() const;
		const String& getFullMessage() const;
		int getErrorCode() const;

		static void setCollectStackTrace(bool collect);

	private:
		String msg;
		String stackTrace;
		String fullMsg;
		int errorCode = 0;

		static bool collectStackTrace;
	};
}
