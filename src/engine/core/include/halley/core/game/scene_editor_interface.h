#pragma once

#include "halley/entity/entity_id.h"
#include "halley/time/halleytime.h"

namespace Halley {
    class RenderContext;
    class World;
    class HalleyAPI;
    class Resources;
    class UIFactory;
    class IUIElement;

    class SceneEditorContext {
    public:
        const HalleyAPI* api;
        Resources* resources;
    };

    class ComponentEditorContext {
    public:
        ComponentEditorContext(UIFactory& factory)
            : factory(factory)
        {}

        UIFactory& getFactory() { return factory; }

    private:
        UIFactory& factory;
    };

    class IComponentEditorFieldFactory {
    public:
        virtual ~IComponentEditorFieldFactory() = default;
    	virtual String getFieldType() = 0;
        virtual std::shared_ptr<IUIElement> createField(ComponentEditorContext& context, const String& fieldName, ConfigNode& componentData, const String& defaultValue) = 0;
    };
	
    class ISceneEditor {
    public:
        virtual ~ISceneEditor() = default;

        virtual void init(SceneEditorContext& context) = 0;
        virtual void update(Time t) = 0;
        virtual void render(RenderContext& rc) = 0;
    	
        virtual World& getWorld() = 0;
        virtual void spawnPending() = 0;

        virtual EntityId getCameraId() = 0;
        virtual void dragCamera(Vector2f amount) = 0;
        virtual void changeZoom(int amount, Vector2f cursorPosRelToCamera) = 0;

    	virtual std::vector<std::unique_ptr<IComponentEditorFieldFactory>> getComponentEditorFieldFactories() = 0;
    };

	class ISceneData {
	public:
		virtual ~ISceneData() = default;

		virtual ConfigNode getEntityData(const String& id) = 0;
	};
}
