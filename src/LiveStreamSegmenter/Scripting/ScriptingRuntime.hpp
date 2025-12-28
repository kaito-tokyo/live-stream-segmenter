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
#include <optional>
#include <string>
#include <typeindex>

#include <quickjs.h>

#include <ILogger.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

class ScopedJSString {
public:
	ScopedJSString(std::shared_ptr<JSContext> ctx, const char *str) noexcept : ctx_(std::move(ctx)), str_(str) {}

	~ScopedJSString()
	{
		if (str_) {
			JS_FreeCString(ctx_.get(), str_);
		}
	}

	const char *get() const noexcept { return str_ ? str_ : ""; }

private:
	std::shared_ptr<JSContext> ctx_;
	const char *str_;
};

class ScopedJSValue {
public:
	ScopedJSValue() noexcept : ctx_(nullptr), v_(JS_UNDEFINED) {}
	ScopedJSValue(std::shared_ptr<JSContext> ctx, JSValue v) noexcept : ctx_(std::move(ctx)), v_(v) {}

	~ScopedJSValue()
	{
		if (!JS_IsUndefined(v_)) {
			JS_FreeValue(ctx_.get(), v_);
		}
	}

	ScopedJSValue(ScopedJSValue &&other) noexcept : ctx_(std::move(other.ctx_)), v_(other.v_)
	{
		other.v_ = JS_UNDEFINED;
	}
	ScopedJSValue &operator=(ScopedJSValue &&other) noexcept
	{
		if (this != &other) {
			if (!JS_IsUndefined(v_)) {
				JS_FreeValue(ctx_.get(), v_);
			}
			ctx_ = other.ctx_;
			v_ = other.v_;
			other.v_ = JS_UNDEFINED;
		}
		return *this;
	}

	JSValue get() const { return v_; }

	std::optional<std::string> asString() const
	{
		if (JS_IsString(v_)) {
			ScopedJSString cstr(ctx_, JS_ToCString(ctx_.get(), v_));
			return std::string(cstr.get());
		} else {
			return std::nullopt;
		}
	}

	std::optional<std::int64_t> asInt64() const
	{
		if (JS_IsNumber(v_)) {
			std::int64_t val;
			if (JS_ToInt64(ctx_.get(), &val, v_) == 0) {
				return val;
			}
		}

		return std::nullopt;
	}

	std::optional<std::string> asExceptionString() const
	{
		if (JS_IsException(v_)) {
			ScopedJSValue exception(ctx_, JS_GetException(ctx_.get()));
			ScopedJSString str(ctx_, JS_ToCString(ctx_.get(), exception.get()));
			return std::string(str.get());
		} else {
			return std::nullopt;
		}
	}

	ScopedJSValue(const ScopedJSValue &) = delete;
	ScopedJSValue &operator=(const ScopedJSValue &) = delete;

private:
	std::shared_ptr<JSContext> ctx_;
	JSValue v_;
};

class ScriptingRuntime {
public:
	ScriptingRuntime(std::shared_ptr<const Logger::ILogger> logger);
	~ScriptingRuntime();

	ScriptingRuntime(const ScriptingRuntime &) = delete;
	ScriptingRuntime &operator=(const ScriptingRuntime &) = delete;
	ScriptingRuntime(ScriptingRuntime &&) = delete;
	ScriptingRuntime &operator=(ScriptingRuntime &&) = delete;

	template <typename T>
	void registerCustomClass(const JSClassDef* classDef) {
		JSClassID classId = 0;
		JS_NewClassID(rt_.get(), &classId);

		if (JS_NewClass(rt_.get(), classId, classDef) < 0) {
			throw std::runtime_error("RegistrationError(ScriptingRuntime::registerCustomClass)");
		}

		registeredClasses_[std::type_index(typeid(T))] = classId;
	}

	const std::shared_ptr<const Logger::ILogger> logger_;
	const std::shared_ptr<JSRuntime> rt_;

	std::unordered_map<std::type_index, JSClassID> registeredClasses_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
