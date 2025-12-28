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

#include "EventScriptingContext.hpp"

extern "C" const uint32_t qjsc_dayjs_bundle_size;
extern "C" const uint8_t qjsc_dayjs_bundle[];
extern "C" const uint32_t qjsc_ini_bundle_size;
extern "C" const uint8_t qjsc_ini_bundle[];

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

EventScriptingContext::EventScriptingContext(std::shared_ptr<ScriptingRuntime> runtime,
					     std::shared_ptr<Logger::ILogger> logger,
					     const std::filesystem::path &dbPath)
	: runtime_(std::move(runtime)),
	  logger_(std::move(logger)),
	  ctx_(std::shared_ptr<JSContext>(JS_NewContextRaw(runtime_->rt_.get()), JS_FreeContext))
{
	static_cast<void>(dbPath);
	if (!ctx_) {
		throw std::runtime_error("InitRuntimeError(ScriptingRuntime::ScriptingRuntime)");
	}

	JS_AddIntrinsicBaseObjects(ctx_.get());
	JS_AddIntrinsicJSON(ctx_.get());
	JS_AddIntrinsicDate(ctx_.get());
	JS_AddIntrinsicRegExpCompiler(ctx_.get());
	JS_AddIntrinsicRegExp(ctx_.get());
	JS_AddIntrinsicEval(ctx_.get());
	JS_AddIntrinsicPromise(ctx_.get());

	loadModule(qjsc_dayjs_bundle_size, qjsc_dayjs_bundle);
	loadModule(qjsc_ini_bundle_size, qjsc_ini_bundle);

	database_ = std::make_shared<ScriptingDatabase>(runtime_, ctx_, dbPath);
}

EventScriptingContext::~EventScriptingContext() = default;

void EventScriptingContext::loadEventHandler(const char *script)
{
	ScopedJSValue moduleObj(ctx_, JS_Eval(ctx_.get(), script, strlen(script), "<input>",
					      JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY));

	if (JS_IsException(moduleObj.get())) {
		logger_->error("Failed to compile JavaScript module.");
		ScopedJSValue exception(ctx_, JS_GetException(ctx_.get()));
		ScopedJSString str(ctx_, JS_ToCString(ctx_.get(), exception.get()));
		logger_->error("Pending job exception: {}", str.get());
		return;
	} else if (!JS_IsModule(moduleObj.get())) {
		throw std::runtime_error("InvalidModuleError(EventScriptingContext::loadEventHandler)");
	}

	ScopedJSValue ret(ctx_, JS_EvalFunction(ctx_.get(), JS_DupValue(ctx_.get(), moduleObj.get())));
	if (JS_IsException(ret.get())) {
		logger_->error("Failed to execute JavaScript module.");
		ScopedJSValue exception(ctx_, JS_GetException(ctx_.get()));
		ScopedJSString str(ctx_, JS_ToCString(ctx_.get(), exception.get()));
		logger_->error("Pending job exception: {}", str.get());
		return;
	}

	JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(moduleObj.get()));

	ScopedJSValue evalRes(ctx_, JS_EvalFunction(ctx_.get(), JS_DupValue(ctx_.get(), moduleObj.get())));

	if (JS_IsException(evalRes.get())) {
		logger_->error("Failed to execute JavaScript function.");
		ScopedJSValue exception(ctx_, JS_GetException(ctx_.get()));
		ScopedJSString str(ctx_, JS_ToCString(ctx_.get(), exception.get()));
		logger_->error("Pending job exception: {}", str.get());
		return;
	}

	ScopedJSValue ns(ctx_, JS_GetModuleNamespace(ctx_.get(), m));
	eventHandlerNs_ = std::move(ns);
}

ScopedJSValue EventScriptingContext::getModuleProperty(const char *property) const
{
	ScopedJSValue val(ctx_, JS_GetPropertyStr(ctx_.get(), eventHandlerNs_.get(), property));
	return val;
}

void EventScriptingContext::loadModule(std::uint32_t size, const std::uint8_t *buf)
{
	ScopedJSValue moduleObj(ctx_, JS_ReadObject(ctx_.get(), buf, size, JS_READ_OBJ_BYTECODE));

	if (std::optional<std::string> exceptionString = moduleObj.asExceptionString()) {
		logger_->error("name=ReadObjectError\tlocation=EventScriptingContext::loadModule\tmessage={}",
			       exceptionString.value());
		throw std::runtime_error("ReadObjectError(EventScriptingContext::loadModule)");
	}

	ScopedJSValue res(ctx_, JS_EvalFunction(ctx_.get(), JS_DupValue(ctx_.get(), moduleObj.get())));

	if (std::optional<std::string> exceptionString = res.asExceptionString()) {
		logger_->error("name=EvalError\tlocation=EventScriptingContext::loadModule\tmessage={}",
			       exceptionString.value());
		throw std::runtime_error("EvalError(EventScriptingContext::loadModule)");
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
