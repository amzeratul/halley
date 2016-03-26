#pragma once

namespace Halley
{
	class CoreAPI
	{
	public:
		unsigned int getTicks();
		void delay(unsigned int ms);

	private:
		friend class HalleyAPI;

		void init();
		void deInit();
	};
}