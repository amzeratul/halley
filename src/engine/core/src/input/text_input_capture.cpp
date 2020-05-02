#include "input/text_input_capture.h"
#include <memory>

using namespace Halley;

TextInputCapture::TextInputCapture(TextInputData& inputData, SoftwareKeyboardData softKeyboardData, std::unique_ptr<ITextInputCapture> _capture)
	: capture(std::move(_capture))
{
	capture->open(inputData, std::move(softKeyboardData));
}

TextInputCapture::~TextInputCapture()
{
	if (capture) {
		capture->close();
	}
}

bool TextInputCapture::update() const
{
	if (capture->isOpen()) {
		capture->update();
	}
	return capture->isOpen();
}
