#pragma once

namespace Halley {
	class NullableReferenceAnchor;
	
	class NullableReference {
		friend class NullableReferenceAnchor;

	public:
		~NullableReference();

		NullableReference();
		NullableReference(const NullableReference& other);
		NullableReference(NullableReference&& other);

		NullableReference& operator=(const NullableReference& other);
		NullableReference& operator=(NullableReference&& other);

		bool isValid() const;

	protected:
		NullableReferenceAnchor& getRaw() const;

	private:
		NullableReferenceAnchor* ref = nullptr;
		NullableReference* prev = nullptr;
		NullableReference* next = nullptr;

		NullableReference(NullableReferenceAnchor& anchor);
		void nullify();
		void setReference(NullableReferenceAnchor* anchor);
	};

	template <typename T>
	class NullableReferenceOf : public NullableReference {
	public:
		NullableReferenceOf()
		{}

		NullableReferenceOf(const NullableReference& other)
			: NullableReference(other)
		{}
		
		NullableReferenceOf(NullableReference&& other)
			: NullableReference(other)
		{}
		
		NullableReferenceOf(const NullableReferenceOf& other)
			: NullableReference(other)
		{}
		
		NullableReferenceOf(NullableReferenceOf&& other)
			: NullableReference(other)
		{}

		NullableReferenceOf& operator=(const NullableReference& other)
		{
			NullableReference::operator=(other);
			return *this;
		}
		
		NullableReferenceOf& operator=(NullableReference&& other)
		{
			NullableReference::operator=(other);
			return *this;
		}

		NullableReferenceOf& operator=(const NullableReferenceOf& other)
		{
			NullableReference::operator=(other);
			return *this;
		}
		
		NullableReferenceOf& operator=(NullableReferenceOf&& other)
		{
			NullableReference::operator=(other);
			return *this;
		}

		T& get() const
		{
			return reinterpret_cast<T&>(getRaw());
		}

		T& operator*() const
		{
			return get();
		}
	};

	class NullableReferenceAnchor {
		friend class NullableReference;

	public:
		NullableReferenceAnchor();
		NullableReferenceAnchor(NullableReferenceAnchor&& other);
		NullableReferenceAnchor& operator=(NullableReferenceAnchor&& other);

		NullableReferenceAnchor(const NullableReferenceAnchor& other) = delete;
		NullableReferenceAnchor& operator=(const NullableReferenceAnchor& other) = delete;

		~NullableReferenceAnchor();
		
		NullableReference getReference();

		template <typename T>
		NullableReferenceOf<T> getReferenceOf()
		{
			return NullableReferenceOf<T>(getReference());
		}

	private:
		NullableReference* firstReference = nullptr;

		void addReference(NullableReference* ref);
	};
}
