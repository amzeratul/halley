#pragma once

class System
{
public:
	virtual ~System() {}
	void step();

protected:
	virtual void doStep() = 0;

private:
	int nsTaken;
};
