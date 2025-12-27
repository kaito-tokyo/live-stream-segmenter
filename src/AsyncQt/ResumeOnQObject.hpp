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

#include <QMetaObject>
#include <QPointer>

namespace KaitoTokyo::AsyncQt {

class ResumeOnQObject {
public:
	ResumeOnQObject(QObject *context) : context_(context)
	{
		if (context_ == nullptr) {
			throw std::invalid_argument("ContextIsNullError(ResumeOnQObject::ResumeOnQObject)");
		}
	}

	bool await_ready() const
	{
		if (!context_) {
			throw std::runtime_error("ContextDeletedError(ResumeOnQObject::await_ready)");
		}

		return QThread::currentThread() == context_->thread();
	}

	void await_suspend(std::coroutine_handle<> h)
	{
		if (!context_) {
			error_ = std::make_exception_ptr(
				std::runtime_error("ContextDeletedError(ResumeOnQObject::await_suspend)"));
			h.resume();
			return;
		}

		bool queued = QMetaObject::invokeMethod(context_, [h]() mutable { h.resume(); }, Qt::QueuedConnection);

		if (!queued) {
			error_ = std::make_exception_ptr(
				std::runtime_error("InvokeMethodFailedError(ResumeOnQObject::await_suspend)"));
			h.resume();
		}
	}

	void await_resume()
	{
		if (error_) {
			std::rethrow_exception(error_);
		}

		if (!context_) {
			throw std::runtime_error("ContextDeletedError(ResumeOnQObject::await_resume)");
		}
	}

private:
	QPointer<QObject> context_;
	std::exception_ptr error_ = nullptr;
};

} // namespace KaitoTokyo::AsyncQt
