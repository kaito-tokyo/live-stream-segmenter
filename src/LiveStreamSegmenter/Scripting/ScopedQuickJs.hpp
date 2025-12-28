/*
 * Live Stream Segmenter - Scripting Module
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

#include <memory>

#include <quickjs.h>

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

struct JSRuntimeDeleter {
	void operator()(JSRuntime *rt) { JS_FreeRuntime(rt); }
};
using UniqueJSRuntime = std::unique_ptr<JSRuntime, JSRuntimeDeleter>;

struct JSContextDeleter {
	void operator()(JSContext *ctx) { JS_FreeContext(ctx); }
};
using UniqueJSContext = std::unique_ptr<JSContext, JSContextDeleter>;

class ScopedJSValue {
public:
	ScopedJSValue(JSContext *ctx, JSValue v) : ctx_(ctx), v_(v) {}

	ScopedJSValue(ScopedJSValue &&other) noexcept : ctx_(other.ctx_), v_(other.v_) { other.v_ = JS_UNDEFINED; }

	~ScopedJSValue()
	{
		if (!JS_IsUndefined(v_)) {
			JS_FreeValue(ctx_, v_);
		}
	}

	operator JSValue() const { return v_; }

	JSValue get() const { return v_; }

	ScopedJSValue(const ScopedJSValue &) = delete;
	ScopedJSValue &operator=(const ScopedJSValue &) = delete;

private:
	JSContext *ctx_;
	JSValue v_;
};

class ScopedJSString {
public:
	ScopedJSString(JSContext *ctx, const char *str) : ctx_(ctx), str_(str) {}

	~ScopedJSString()
	{
		if (str_) {
			JS_FreeCString(ctx_, str_);
		}
	}

	const char *get() const { return str_ ? str_ : ""; }

	operator const char *() const { return get(); }

private:
	JSContext *ctx_;
	const char *str_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
