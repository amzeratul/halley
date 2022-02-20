#pragma once

#include "executor.h"
#include <memory>
#include <atomic>
#include <halley/data_structures/maybe.h>
#include <thread>
#include <mutex>
#include <condition_variable>
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
			using ReturnType = typename std::result_of<F(T)>::type;

			static ReturnType call(F callback, T&& value)
			{
				return callback(std::move(value));
			}
		};

		using DataType = T;

		template <typename P, typename F>
		static void setPromise(P& promise, F&& callable)
		{
			if (!promise.isCancelled()) {
				promise.setValue(std::move(callable()));
			}
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
		static void setPromise(P& promise, F&& callable)
		{
			if (!promise.isCancelled()) {
				callable();
				promise.set();
			}
		}
	};

	template <typename T>
	class FutureData
	{
	public:
		FutureData()
			: available(false)
			, cancelled(false)
		{}

		FutureData(T data)
			: available(true)
			, cancelled(false)
			, data(std::move(data))
		{
			makeAvailable();
		}

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
			return doGet<T>();
		}

		void wait()
		{
			if (!available) {
				std::unique_lock<std::mutex> lock(mutex);
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
				std::unique_lock<std::mutex> lock(mutex);
				if (available.load()) {
					lock.unlock();
					apply(f);
				} else {
					continuations.push_back(f);
				}
			}
		}

		void cancel()
		{
			cancelled = true;
		}

		bool isCancelled() const
		{
			return cancelled;
		}

	private:
		void apply(std::function<void(T)> f) {
			f(doGet<T>());
		}

		template<typename T0>
		T0 doGet()
		{
			if (!data) {
				throw Exception("Data is not initialized.", HalleyExceptions::Utils);
			}

			if constexpr (std::is_copy_constructible<T0>::value) {
				return *data;
			} else {
				auto result = std::move(*data);
				data.reset();
				return result;
			}
		}

		void makeAvailable()
		{
			Vector<std::function<void(T)>> toRun;

			{
				std::unique_lock<std::mutex> lock(mutex);
				toRun = std::move(continuations);
				continuations.clear();
				available.store(true);
				condition.notify_all();
			}

			for (auto& f : toRun) {
				apply(f);
			}
		}

		std::atomic<bool> available;
		std::atomic<bool> cancelled;
		std::optional<T> data;

		Vector<std::function<void(T)>> continuations;
		std::mutex mutex;
		std::condition_variable condition;
	};

	template <typename T>
	class Future
	{
		using DataType = typename TaskHelper<T>::DataType;

	public:
		Future()
		{}

		Future(std::shared_ptr<FutureData<DataType>> data)
			: data(std::move(data))
		{}

		Future(const Future& o) = default;
		Future(Future&& f) = default;
		Future& operator=(const Future& o) = default;
		Future& operator=(Future&& o) = default;

		DataType get()
		{
			if (!data) {
				throw Exception("Future has not been bound.", HalleyExceptions::Utils);
			}
			if (data->isCancelled()) {
				throw Exception("Future was cancelled.", HalleyExceptions::Utils);
			}
			return data->get();
		}

		void wait()
		{
			if (!data) {
				throw Exception("Future has not been bound.", HalleyExceptions::Utils);
			}
			if (data->isCancelled()) {
				throw Exception("Waiting on cancelled future is not supported.", HalleyExceptions::Utils);
			}
			return data->wait();
		}

		void cancel()
		{
			if (!data) {
				throw Exception("Future has not been bound.", HalleyExceptions::Utils);
			}
			data->cancel();
		}

		bool hasValue() const
		{
			return data && data->hasValue();
		}

		bool isReady() const
		{
			return hasValue();
		}

		bool isValid() const
		{
			return !!data;
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
			data->addContinuation([joinFuture](typename TaskHelper<T>::DataType /*v*/) mutable {
				joinFuture.notify();
			});
		}

		static Future<T> makeImmediate(DataType value)
		{
			return Future<T>(std::make_shared<FutureData<DataType>>(std::move(value)));
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

		void setValue(T value)
		{
			Expects(futureData != nullptr);
			futureData->set(std::move(value));
		}

		Future<T> getFuture()
		{
			return future;
		}

		bool isCancelled() const
		{
			Expects(futureData != nullptr);
			return futureData->isCancelled();
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

		bool isCancelled() const
		{
			return futureData->isCancelled();
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

		int notify()
		{
			std::unique_lock<std::mutex> lock(mutex);
			return --waitingFor;
			/*
			// Lock-free, juggling razors here!
			while (true) {
				int prev = waitingFor;
				int next = prev - 1;
				if (waitingFor.exchange(next) == prev) {
					return next; // Swapped successfully
				}
				// Nope, race condition. Try again.
			}
			*/
		}

	private:
		std::mutex mutex;
		std::atomic<int> waitingFor;
	};

	class JoinFuture
	{
	public:
		JoinFuture(int n)
			: data(std::make_shared<JoinFutureData>(n))
		{
			if (n == 0) {
				promise.set();
			}
		}

		void notify()
		{
			int nLeft = data->notify();
			if (nLeft == 0) {
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
	class MovableFunctionBase
	{
	public:
		virtual ~MovableFunctionBase() {}
		virtual T operator()() = 0;
	};

	template <typename T>
	class MovableStdFunction final : public MovableFunctionBase<T>
	{
	public:
		MovableStdFunction(std::function<T()> f)
			: f(f)
		{}

		T operator()() override {
			return f();
		}

	private:
		std::function<T()> f;
	};

	template <typename T, typename U>
	class MovableBoundFunction final : public MovableFunctionBase<T>
	{
	public:
		MovableBoundFunction(std::function<T(U&&)> f, U&& v)
			: f(f)
			, v(std::move(v))
		{}

		T operator()() override {
			return f(std::move(v));
		}

	private:
		std::function<T(U&&)> f;
		U v;
	};

	template <typename T>
	class MovableFunction
	{
	public:
		MovableFunction()
		{}

		MovableFunction(std::function<T()> f)
			: f(std::make_shared<MovableStdFunction<T>>(f))
		{}

		template <typename F, typename U>
		MovableFunction(F f, U&& v)
			: f(std::make_shared<MovableBoundFunction<T, U>>(f, std::move(v)))
		{}

		T operator()() {
			return (*f)();
		}

	private:
		std::shared_ptr<MovableFunctionBase<T>> f;
	};

	template <typename T>
	class TaskQueueHelper
	{
	public:
		[[nodiscard]] static Future<T> enqueueOn(ExecutionQueue& e, MovableFunction<T> payload)
		{
			Promise<T> promise;
			enqueueOn(e, std::move(payload), promise);
			return promise.getFuture();
		}
		
		static void enqueueOn(ExecutionQueue& e, MovableFunction<T> payload, Promise<T> promise)
		{
			e.addToQueue([payload(std::move(payload)), promise(promise)]() mutable {
				TaskHelper<T>::setPromise(promise, payload);
			});
		}
	};

	template<typename T>
	template<typename E, typename F>
	inline auto Future<T>::then(E & e, F f) -> Future<typename TaskHelper<T>::template FunctionHelper<F>::ReturnType>
	{
		using R = typename TaskHelper<T>::template FunctionHelper<F>::ReturnType;
		std::reference_wrapper<E> executor(e);

		auto promise = Promise<R>();
		data->addContinuation([promise, f, executor](typename TaskHelper<T>::DataType v) mutable {
			TaskQueueHelper<R>::enqueueOn(executor.get(), MovableFunction<R>(f, std::move(v)), promise);
		});
		return promise.getFuture();
	}
}
