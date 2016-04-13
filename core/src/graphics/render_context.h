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
		void bind(std::function<void(Painter&)> f) const
		{
			painter.bind(camera, renderTarget);
			f(painter);
		}

		RenderContext(RenderContext&& context);

		RenderContext with(Camera& camera) const;
		RenderContext with(RenderTarget& camera) const;
		RenderContext subArea(Rect4f area) const;

	private:
		Painter& painter;
		Camera& camera;
		RenderTarget& renderTarget;

		RenderContext(Painter& painter, Camera& camera, RenderTarget& renderTarget);
	};
}
