#pragma once
#include "halley/data_structures/vector.h"
#include "halley/utils/utils.h"
#include "halley/maths/random.h"

namespace Halley
{
	struct FuzzerOptions
	{
		int minMutations = 1;
		int maxMutations = 5;
	};

	struct FuzzerResults
	{
		int successes = 0;
		int failures = 0;
		int exceptions = 0;
	};

	class Fuzzer
	{
	public:
		Fuzzer();
		void setCorpus(Vector<Bytes> corpus);
		void setEvaluator(std::function<bool(const Bytes&)> function);
		void setOptions(FuzzerOptions options);

		FuzzerResults runTrial(int n) const;

	private:
		mutable Random rng;

		using Mutator = std::function<void(Bytes&)>;

		Vector<Bytes> corpus;
		Vector<Mutator> mutators;
		std::function<bool(const Bytes&)> evaluator;
		FuzzerOptions options;

		void setupMutators();

		void runRound(FuzzerResults& results) const;
		const Bytes& select() const;
		Bytes mutate(const Bytes& data) const;
		void evaluate(const Bytes& data, FuzzerResults& results) const;
	};
}
