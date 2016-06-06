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

}
