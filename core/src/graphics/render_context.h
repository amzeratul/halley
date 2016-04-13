#pragma once
#include "painter.h"

namespace Halley
{
	class Camera;
	class RenderTarget;

	class RenderContext
	{
		friend class CoreRunner;

	public:
		void bind(std::function<void(Painter&)> f)
		{
			pushContext();
			setActive();
			f(painter);
			popContext();
		}

		RenderContext(RenderContext&& context);

		RenderContext with(Camera& camera) const;
		RenderContext with(RenderTarget& camera) const;
		RenderContext subArea(Rect4f area) const;

	private:
		Painter& painter;
		Camera& camera;
		RenderTarget& renderTarget;

		RenderContext* restore = nullptr;

		RenderContext(Painter& painter, Camera& camera, RenderTarget& renderTarget);
		void setActive();
		void pushContext();
		void popContext();
	};
}
