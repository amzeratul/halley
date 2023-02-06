#include "halley/tools/project/project_comments.h"
using namespace Halley;

ProjectComments::ProjectComments(Path commentsRoot)
	: commentsRoot(std::move(commentsRoot))
{
}
