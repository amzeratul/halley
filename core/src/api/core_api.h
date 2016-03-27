#pragma once

namespace Halley
{
	class CoreAPI
	{
	public:
		virtual ~CoreAPI() {}
		virtual void quit() = 0;
	};
}