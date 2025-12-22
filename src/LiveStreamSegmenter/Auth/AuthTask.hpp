/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This software is licensed under the MIT License.
 * For the full text of the license, see the file "LICENSE.MIT"
 * in the distribution root.
 */

#pragma once

#include <coroutine>
#include <exception>
#include <utility>

template <typename T>
struct Task {
	struct promise_type {
		T value_;
		std::exception_ptr exception_;

		Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		void return_value(T v) { value_ = std::move(v); }
		void unhandled_exception() { exception_ = std::current_exception(); }
	};

	std::coroutine_handle<promise_type> handle_;

	explicit Task(std::coroutine_handle<promise_type> h) : handle_(h) {}
	~Task() { if (handle_) handle_.destroy(); }

	bool await_ready() { return handle_.done(); }
	void await_suspend(std::coroutine_handle<>) {}
	T await_resume() {
		if (handle_.promise().exception_) std::rethrow_exception(handle_.promise().exception_);
		return std::move(handle_.promise().value_);
	}
};
