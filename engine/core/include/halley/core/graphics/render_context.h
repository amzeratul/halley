#pragma once
#include "painter.h"

namespace Halley
{
	class Camera;
	class RenderTarget;

	class RenderContext
	{
		friend class Core;

	public:
		void bind(std::function<void(Painter&)> f)
		{
			pushContext();
			setActive();
			f(painter);
			setInactive();
			popContext();
		}

		RenderContext(RenderContext&& context);

		RenderContext with(Camera& camera) const;
		RenderContext with(RenderTarget& target) const;
		RenderContext subArea(Rect4i area) const;

		Camera& getCamera() const { return camera; }
		Rect4i getViewPort() const { return viewPort; }
		RenderTarget& getRenderTarget() const { return renderTarget; }

	private:
		Painter& painter;
		Camera& camera;
		RenderTarget& renderTarget;
		Rect4i viewPort;

		RenderContext* restore = nullptr;

		RenderContext(Painter& painter, Camera& camera, RenderTarget& renderTarget, Rect4i viewPort);
		void setActive();
		void setInactive();
		void pushContext();
		void popContext();
	};
}
