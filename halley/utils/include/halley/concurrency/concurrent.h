#pragma once
#include "executor.h"
#include <functional>
#include "future.h"
#include "task.h"
#include <halley/text/halleystring.h>

namespace Halley
{
	namespace Concurrent
	{
		template <typename F>
		auto execute(F f) -> Future<decltype(f())>
		{
			return execute(Executor::getDefault(), Task<decltype(f())>(f));
		}

		template <typename T>
		auto execute(Task<T> task) -> Future<T>
		{
			return execute(Executor::getDefault(), task);
		}

		template <typename F>
		auto execute(Executor& e, F f) -> Future<decltype(f())>
		{
			return execute(e, Task<decltype(f())>(f));
		}

		template <typename T>
		auto execute(Executor& e, Task<T> task) -> Future<T>
		{
			return task.enqueueOn(e);
		}

		template <typename Iter>
		auto whenAll(Iter begin, Iter end) -> Future<void>
		{
			JoinFuture future(int(end - begin));
			for (Iter i = begin; i != end; ++i) {
				(*i).thenNotify(future);
			}
			return future.getFuture();
		}

		template <typename T, typename F>
		void foreach(T begin, T end, F&& f)
		{
			foreach(Executor::getDefault(), begin, end, std::move(f));
		}

		template <typename T, typename F>
		void foreach(Executor& e, T begin, T end, F&& f)
		{
			const size_t n = end - begin;
			constexpr size_t maxThreads = 8;
			size_t nThreads = std::min(maxThreads, e.threadCount());
			std::array<Future<void>, maxThreads> futures;

			size_t prevEnd = 0;
			for (size_t j = 0; j < nThreads; ++j) {
				size_t curStart = prevEnd;
				size_t curEnd = n * (j + 1) / nThreads;
				prevEnd = curEnd;

				futures[j] = execute([begin, f, curStart, curEnd]() {
					for (auto i = begin + curStart; i < begin + curEnd; ++i) {
						f(*i);
					}
				});
			}

			whenAll(futures.data(), futures.data() + nThreads).wait();
		}

		void setThreadName(String name);
		String getThreadName();
	}
}
