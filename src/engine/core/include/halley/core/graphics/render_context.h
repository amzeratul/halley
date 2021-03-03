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

		RenderContext(const RenderContext& context) noexcept;
		RenderContext(RenderContext&& context) noexcept;

		RenderContext with(const Camera& camera) const;
		RenderContext with(RenderTarget& defaultRenderTarget) const;
		const Camera& getCamera() const { return camera; }

		RenderTarget& getDefaultRenderTarget() const;

		void flush();

	private:
		Painter& painter;
		const Camera& camera;
		RenderTarget& defaultRenderTarget;

		RenderContext* restore = nullptr;

		RenderContext(Painter& painter, const Camera& camera, RenderTarget& renderTarget);
		void setActive();
		void setInactive();
		void pushContext();
		void popContext();
	};
}
