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
		void bind(const std::function<void(Painter&)>& f)
		{
			pushContext();
			setActive();
			f(painter);
			setInactive();
			popContext();
		}

		RenderContext(RenderContext&& context) noexcept;

		RenderContext with(Camera& camera) const;
		RenderContext with(RenderTarget& defaultRenderTarget) const;
		Camera& getCamera() const { return camera; }

		RenderTarget& getDefaultRenderTarget();

		void flush();

	private:
		Painter& painter;
		Camera& camera;
		RenderTarget& defaultRenderTarget;

		RenderContext* restore = nullptr;

		RenderContext(Painter& painter, Camera& camera, RenderTarget& renderTarget);
		void setActive();
		void setInactive();
		void pushContext();
		void popContext();
	};
}
