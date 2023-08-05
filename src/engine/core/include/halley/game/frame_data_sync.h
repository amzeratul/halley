#pragma once

#include "frame_data.h"
#include "halley/utils/algorithm.h"

namespace Halley {
    template<typename T>
    class FrameDataSync {
    public:
        // Update thread
    	bool write(T v)
    	{
            const auto curFrame = BaseFrameData::hasCurrent() ? BaseFrameData::getCurrent().frameIdx : 0;
        	std::unique_lock lock(mutex);

            for (const auto& d: data) {
	            if (d.first >= curFrame) {
                    return false;
	            }
            }

            data.emplace_back(curFrame, std::move(v));
            return true;
    	}

        // Render thread
    	std::optional<T> read()
        {
            const auto curFrame = BaseFrameData::hasCurrent() ? BaseFrameData::getCurrent().frameIdx : 0;
        	std::unique_lock lock(mutex);

            std::optional<T> result;
            for (auto& d: data) {
	            if (d.first == curFrame) {
                    result = std::move(d.second);
	            } 
            }
            std_ex::erase_if(data, [&] (const auto& e) { return e.first <= curFrame; });
            return result;
        }

    private:
        std::mutex mutex;
        Vector<std::pair<int, T>> data;
    };
}
