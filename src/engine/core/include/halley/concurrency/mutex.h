#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <shared_mutex>

namespace Halley {

	namespace Detail {
		template <typename MType>
		class MutexBase {
		public:
			using MutexType = MType;

			MutexBase()
#ifdef __PROSPERO__
				: mutex(nullptr)
#else
				: mutex()
#endif
			{
			}

			MutexType& getStdMutex() { return mutex; }
			const MutexType& getStdMutex() const { return mutex; }

		private:
			MutexType mutex;
		};

		template <typename LockType, typename M>
		class LockBase {
		public:
			LockBase() = default;

			LockBase(M& mutex)
				: lockObj(mutex.getStdMutex())
			{
			}

			LockBase(M& mutex, std::defer_lock_t t)
				: lockObj(mutex.getStdMutex(), t)
			{
			}

			LockBase(M& mutex, std::try_to_lock_t t)
				: lockObj(mutex.getStdMutex(), t)
			{
			}

			LockBase(M& mutex, std::adopt_lock_t t)
				: lockObj(mutex.getStdMutex(), t)
			{
			}

			void lock() { lockObj.lock(); }
			bool tryLock() { return lockObj.try_lock(); }
			void unlock() { lockObj.unlock(); }

			LockType& getLock() { return lockObj; }
			const LockType& getLock() const { return lockObj; }

		private:
			LockType lockObj;
		};

		template <typename ConditionVariableType>
		class ConditionVariableBase {
		public:
			ConditionVariableBase()
#ifdef __PROSPERO__
				: cond(nullptr)
#else
				: cond()
#endif
			{
			}

			void notifyOne() { cond.notify_one(); }
			void notifyAll() { cond.notify_all(); }

			template <typename Lock>
			void wait(Lock& lock)
			{
				cond.wait(lock.getLock());
			}

			template <typename Lock, typename Rep, typename Period>
			bool waitFor(Lock& lock, const std::chrono::duration<Rep, Period>& time)
			{
				return cond.wait_for(lock.getLock(), time) == std::cv_status::no_timeout;
			}

		private:
			ConditionVariableType cond;
		};
	}

	template <typename M>
	class UniqueLock : public Detail::LockBase<std::unique_lock<typename M::MutexType>, M> {
	public:
		UniqueLock() = default;

		UniqueLock(M& mutex)
			: Detail::LockBase<std::unique_lock<typename M::MutexType>, M>(mutex)
		{
		}

		UniqueLock(M& mutex, std::defer_lock_t t)
			: Detail::LockBase<std::unique_lock<typename M::MutexType>, M>(mutex, t)
		{
		}

		UniqueLock(M& mutex, std::try_to_lock_t t)
			: Detail::LockBase<std::unique_lock<typename M::MutexType>, M>(mutex, t)
		{
		}

		UniqueLock(M& mutex, std::adopt_lock_t t)
			: Detail::LockBase<std::unique_lock<typename M::MutexType>, M>(mutex, t)
		{
		}
	};

	template <typename M>
	class SharedLock : public Detail::LockBase<std::shared_lock<typename M::MutexType>, M> {
	public:
		SharedLock(M& mutex)
			: Detail::LockBase<std::shared_lock<typename M::MutexType>, M>(mutex)
		{
		}

		SharedLock(M& mutex, std::defer_lock_t t)
			: Detail::LockBase<std::shared_lock<typename M::MutexType>, M>(mutex, t)
		{
		}

		SharedLock(M& mutex, std::try_to_lock_t t)
			: Detail::LockBase<std::shared_lock<typename M::MutexType>, M>(mutex, t)
		{
		}

		SharedLock(M& mutex, std::adopt_lock_t t)
			: Detail::LockBase<std::shared_lock<typename M::MutexType>, M>(mutex, t)
		{
		}
	};

	using Mutex = Detail::MutexBase<std::mutex>;
	using SharedMutex = Detail::MutexBase<std::shared_mutex>;

	/*
	template<typename M>
	using UniqueLock = Detail::LockBase<std::unique_lock<typename M::MutexType>, M>;

	template<typename M>
	using SharedLock = Detail::LockBase<std::shared_lock<typename M::MutexType>, M>;
	*/

	using ConditionVariable = Detail::ConditionVariableBase<std::condition_variable>;
	using ConditionVariableAny = Detail::ConditionVariableBase<std::condition_variable_any>;

};
