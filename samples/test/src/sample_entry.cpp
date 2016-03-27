#include <iostream>
#include <vector>
#include "prec.h"

class TestData : public PrivateEngineData
{
public:
	int value = 0;
};

class TestRunner : public IRunner
{
public:
	PrivateEngineData* initialize(std::vector<std::string>) override
	{
		return new TestData();
	}

	bool step(PrivateEngineData* data) override
	{
		std::cout << ".";
		return true;
	}

	void terminate(PrivateEngineData* data) override
	{
		delete data;
	}
};

extern "C" {
	__declspec(dllexport) IRunner* __stdcall HalleyGetRunner();
	__declspec(dllexport) void __stdcall HalleyDestroyRunner(IRunner*);
}

IRunner* _stdcall HalleyGetRunner()
{
	return new TestRunner();
}

void _stdcall HalleyDestroyRunner(IRunner* runner)
{
	delete runner;
}
