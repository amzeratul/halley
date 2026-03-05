#pragma once

#include <string_view>

namespace Halley {
	namespace HalleyExceptions {
		enum Type {
			Unknown = 0,

			Assert = 10,
			
			SystemPlugin = 100,
			VideoPlugin = 101,
			AudioOutPlugin = 102,
			PlatformPlugin = 103,
			NetworkPlugin = 104,
			InputPlugin = 105,
			MoviePlugin = 106,
			AnalyticsPlugin = 107,
			
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
			Compression = 214,
			Scripting = 215,
			Web = 216,
			DataStructures = 217,

			LastHalleyReserved = 999
		};

		[[noreturn]] void throwException(std::string_view msg, int code);
	}
}
