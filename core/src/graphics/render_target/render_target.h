/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "../texture.h"

namespace Halley {
	class RenderTarget {
	public:
		virtual ~RenderTarget() {}

		virtual Vector2f getSize() const = 0;
		virtual Vector2f getViewSize() const = 0;
		virtual Vector2f getOrigin() const = 0;

	protected:
		virtual void bind(int attachment, bool preserveCurrent) = 0;
		virtual void unbind() = 0;

		static std::shared_ptr<RenderTarget> curTarget;
	};

	class TextureRenderTarget : public RenderTarget {
	public:
		virtual ~TextureRenderTarget() {}
		virtual void setTarget(std::shared_ptr<Texture> tex)=0;
		virtual std::shared_ptr<Texture> getTexture() const=0;
		virtual std::shared_ptr<Texture> getDepthTexture() const = 0;
	};
}
