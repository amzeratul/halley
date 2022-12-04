#pragma once

namespace Halley {
   	using IndexType = unsigned short;

    enum class PrimitiveType
    {
        Triangle
    };

    enum class TimestampType {
	    FrameStart,
        FrameEnd,
        CommandStart,
        CommandSetupDone,
        CommandEnd
    };
}
