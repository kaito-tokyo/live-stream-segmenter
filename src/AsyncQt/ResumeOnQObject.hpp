/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo AsyncQt Library
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
#include <stdexcept>

#include <QMetaObject>
#include <QThread>

namespace KaitoTokyo::AsyncQt {

/**
 * @brief Transfers execution of the coroutine to the thread context of a specified QObject.
 *
 * @section design_rationale Design Rationale & Philosophy
 * This class is designed with a <b>"Zero-Overhead, Contract-Based"</b> philosophy.
 * It prioritizes runtime performance and minimal footprint over safety against misuse.
 *
 * <ul>
 * <li><b>Raw Pointer Usage:</b> It deliberately uses a raw `QObject*` instead of `QPointer` or `QWeakPointer`.
 * This avoids the overhead of global mutex locks and signal connections associated with weak pointers.</li>
 * <li><b>User Responsibility (Contract):</b> The user guarantees that the target `QObject` remains alive
 * until the coroutine resumes. If the object is destroyed while suspended, the coroutine will
 * intentionally leak (hang) rather than attempting a dangerous recovery or throwing exceptions.
 * Accessing a destroyed object constitutes Undefined Behavior (UB), which is acceptable for contract violations.</li>
 * <li><b>Forced Yielding:</b> `await_ready` always returns `false` to avoid the cost of thread ID checks.
 * This enforces a consistent behavior where the coroutine always yields to the event loop,
 * preventing event starvation even if called from the same thread.</li>
 * </ul>
 *
 * @warning <b>DO NOT add safety checks (e.g., QPointer, validity checks) to this class.</b>
 * Such additions would violate the core design philosophy of zero overhead.
 */
class ResumeOnQObject {
public:
	/**
     * @brief Constructs the awaiter with a target context.
     *
     * @param context The QObject whose thread will execute the continuation.
     * @throws std::invalid_argument if \p context is nullptr. This check exists solely to catch obvious bugs early.
     */
	explicit ResumeOnQObject(QObject *context) : context_(context)
	{
		if (context_ == nullptr) {
			throw std::invalid_argument("ContextIsNullError(ResumeOnQObject)");
		}
	}

	/**
     * @brief Always returns false to force a yield.
     *
     * @details Optimization for the "same thread" case (returning true) is intentionally omitted
     * to avoid the overhead of `QThread::currentThread()` checks and to ensure consistent
     * event loop processing behavior (post/yield).
     *
     * @return Always false.
     */
	bool await_ready() const noexcept { return false; }

	/**
     * @brief Suspends the coroutine and posts the continuation to the target context's event loop.
     *
     * @details This method uses `Qt::QueuedConnection` to schedule `h.resume()`.
     * If `context_` is destroyed after this call but before execution, Qt's `invokeMethod` mechanism
     * ensures the callback is simply discarded. In that case, the coroutine will never resume (leak),
     * which is the intended behavior for this edge case.
     *
     * @param h The handle to the suspended coroutine.
     */
	void await_suspend(std::coroutine_handle<> h) const
	{
		// Use invokeMethod with context_ as the context object.
		// Qt handles the cleanup if context_ dies pending the event.
		QMetaObject::invokeMethod(context_, [h]() mutable { h.resume(); }, Qt::QueuedConnection);
	}

	/**
     * @brief Resumes execution. No checks are performed.
     *
     * @details If the context was destroyed immediately before this call, accessing it in the
     * subsequent user code will result in Undefined Behavior (likely a crash).
     * This "Fail Fast" behavior is intentional.
     */
	void await_resume() const noexcept {}

private:
	QObject *context_;
};

} // namespace KaitoTokyo::AsyncQt
