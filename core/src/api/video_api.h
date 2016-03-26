#pragma once

namespace Halley
{
	class VideoAPI
	{
	public:
		void flip();

	private:
		friend class HalleyAPI;

		void init();
		void deInit();
	};
}
