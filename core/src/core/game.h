#pragma once

namespace Halley
{
	class Game
	{
	public:
		virtual ~Game() = default;

		virtual String getName() const = 0;
		virtual String getDataPath() const = 0;
		virtual bool isDevBuild() const = 0;

		virtual void init() {}
		virtual void deInit() {}
	};
}