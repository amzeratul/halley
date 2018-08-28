#include "halley/bytes/fuzzer.h"
#include "halley/support/exception.h"

using namespace Halley;

Fuzzer::Fuzzer()
{
	std::vector<gsl::byte> initVec(128);
	Random::getGlobal().getBytes(initVec);
	rng.setSeed(reinterpret_cast<char*>(initVec.data()), initVec.size());

	setupMutators();
}

void Fuzzer::setCorpus(std::vector<Bytes> corpus)
{
	this->corpus = std::move(corpus);
}

void Fuzzer::setEvaluator(std::function<bool(const Bytes&)> function)
{
	evaluator = std::move(function);
}

void Fuzzer::setOptions(FuzzerOptions options)
{
	this->options = std::move(options);
	setupMutators();
}

FuzzerResults Fuzzer::runTrial(int n) const
{
	if (corpus.empty()) {
		throw Exception("Fuzzer failed to run, corpus is empty.", HalleyExceptions::Utils);
	}
	if (mutators.empty()) {
		throw Exception("Fuzzer failed to run, mutators are empty.", HalleyExceptions::Utils);
	}

	FuzzerResults results;
	for (int i = 0; i < n; ++i) {
		runRound(results);
	}
	return results;
}

void Fuzzer::setupMutators()
{
	mutators.emplace_back([=] (Bytes& data)
	{
		// Set random element to zero
		rng.getRandomElement(data) = 0;
	});
	mutators.emplace_back([=] (Bytes& data)
	{
		// Set random element to 0xFF
		rng.getRandomElement(data) = 0xFF;
	});
	mutators.emplace_back([=] (Bytes& data)
	{
		// Flip bits in random element
		auto& e = rng.getRandomElement(data);
		e = ~e;
	});
	mutators.emplace_back([=] (Bytes& data)
	{
		// Delete random element
		data.erase(data.begin() + rng.getRandomIndex(data));
	});
	mutators.emplace_back([=] (Bytes& data)
	{
		// Insert zero
		data.insert(data.begin() + rng.getRandomIndex(data), 0);
	});
	mutators.emplace_back([=] (Bytes& data)
	{
		// Insert random
		data.insert(data.begin() + rng.getRandomIndex(data), static_cast<unsigned char>(rng.getInt<int>(0, 0xFF)));
	});
}

void Fuzzer::runRound(FuzzerResults& results) const
{
	evaluate(mutate(select()), results);
}

const Bytes& Fuzzer::select() const
{
	return rng.getRandomElement(corpus);
}

Bytes Fuzzer::mutate(const Bytes& data) const
{
	const int nMutations = rng.getInt(options.minMutations, options.maxMutations);

	Bytes result = data;
	for (int i = 0; i < nMutations; ++i) {
		rng.getRandomElement(mutators)(result);
	}
	return result;
}

void Fuzzer::evaluate(const Bytes& data, FuzzerResults& results) const
{
	try {
		const bool evalResult = evaluator(data);
		if (evalResult) {
			results.successes++;
		} else {
			results.failures++;
		}
	} catch (...) {
		results.exceptions++;
	}
}
