#pragma once
#include "halley/file/path.h"

namespace Halley {
    class ProjectComments {
    public:
        ProjectComments(Path commentsRoot);

    private:
        Path commentsRoot;
    };
}
