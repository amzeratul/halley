#include "../../core/include/halley_main.h"

class SampleGame : public Halley::Game
{
public:
	Halley::String getName() const override
	{
		return "Sample game";
	}

	Halley::String getDataPath() const override
	{
		return "halley/sample";
	}

	bool isDevBuild() const override
	{
		return true;
	}
};

HalleyGame(SampleGame);
