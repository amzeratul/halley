#include "preferences.h"

Halley::Preferences::Preferences(String path) 
	: filePath(path)
{}

void Halley::Preferences::addRecent(String path)
{
	
}

Halley::Rect4i Halley::Preferences::getWindowRect() const
{
	return Rect4i(0, 0, 1280, 720);
}

void Halley::Preferences::save() const
{
	
}

void Halley::Preferences::load()
{
	
}
