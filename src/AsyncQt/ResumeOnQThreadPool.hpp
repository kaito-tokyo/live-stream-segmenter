/*
 * KaitoTokyo AsyncQt Library
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <coroutine>
#include <memory>
#include <stdexcept>

#include <QThreadPool>

namespace KaitoTokyo::AsyncQt {

/**
 * @brief Transfers execution of the coroutine to a thread managed by the specified QThreadPool.
 *
 * @section design_rationale Design Rationale & Philosophy
 * This class is designed with a <b>"Zero-Overhead, Contract-Based"</b> philosophy.
 * It prioritizes runtime performance and minimal footprint over safety against misuse.
 *
 * <ul>
 * <li><b>Raw Pointer Usage:</b> It deliberately uses a raw `QThreadPool*`.
 * This avoids any overhead associated with smart pointers or lifetime tracking mechanisms.</li>
 * <li><b>User Responsibility (Contract):</b> The user guarantees that the target `QThreadPool` remains alive
 * until the coroutine is suspended and the task is submitted. If the pool is destroyed while
 * the coroutine is waiting to run, the behavior depends on QThreadPool's destruction policy,
 * but generally, pending tasks may be discarded (leak) or cause UB if accessed.
 * Ensuring the pool outlives the coroutine tasks is the user's responsibility.</li>
 * <li><b>Forced Yielding:</b> `await_ready` always returns `false`.
 * This ensures that the execution explicitly moves to the thread pool, avoiding accidental
 * synchronous execution on the current thread which could block the UI or caller.</li>
 * </ul>
 *
 * @warning <b>DO NOT add safety checks (e.g., validity checks, smart pointers) to this class.</b>
 * Such additions would violate the core design philosophy of zero overhead.
 */
class ResumeOnQThreadPool {
public:
	/**
	 * @brief Constructs the awaiter with a target thread pool.
	 *
	 * @param threadPool_ The QThreadPool to which the coroutine will be submitted.
	 * @throws std::invalid_argument if \p threadPool_ is nullptr. This check exists solely to catch obvious bugs early.
	 */
	ResumeOnQThreadPool(QThreadPool *threadPool_) : threadPool(threadPool_)
	{
		if (threadPool_ == nullptr) {
			throw std::invalid_argument("ThreadPoolIsNullError");
		}
	}

	/**
	 * @brief Always returns false to force execution on the thread pool.
	 *
	 * @return Always false.
	 */
	bool await_ready() const noexcept { return false; }

	/**
	 * @brief Suspends the coroutine and submits the continuation to the thread pool.
	 *
	 * @param h The handle to the suspended coroutine.
	 */
	void await_suspend(std::coroutine_handle<> h) const
	{
		// Contract: threadPool must be valid at this point.
		// If QThreadPool::start fails (rare, e.g., if strictly limited),
		// it is usually handled by Qt internal queuing, but exceptions here are not caught
		// to maintain zero-overhead.
		threadPool->start([h]() mutable { h.resume(); });
	}

	/**
	 * @brief Resumes execution. No checks are performed.
	 */
	void await_resume() const noexcept {}

private:
	QThreadPool *threadPool;
};

} // namespace KaitoTokyo::AsyncQt
