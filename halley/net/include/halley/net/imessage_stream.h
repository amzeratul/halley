#pragma once

namespace Halley
{
	class IMessageStream
	{
	public:
		virtual ~IMessageStream() {}

		virtual bool isReliable() const = 0;
	};
}