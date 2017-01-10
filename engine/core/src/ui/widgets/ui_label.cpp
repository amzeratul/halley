#include "ui/widgets/ui_label.h"

using namespace Halley;

UILabel::UILabel(TextRenderer text)
	: UIWidget("", text.getExtents())
{
}
