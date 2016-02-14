#pragma once

#include <initializer_list>
#include "family_binding.h"
#include "family_type.h"

using Time = float;

class System
{
public:
	System(std::initializer_list<FamilyMaskType> familyMasks);
	virtual ~System() {}
	void step();

protected:
	virtual void tick(Time time) = 0;

private:
	int nsTaken;
};

