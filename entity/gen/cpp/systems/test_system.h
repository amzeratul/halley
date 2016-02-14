#include "system.h"
#include "../components/test_component.h"
#include "../components/foo_component.h"
#include "../components/bar_component.h"

class TestSystem : public System {
public:
    TestSystem() : System({MainFamily::familyMaskValue, AuxFamily::familyMaskValue}) {}

protected:
    void tick(Time time) override; // Implement me

private:
    class MainFamily {
    public:
        TestComponent* const test;
        FooComponent* const foo;

        using Type = FamilyType<TestComponent, FooComponent>;
        static constexpr FamilyMaskType familyMaskValue = Type::getMask();
    private:
        MainFamily() = delete;
        ~MainFamily() = delete;
    };
    FamilyBinding<MainFamily> mainFamily;

    class AuxFamily {
    public:
        TestComponent* const test;
        BarComponent* const bar;

        using Type = FamilyType<TestComponent, BarComponent>;
        static constexpr FamilyMaskType familyMaskValue = Type::getMask();
    private:
        AuxFamily() = delete;
        ~AuxFamily() = delete;
    };
    FamilyBinding<AuxFamily> auxFamily;

};
