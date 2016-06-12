#pragma once

#include <memory>
#include <atomic>
#include <boost/optional.hpp>
#include <halley/support/exception.h>

namespace Halley
{
	struct VoidWrapper
	{};

	template <typename T>
	struct TaskHelper
	{
		template <typename F>
		struct FunctionHelper
		{
			F f;
			using ReturnType = decltype(f(*(T*)nullptr));

			static ReturnType call(F callback, T&& value)
			{
				return callback(value);
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
			using ReturnType = decltype(f());

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
			return data.get();
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
				f(data.get());
			} else {
				boost::unique_lock<boost::mutex> lock(mutex);
				if (available.load()) {
					lock.unlock();
					f(data.get());
				} else {
					continuations.push_back(f);
				}
			}
		}

	private:
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
				f(data.get());
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
		auto then(F f) -> Future<typename TaskHelper<T>::FunctionHelper<F>::ReturnType>
		{
			return then(Executor::getDefault(), f);
		}

		template <typename E, typename F>
		auto then(E& e, F f) -> Future<typename TaskHelper<T>::FunctionHelper<F>::ReturnType>
		{
			using R = TaskHelper<T>::FunctionHelper<F>::ReturnType;
			auto task = Task<R>();
			std::reference_wrapper<E> executor(e);
			data->addContinuation([task, f, executor](typename TaskHelper<T>::DataType v) mutable {
				auto f2 = f;
				task.setPayload([f2, v]() mutable -> R {
					return TaskHelper<T>::FunctionHelper<F>::call(f2, std::move(v));
				});
				task.enqueueOn(executor.get());
			});
			return task.getFuture();
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

		Future<void> getFuture()
		{
			return future;
		}

	private:
		std::shared_ptr<FutureData<VoidWrapper>> futureData;
		Future<void> future;
	};
}
