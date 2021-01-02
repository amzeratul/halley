#include "task_details.h"
using namespace Halley;

TaskDetails::TaskDetails(UIFactory& factory)
	: UIWidget("taskDetails", {}, UISizer())
	, factory(factory)
{
}
