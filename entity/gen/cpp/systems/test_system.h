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
        EntityId entityId;

        TestComponent& test;
        FooComponent& foo;

        using Type = FamilyType<TestComponent, FooComponent>;
    };

    class AuxFamily {
    public:
        EntityId entityId;

        TestComponent& test;
        BarComponent& bar;

        using Type = FamilyType<TestComponent, BarComponent>;
    };

    Halley::FamilyBinding<MainFamily> mainFamily;
    Halley::FamilyBinding<AuxFamily> auxFamily;
};
