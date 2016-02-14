#include "system.h"
#include "../components/test_component.h"
#include "../components/foo_component.h"
#include "../components/bar_component.h"

class TestSystem : public System {
public:
    TestSystem() : System({&mainFamily, &auxFamily}) {}

protected:
    void tick(Time time) override; // Implement me

private:
    class MainFamily {
    public:
        EntityId entityId;

        TestComponent& test;
        FooComponent& foo;

        using Type = FamilyType<TestComponent, FooComponent>;
        static constexpr FamilyMaskType familyMaskValue = Type::mask;
    private:
        MainFamily() = delete;
        ~MainFamily() = delete;
    };
    FamilyBinding<MainFamily> mainFamily;

    class AuxFamily {
    public:
        EntityId entityId;

        TestComponent& test;
        BarComponent& bar;

        using Type = FamilyType<TestComponent, BarComponent>;
        static constexpr FamilyMaskType familyMaskValue = Type::mask;
    private:
        AuxFamily() = delete;
        ~AuxFamily() = delete;
    };
    FamilyBinding<AuxFamily> auxFamily;

};
