#pragma once

#include "stage.h"
#include "halley/graphics/sprite/sprite_painter.h"

namespace Halley {
    class IRenderTest {
    public:
        virtual ~IRenderTest() = default;

        void init(Resources& resources, const HalleyAPI& api, SpritePainter& spritePainter);
        virtual void onInit() {}
        virtual void update(Time dt, Time totalTime) {}
        virtual void render(RenderContext& rc) const;

    protected:
        Resources* resources = nullptr;
        const HalleyAPI* api = nullptr;
        SpritePainter* spritePainter = nullptr;
    };

    class SSBOTest : public IRenderTest {
    public:
	    void onInit() override;
	    void update(Time dt, Time totalTime) override;

    private:
        Sprite sprite;
        SpritePainter painter;
    };

    class RenderTestStage : public Stage {
    public:
        enum class TestType {
            SSBOTest
        };

	    void init() override;

	    void onVariableUpdate(Time dt) override;
	    void onRender(RenderContext& rc) const override;

    	bool hasMultithreadedRendering() const override;

    private:
        std::unique_ptr<IRenderTest> curTest;
        Time elapsedTime = 0;
        mutable SpritePainter spritePainter;

    	void setTestType(TestType testType);
        std::unique_ptr<IRenderTest> makeTest(TestType testType) const;
    };
}
