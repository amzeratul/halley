// Halley codegen version 132
#pragma once

#include <halley.hpp>

#include "halley/entity/services/painter_service.h"
#include "halley/entity/services/screen_service.h"
#include "halley/entity/services/dev_service.h"


// Generated file; do not modify.
template <typename T>
class CameraRenderSystemBase : private Halley::System {
public:
	CameraRenderSystemBase()
		: System({}, {})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	const Halley::HalleyAPI& getAPI() const {
		return doGetAPI();
	}
	Halley::Resources& getResources() const {
		return doGetResources();
	}
	Halley::TempMemoryPool& getTempMemoryPool() const {
		return doGetWorld().getRenderMemoryPool();
	}

	PainterService& getPainterService() const {
		return *painterService;
	}

	DevService& getDevService() const {
		return *devService;
	}

	ScreenService& getScreenService() const {
		return *screenService;
	}

private:
	friend Halley::System* halleyCreateCameraRenderSystem();

	PainterService* painterService{ nullptr };
	DevService* devService{ nullptr };
	ScreenService* screenService{ nullptr };
	void initBase() override final {
		painterService = &doGetWorld().template getService<PainterService>(getName());
		devService = &doGetWorld().template getService<DevService>(getName());
		screenService = &doGetWorld().template getService<ScreenService>(getName());
		invokeInit<T>(static_cast<T*>(this));
	}

	void renderBase(Halley::RenderContext& rc) override final {
		static_cast<T*>(this)->render(rc);
	}

};

