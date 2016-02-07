#include "system.h"
class TestSystem : public System {
public:
    // TODO: this requires 1 families.
protected:
    void doStep() override;
};