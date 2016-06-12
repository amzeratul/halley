#pragma once

namespace Halley
{
	template <typename T>
	class Task
	{
	public:
		Task()
		{}

		Task(std::function<T()> f)
			: payload(f)
		{}

		void setPayload(std::function<T()> f)
		{
			payload = f;
		}

		Future<T> enqueueOn(Executor& e)
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
			return enqueueOn(Executor::getDefault());
		}

		Future<T> getFuture()
		{
			return promise.getFuture();
		}

	private:
		Promise<T> promise;
		std::function<T()> payload;
	};
}