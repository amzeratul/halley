#pragma once

namespace Halley {
	class NullableReference;

	class NullableReferenceAnchor {
		friend class NullableReference;

	public:
		~NullableReferenceAnchor();

	private:
		NullableReference* firstReference = nullptr;

		void addReference(NullableReference* ref);
	};

	class NullableReference {
		friend class NullableReferenceAnchor;

	public:
		NullableReference(NullableReferenceAnchor& anchor);
		~NullableReference();
		NullableReferenceAnchor& get() const;
		bool isValid() const;

	private:
		NullableReferenceAnchor* ref = nullptr;
		NullableReference* prev = nullptr;
		NullableReference* next = nullptr;

		void nullify();
	};
}
