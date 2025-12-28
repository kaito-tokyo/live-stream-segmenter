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

#include <ILogger.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

class ScopedJSValue {
public:
	ScopedJSValue() noexcept : ctx_(nullptr), v_(JS_UNDEFINED) {}
	ScopedJSValue(JSContext *ctx, JSValue v) : ctx_(ctx), v_(v) {}

	ScopedJSValue(ScopedJSValue &&other) noexcept : ctx_(other.ctx_), v_(other.v_) { other.v_ = JS_UNDEFINED; }
	ScopedJSValue &operator=(ScopedJSValue &&other) noexcept
	{
		if (this != &other) {
			if (!JS_IsUndefined(v_)) {
				JS_FreeValue(ctx_, v_);
			}
			ctx_ = other.ctx_;
			v_ = other.v_;
			other.v_ = JS_UNDEFINED;
		}
		return *this;
	}

	~ScopedJSValue()
	{
		if (!JS_IsUndefined(v_)) {
			JS_FreeValue(ctx_, v_);
		}
	}

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

private:
	JSContext *ctx_;
	const char *str_;
};

class ScriptingRuntime {
public:
	ScriptingRuntime(std::shared_ptr<const Logger::ILogger> logger);
	~ScriptingRuntime();

	const std::shared_ptr<const Logger::ILogger> logger_;
	const std::shared_ptr<JSRuntime> rt_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
