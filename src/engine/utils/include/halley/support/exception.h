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

namespace Halley {
	namespace HalleyExceptions {
		enum Type {
			Unknown = 0,
			
			SystemPlugin = 100,
			VideoPlugin = 101,
			AudioOutPlugin = 102,
			PlatformPlugin = 103,
			NetworkPlugin = 104,
			InputPlugin = 105,
			MoviePlugin = 106,
			
			Core = 200,
			Entity = 201,
			UI = 202,
			AudioEngine = 203,
			Concurrency = 204,
			File = 205,
			OS = 206,
			Resources = 207,
			Tools = 208,
			Lua = 209,
			Utils = 210,
			Network = 211,
			Graphics = 212,
			Input = 213,

			LastHalleyReserved = 999
		};
	}

	class Exception : public std::exception {
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

	private:
		String msg;
		String stackTrace;
		String fullMsg;
		int errorCode = 0;
	};
}
