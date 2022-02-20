#pragma once

#include <set>
#include <map>
#include "halley/data_structures/vector.h"

#include "halley/data_structures/hash_map.h"

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
		return *(begin + rng.getSizeT(decltype(size)(0), size - 1));
	}

	template <typename T>
	struct DeReference {
		auto operator()(T& v) -> decltype(*T())
		{
			return *v;
		}
	};

	template <typename T>
	struct DontDeReference {
		T& operator()(T& v)
		{
			return v;
		}
	};

	template<typename Iter, typename W, typename R, typename F>
	auto pickRandomWeighted(Iter begin, Iter end, W weightFunc, R& rng, F deRefFunc) -> Iter
	{
		using WeightType = decltype(weightFunc(deRefFunc(begin)));
		WeightType totalWeight = 0;

		for (Iter iter = begin; iter != end; ++iter) {
			totalWeight += weightFunc(deRefFunc(iter));
		}
	
		if (totalWeight <= 0) {
			return end;
		}

		const WeightType pick = rng.get(0, totalWeight);
		WeightType accum = 0;
		for (Iter iter = begin; iter != end; ) {
			accum += weightFunc(deRefFunc(iter));
			if (accum > pick) {
				return iter;
			}

			auto next = ++iter;
			if (next == end) {
				return iter;
			}
			iter = next;
		}

		return end;
	}

	template<typename Iter, typename W, typename R, typename F>
	auto pickRandomWeightedOneCall(Iter begin, Iter end, W weightFunc, R& rng, F deRefFunc) -> Iter
	{
		using WeightType = decltype(weightFunc(deRefFunc(begin)));
		const size_t n = static_cast<size_t>(end - begin);

		WeightType totalWeight = 0;
		Vector<WeightType> weights;
		weights.resize(n);

		for (Iter iter = begin; iter != end; ++iter) {
			const auto w = weightFunc(deRefFunc(iter));
			weights[iter - begin] = w;
			totalWeight += w;
		}
	
		if (totalWeight <= 0) {
			return end;
		}

		const WeightType pick = rng.get(0, totalWeight);
		WeightType accum = 0;
		for (Iter iter = begin; iter != end; ) {
			accum += weights[iter - begin];
			if (accum > pick) {
				return iter;
			}

			auto next = ++iter;
			if (next == end) {
				return iter;
			}
			iter = next;
		}

		return end;
	}

	template<typename Iter, typename W, typename R>
	auto pickRandomWeighted(Iter begin, Iter end, W weightFunc, R& rng)
	{
		return pickRandomWeighted(begin, end, weightFunc, rng, DeReference<Iter>());
	}

	template<typename Iter, typename W, typename R>
	auto pickRandomWeightedOneCall(Iter begin, Iter end, W weightFunc, R& rng)
	{
		return pickRandomWeightedOneCall(begin, end, weightFunc, rng, DeReference<Iter>());
	}

	template<typename Iter, typename R>
	void shuffle(Iter begin, Iter end, R& rng)
	{
		if (begin == end) {
			return;
		}

		size_t n = end - begin - 1;
		for(size_t i = 0; i < n; ++i) {
			size_t j = rng.getSizeT(i, n);
			std::swap(begin[i], begin[j]);
		}
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

	template <typename K, typename V, typename C>
	bool contains(const std::map<K, V>& container, const C& key)
	{
		return container.find(key) != container.end();
	}

	template <typename K, typename V>
	bool contains(const Halley::HashMap<K, V>& container, const K& key)
	{
		return container.find(key) != container.end();
	}

	template <typename C, typename F>
	bool contains_if(const C& container, F predicate)
	{
		return std::find_if(container.begin(), container.end(), predicate) != container.end();
	}

	template <typename C, typename F>
	void erase_if_value(C& map, F predicate)
	{
		const auto endIter = map.end();
		for (auto iter = map.begin(); iter != endIter;) {
		     if (predicate(iter->second)) {
		          iter = map.erase(iter);
		     } else {
		          ++iter;
		     }
		}		
	}

	template <typename C, typename F>
	void erase_if_key(C& map, F predicate)
	{
		const auto endIter = map.end();
		for (auto iter = map.begin(); iter != endIter;) {
		     if (predicate(iter->first)) {
		          iter = map.erase(iter);
		     } else {
		          ++iter;
		     }
		}		
	}

	template <typename C, typename V>
	void erase(C& container, const V& value)
	{
		container.erase(std::remove(container.begin(), container.end(), value), container.end());
	}

	template <typename C, typename F>
	void erase_if(C& container, F predicate)
	{
		container.erase(std::remove_if(container.begin(), container.end(), predicate), container.end());
	}

	template <typename C, typename V>
	auto find(const C& container, const V& value)
	{
		return std::find(container.begin(), container.end(), value);
	}

	template <typename C, typename V>
	auto find(C& container, const V& value)
	{
		return std::find(container.begin(), container.end(), value);
	}

	template <typename C, typename F>
	auto find_if(const C& container, F predicate)
	{
		return std::find_if(container.begin(), container.end(), predicate);
	}

	template <typename C, typename F>
	auto find_if(C& container, F predicate)
	{
		return std::find_if(container.begin(), container.end(), predicate);
	}

	template <typename C, typename S>
	auto move_push_back(C& destination, S& source)
	{
		destination.reserve(destination.size() + source.size());
		for (const auto& s: source) {
			destination.push_back(std::move(s));
		}
		source.clear();
	};
}
