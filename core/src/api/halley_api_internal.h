#pragma once

union SDL_Event;

#include "../../utils/include/halley_utils.h"
#include "core_api.h"
#include "video_api.h"
#include "system_api.h"
#include "input_api.h"
#include "../graphics/uniform_type.h"

namespace Halley
{
	class MaterialParameter;

	class HalleyAPIInternal
	{
		friend class CoreRunner;
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
		virtual std::function<void(MaterialParameter& p)> getUniformBinding(unsigned int address, UniformType type, int n, void* data) = 0;
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
	};

	using CoreAPIInternal = CoreAPI;
}

