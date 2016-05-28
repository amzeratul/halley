#pragma once

union SDL_Event;

#include "halley/halley_utils.h"
#include "core_api.h"
#include "video_api.h"
#include "system_api.h"
#include "input_api.h"
#include "halley/core/graphics/material/uniform_type.h"

namespace Halley
{
	class MaterialParameter;

	class HalleyAPIInternal
	{
		friend class Core;
		friend class HalleyAPI;

	public:
		virtual ~HalleyAPIInternal() {}

		virtual void init() = 0;
		virtual void deInit() = 0;
		virtual void processEvent(SDL_Event& event) = 0;
	};

	class VideoAPIInternal : public VideoAPI, public HalleyAPIInternal
	{
	public:
		virtual ~VideoAPIInternal() {}

		virtual std::unique_ptr<Painter> makePainter() = 0;
		virtual std::function<void(int, void*)> getUniformBinding(UniformType type, int n) = 0;
	};

	class SystemAPIInternal : public SystemAPI, public HalleyAPIInternal
	{
	public:
		virtual ~SystemAPIInternal() {}
	};

	class InputAPIInternal : public InputAPI, public HalleyAPIInternal
	{
	public:
		virtual ~InputAPIInternal() {}

		virtual void beginEvents(Time t) = 0;
	};

	using CoreAPIInternal = CoreAPI;
}

