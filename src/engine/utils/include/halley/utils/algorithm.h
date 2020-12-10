#pragma once

#include <set>
#include <map>
#include <vector>
#include <unordered_map>

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
	auto pickRandom(Iter begin, Iter end, R& rng) -> decltype(*begin)
	{
		if (begin == end) {
			return *begin;
		}
		const auto size = end - begin;
		return *(begin + rng.getInt(decltype(size)(0), size - 1));
	}


}

namespace std_ex {
	template <typename C, typename T>
	bool contains(const C& container, const T& elem)
	{
		return std::find(container.begin(), container.end(), elem) != container.end();
	}

	template <typename T>
	bool contains(const std::set<T>& container, const T& key)
	{
		return container.find(key) != container.end();
	}

	template <typename K, typename V>
	bool contains(const std::map<K, V>& container, const K& key)
	{
		return container.find(key) != container.end();
	}

	template <typename K, typename V>
	bool contains(const std::unordered_map<K, V>& container, const K& key)
	{
		return container.find(key) != container.end();
	}

	template <typename C, typename F>
	bool contains_if(const C& container, F predicate)
	{
		return std::find_if(container.begin(), container.end(), predicate) != container.end();
	}
}