#pragma once

namespace Halley
{

	template <typename Iter, typename T, typename F>
	Iter findHighestScore(Iter begin, Iter end, T startScore, F eval)
	{
		Iter bestResult = end;
		T bestScore = startScore;
		for (Iter i = begin; i != end; ++i) {
			T score = eval(*i);
			if (score > bestScore) {
				bestScore = score;
				bestResult = i;
			}
		}
		return bestResult;
	}

	template <typename Iter, typename T, typename F>
	Iter findLowestScore(Iter begin, Iter end, T startScore, F eval)
	{
		Iter bestResult = end;
		T bestScore = startScore;
		for (Iter i = begin; i != end; ++i) {
			T score = eval(*i);
			if (score < bestScore) {
				bestScore = score;
				bestResult = i;
			}
		}
		return bestResult;
	}

	template<typename Iter, typename F>
	auto filter(Iter begin, Iter end, F predicate) -> Vector<typename std::remove_reference<decltype(*begin)>::type> {
		Vector<typename std::remove_reference<decltype(*begin)>::type> result;
		for (Iter i = begin; i != end; ++i) {
			if (predicate(*i)) {
				result.push_back(*i);
			}
		}
		return result;
	}

	template<typename Iter, typename F>
	auto filterRef(Iter begin, Iter end, F predicate) -> Vector<std::reference_wrapper<typename std::remove_reference<decltype(*begin)>::type>>
	{
		decltype(filterRef(begin, end, predicate)) result;
		for (Iter i = begin; i != end; ++i) {
			if (predicate(*i)) {
				result.push_back(*i);
			}
		}
		return result;
	}

	template<typename Iter, typename R>
	auto pickRandom(Iter begin, Iter end, R rng) -> decltype(*begin)
	{
		assert(begin != end);
		auto size = end - begin;
		return *(begin + rng.getInt(decltype(size)(0), size - 1));
	}


}
