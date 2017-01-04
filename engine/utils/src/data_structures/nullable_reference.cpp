#include "halley/data_structures/nullable_reference.h"
#include <gsl/gsl_assert>

using namespace Halley;

NullableReferenceAnchor::NullableReferenceAnchor()
{
}

NullableReferenceAnchor::NullableReferenceAnchor(NullableReferenceAnchor&& other)
{
	firstReference = other.firstReference;
	other.firstReference = nullptr;
}

NullableReferenceAnchor& NullableReferenceAnchor::operator=(NullableReferenceAnchor&& other)
{
	firstReference = other.firstReference;
	other.firstReference = nullptr;
	return *this;
}

NullableReferenceAnchor::~NullableReferenceAnchor()
{
	for (auto ref = firstReference; ref; ref = ref->next) {
		ref->nullify();
	}
	firstReference = nullptr;
}

NullableReference NullableReferenceAnchor::getReference()
{
	return NullableReference(*this);
}

void NullableReferenceAnchor::addReference(NullableReference* ref)
{
	if (firstReference) {
		firstReference->prev = ref;
		ref->next = firstReference;
	}
	firstReference = ref;
}

NullableReference::NullableReference()
{
}

NullableReference::NullableReference(const NullableReference& other)
{
	setReference(other.ref);
}

NullableReference::NullableReference(NullableReference&& other)
{
	setReference(other.ref);
}

NullableReference& NullableReference::operator=(const NullableReference& other)
{
	setReference(other.ref);
	return *this;
}

NullableReference& NullableReference::operator=(NullableReference&& other)
{
	setReference(other.ref);
	return *this;
}

NullableReference::NullableReference(NullableReferenceAnchor& anchor)
{
	setReference(&anchor);
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
	next = nullptr;
	prev = nullptr;
}

void NullableReference::setReference(NullableReferenceAnchor* anchor)
{
	if (anchor != ref) {
		nullify();
		ref = anchor;
		ref->addReference(this);
	}
}

NullableReferenceAnchor& NullableReference::getRaw() const
{
	Expects(ref);
	return *ref;
}

bool NullableReference::isValid() const
{
	return ref != nullptr;
}
