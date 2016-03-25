#include "system.h"
#include "../components/test_component.h"
#include "../components/foo_component.h"
#include "../components/bar_component.h"

// Generated file; do not modify.
class TestSystem : public Halley::System {
public:
    TestSystem() : System({&mainFamily, &auxFamily}) {}

protected:
    void tick(Halley::Time time) override; // Implement me

private:
    class MainFamily {
    public:
        const EntityId entityId;

        TestComponent* const test;
        FooComponent* const foo;

        using Type = Halley::FamilyType<TestComponent, FooComponent>;
    };

    class AuxFamily {
    public:
        const EntityId entityId;

        TestComponent* const test;
        BarComponent* const bar;

        using Type = Halley::FamilyType<TestComponent, BarComponent>;
    };

    Halley::FamilyBinding<MainFamily> mainFamily;
    Halley::FamilyBinding<AuxFamily> auxFamily;
};
