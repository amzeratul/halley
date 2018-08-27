/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2014 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "halley/text/string_serializer.h"
#include "halley/text/string_converter.h"

using namespace Halley;

String StringSerializer::encode(Vector2i v)
{
	return toString(v);
}

String StringSerializer::encode(Vector2f v)
{
	return toString(v);
}

String StringSerializer::encode(Vector3i v)
{
	return toString(v);
}

String StringSerializer::encode(Vector3f v)
{
	return toString(v);
}

String StringSerializer::encode(String v)
{
	return v;
}

String StringSerializer::encode(int v)
{
	return toString(v);
}

String StringSerializer::encode(float v)
{
	return toString(v);
}

String StringSerializer::encode(bool v)
{
	return toString(v);
}

Halley::String Halley::StringSerializer::encode(Colour c)
{
	return c.toString();
}

StringDeserializer StringSerializer::decode(String v)
{
	return StringDeserializer(v);
}

StringDeserializer::StringDeserializer(String v)
	: value(v)
{
}

StringDeserializer::operator Vector2i() const
{
	auto values = value.split(',');
	return Vector2i(values[0].toInteger(), values[1].toInteger());
}

StringDeserializer::operator Vector2f() const
{
	auto values = value.split(',');
	return Vector2f(values[0].toFloat(), values[1].toFloat());
}

StringDeserializer::operator Vector3i() const
{
	auto values = value.split(',');
	return Vector3i(values[0].toInteger(), values[1].toInteger(), values[2].toInteger());
}

StringDeserializer::operator Vector3f() const
{
	auto values = value.split(',');
	return Vector3f(values[0].toFloat(), values[1].toFloat(), values[2].toFloat());
}

StringDeserializer::operator String() const
{
	return value;
}

StringDeserializer::operator int() const
{
	return value.toInteger();
}

StringDeserializer::operator float() const
{
	return value.toFloat();
}

StringDeserializer::operator bool() const
{
	return value.asciiLower() == "true";
}

StringDeserializer::operator Colour() const
{
	return Colour::fromString(value);
}
