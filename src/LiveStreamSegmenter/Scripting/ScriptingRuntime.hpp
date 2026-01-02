/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * Live Stream Segmenter - Scripting Module
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
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <quickjs.h>

#include <KaitoTokyo/Logger/ILogger.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

class ScopedJSString {
public:
	ScopedJSString(JSContext *ctx, const char *str) noexcept : ctx_(ctx), str_(str) {}

	~ScopedJSString()
	{
		if (str_) {
			JS_FreeCString(ctx_, str_);
		}
	}

	ScopedJSString(const ScopedJSString &) = delete;
	ScopedJSString &operator=(const ScopedJSString &) = delete;

	ScopedJSString(ScopedJSString &&other) noexcept : ctx_(other.ctx_), str_(other.str_) { other.str_ = nullptr; }
	ScopedJSString &operator=(ScopedJSString &&other) noexcept
	{
		if (this != &other) {
			if (str_) {
				JS_FreeCString(ctx_, str_);
			}
			ctx_ = other.ctx_;
			str_ = other.str_;
			other.str_ = nullptr;
		}
		return *this;
	};

	const char *get() const noexcept { return str_; }

	operator bool() const noexcept { return str_ != nullptr; }

private:
	JSContext *ctx_;
	const char *str_;
};

class ScopedJSValue {
public:
	ScopedJSValue() noexcept : ctx_(nullptr), v_(JS_UNDEFINED) {}
	ScopedJSValue(JSContext *ctx, JSValue v) noexcept : ctx_(ctx), v_(v) {}

	~ScopedJSValue()
	{
		if (!JS_IsUndefined(v_)) {
			JS_FreeValue(ctx_, v_);
		}
	}

	ScopedJSValue(const ScopedJSValue &) = delete;
	ScopedJSValue &operator=(const ScopedJSValue &) = delete;

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

	JSValue get() const { return v_; }

	std::optional<std::string> asString() const
	{
		if (JS_IsString(v_)) {
			ScopedJSString cstr(ctx_, JS_ToCString(ctx_, v_));
			return std::string(cstr.get());
		} else {
			return std::nullopt;
		}
	}

	std::optional<std::int64_t> asInt64() const
	{
		if (JS_IsNumber(v_)) {
			std::int64_t val;
			if (JS_ToInt64(ctx_, &val, v_) == 0) {
				return val;
			}
		}

		return std::nullopt;
	}

	std::optional<std::string> asExceptionString() const
	{
		if (JS_IsException(v_)) {
			ScopedJSValue exception(ctx_, JS_GetException(ctx_));
			ScopedJSString str(ctx_, JS_ToCString(ctx_, exception.get()));
			return std::string(str.get());
		} else {
			return std::nullopt;
		}
	}

private:
	JSContext *ctx_;
	JSValue v_;
};

class ScriptingRuntime {
public:
	ScriptingRuntime();
	~ScriptingRuntime();

	ScriptingRuntime(const ScriptingRuntime &) = delete;
	ScriptingRuntime &operator=(const ScriptingRuntime &) = delete;
	ScriptingRuntime(ScriptingRuntime &&) = delete;
	ScriptingRuntime &operator=(ScriptingRuntime &&) = delete;

	void setLogger(std::shared_ptr<const Logger::ILogger> logger) { logger_ = std::move(logger); }

	std::shared_ptr<JSContext> createContextRaw() const;

	template<typename T> JSClassID registerCustomClass(const JSClassDef *classDef)
	{
		auto it = registeredClasses_.find(std::type_index(typeid(T)));
		if (it != registeredClasses_.end()) {
			return it->second;
		}

		JSClassID classId = 0;
		JS_NewClassID(rt_.get(), &classId);

		if (JS_NewClass(rt_.get(), classId, classDef) < 0) {
			throw std::runtime_error("RegistrationError(ScriptingRuntime::registerCustomClass)");
		}

		registeredClasses_[std::type_index(typeid(T))] = classId;

		return classId;
	}

	template<typename T> JSClassID getClassId() const
	{
		auto it = registeredClasses_.find(std::type_index(typeid(T)));
		if (it == registeredClasses_.end()) {
			throw std::runtime_error("ClassNotRegisteredError(ScriptingRuntime::getClassId)");
		}
		return it->second;
	}

	const std::shared_ptr<JSRuntime> rt_;

	std::shared_ptr<const Logger::ILogger> logger_;
	mutable std::unordered_map<std::type_index, JSClassID> registeredClasses_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
