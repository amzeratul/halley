#include "halley/data_structures/nullable_reference.h"
#include <gsl/gsl_assert>

using namespace Halley;

NullableReferenceAnchor::~NullableReferenceAnchor()
{
	for (auto ref = firstReference; ref; ref = ref->next) {
		ref->nullify();
	}
	firstReference = nullptr;
}

void NullableReferenceAnchor::addReference(NullableReference* ref)
{
	if (firstReference) {
		firstReference->prev = ref;
		ref->next = firstReference;
	}
	firstReference = ref;
}

NullableReference::NullableReference(NullableReferenceAnchor& anchor)
{
	anchor.addReference(this);
}

NullableReference::~NullableReference()
{
	if (ref) {
		if (prev) {
			prev->next = next;
		} else {
			// First element
			ref->firstReference = next;
		}

		if (next) {
			next->prev = prev;
		}
	}
}

void NullableReference::nullify()
{
	ref = nullptr;
}

NullableReferenceAnchor& NullableReference::get() const
{
	Expects(ref);
	return *ref;
}

bool NullableReference::isValid() const
{
	return ref != nullptr;
}
