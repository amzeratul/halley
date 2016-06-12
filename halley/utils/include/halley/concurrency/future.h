#pragma once

#include "executor.h"
#include <memory>
#include <atomic>
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <halley/support/exception.h>

namespace Halley
{
	template <typename T>
	class Task;

	struct VoidWrapper
	{};

	template <typename T>
	struct TaskHelper
	{
		template <typename F>
		struct FunctionHelper
		{
			F f;
			using ReturnType = typename std::result_of<F(T)>::type;

			static ReturnType call(F callback, T&& value)
			{
				return callback(std::move(value));
			}
		};

		using DataType = T;

		template <typename P, typename F>
		static void setPromise(P promise, F&& callable)
		{
			promise.setValue(std::move(callable()));
		}
	};

	template <>
	struct TaskHelper<void>
	{
		template <typename F>
		struct FunctionHelper
		{
			F f;
			using ReturnType = typename std::result_of<F()>::type;

			static ReturnType call(F callback, VoidWrapper&& value)
			{
				return callback();
			}
		};

		using DataType = VoidWrapper;

		template <typename P, typename F>
		static void setPromise(P promise, F&& callable)
		{
			callable();
			promise.set();
		}
	};

	template <typename T>
	class FutureData
	{
	public:
		FutureData() = default;
		FutureData(FutureData&&) = delete;
		FutureData(const FutureData&) = delete;
		FutureData& operator=(FutureData&&) = delete;
		FutureData& operator=(const FutureData&) = delete;

		void set(T&& value)
		{
			data = std::move(value);
			makeAvailable();
		}

		T get()
		{
			wait();
			return std::move(doGet<T>(0));
		}

		void wait()
		{
			if (!available) {
				boost::unique_lock<boost::mutex> lock(mutex);
				while (!available) {
					condition.wait(lock);
				}
			}
		}

		bool hasValue() const
		{
			return available.load();
		}

		void addContinuation(std::function<void(T)> f)
		{
			if (available.load()) {
				apply(f);
			} else {
				boost::unique_lock<boost::mutex> lock(mutex);
				if (available.load()) {
					lock.unlock();
					apply(f);
				} else {
					continuations.push_back(f);
				}
			}
		}

	private:
		void apply(std::function<void(T)> f) {
			f(doGet<T>(0));
		}

		template<typename T0>
		T0 doGet(typename std::enable_if<std::is_copy_constructible<T0>::value, int>::type)
		{
			if (!data.is_initialized()) {
				throw Exception("Data is not initialized.");
			}
			return data.get();
		}

		template<typename T0>
		T0 doGet(typename std::enable_if<!std::is_copy_constructible<T0>::value, int>::type)
		{
			if (!data.is_initialized()) {
				throw Exception("Data is not initialized.");
			}
			auto result = std::move(data.get());
			data.reset();
			return std::move(result);
		}

		void makeAvailable()
		{
			std::vector<std::function<void(T)>> toRun;

			{
				boost::unique_lock<boost::mutex> lock(mutex);
				toRun = std::move(continuations);
				continuations.clear();
				available.store(true);
				condition.notify_all();
			}

			for (auto f : toRun) {
				apply(f);
			}
		}

		std::atomic<bool> available;
		boost::optional<T> data;

		std::vector<std::function<void(T)>> continuations;
		boost::mutex mutex;
		boost::condition_variable condition;
	};

	template <typename T>
	class Future
	{
		using DataType = typename TaskHelper<T>::DataType;

	public:
		Future()
		{}

		Future(std::shared_ptr<FutureData<DataType>> data)
			: data(data)
		{}

		Future(const Future& o) = default;
		Future(Future&& f) = default;
		Future& operator=(const Future& o) = default;
		Future& operator=(Future&& o) = default;

		T get()
		{
			if (!data) {
				throw Exception("Future has not been bound.");
			}
			return data->get();
		}

		void wait()
		{
			if (!data) {
				throw Exception("Future has not been bound.");
			}
			return data->wait();
		}

		bool hasValue() const
		{
			return data && data->hasValue();
		}

		bool isReady() const
		{
			return hasValue();
		}

		template <typename F>
		auto then(F f) -> Future<typename TaskHelper<T>::template FunctionHelper<F>::ReturnType>
		{
			return then(ExecutionQueue::getDefault(), f);
		}

		template <typename E, typename F>
		auto then(E& e, F f)->Future<typename TaskHelper<T>::template FunctionHelper<F>::ReturnType>;

		template <typename F>
		auto thenNotify(F joinFuture) -> void
		{
			data->addContinuation([joinFuture](typename TaskHelper<T>::DataType v) mutable {
				joinFuture.notify();
			});
		}

	private:

		std::shared_ptr<FutureData<DataType>> data;
	};

	template <typename T>
	class Promise
	{
		using DataType = typename TaskHelper<T>::DataType;

	public:
		Promise()
			: futureData(std::make_shared<FutureData<DataType>>())
			, future(futureData)
		{
		}

		void setValue(T&& value)
		{
			futureData->set(std::move(value));
		}

		Future<T> getFuture()
		{
			return future;
		}

	private:
		std::shared_ptr<FutureData<DataType>> futureData;
		Future<T> future;
	};

	template <>
	class Promise<void>
	{
	public:
		Promise()
			: futureData(std::make_shared<FutureData<VoidWrapper>>())
			, future(futureData)
		{
		}

		void set()
		{
			futureData->set(VoidWrapper());
		}

		Future<void> getFuture() const
		{
			return future;
		}

	private:
		std::shared_ptr<FutureData<VoidWrapper>> futureData;
		Future<void> future;
	};

	class JoinFutureData
	{
	public:
		JoinFutureData(int n)
		{
			waitingFor.store(n);
		}

		bool notify()
		{
			// Lock-free, juggling razors here!
			while (true) {
				int prev = waitingFor;
				int next = waitingFor - 1;
				if (waitingFor.exchange(next) == prev) {
					if (next == 0) {
						// Last one, enqueue
						return true;
					}
					return false; // Swapped successfully
				}
				// Nope, race condition. Try again.
			}
		}

	private:
		std::atomic<int> waitingFor;
	};

	class JoinFuture
	{
	public:
		JoinFuture(int n)
			: data(std::make_shared<JoinFutureData>(n))
		{}

		void notify()
		{
			if (data->notify()) {
				promise.set();
			}
		}

		Future<void> getFuture() const
		{
			return promise.getFuture();
		}

	private:
		std::shared_ptr<JoinFutureData> data;
		Promise<void> promise;
	};


	template <typename T>
	class Task
	{
	public:
		Task()
		{}

		Task(std::function<T()> f)
			: payload(f)
		{}

		void setPayload(std::function<T()>&& f)
		{
			payload = std::move(f);
		}

		Future<T> enqueueOn(ExecutionQueue& e)
		{
			auto f = payload;
			auto p = promise;
			e.addToQueue([f, p]() mutable {
				TaskHelper<T>::setPromise(p, f);
			});
			return getFuture();
		}

		Future<T> enqueueOnDefault()
		{
			return enqueueOn(ExecutionQueue::getDefault());
		}

		Future<T> getFuture()
		{
			return promise.getFuture();
		}

	private:
		Promise<T> promise;
		std::function<T()> payload;
	};

	template<typename T>
	template<typename E, typename F>
	inline auto Future<T>::then(E & e, F f) -> Future<typename TaskHelper<T>::template FunctionHelper<F>::ReturnType>
	{
		using R = typename TaskHelper<T>::template FunctionHelper<F>::ReturnType;
		std::reference_wrapper<E> executor(e);

		auto task = Task<R>();
		data->addContinuation([task, f, executor](typename TaskHelper<T>::DataType v) mutable {
			auto f2 = std::bind(f, std::move(v));
			task.setPayload(std::move(f2));
			task.enqueueOn(executor.get());
		});
		return task.getFuture();
	}
}
