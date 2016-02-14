#include "component.h"
class TestComponent : public Component {
public:
    constexpr static int componentIndex = 0;

    float elapsed;
    int test;

};
