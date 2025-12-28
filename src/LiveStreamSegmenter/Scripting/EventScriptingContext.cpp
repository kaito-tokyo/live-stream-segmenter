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

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

EventScriptingContext::EventScriptingContext(const ScriptingRuntime &scriptingRuntime, const std::filesystem::path &)
	: scriptingRuntime_(scriptingRuntime),
	  logger_(scriptingRuntime_.logger_),
	  rt_(scriptingRuntime_.rt_),
	  ctx_(std::shared_ptr<JSContext>(JS_NewContextRaw(rt_.get()), JS_FreeContext))
{
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
}

EventScriptingContext::~EventScriptingContext() = default;

void EventScriptingContext::loadEventHandler(const char *script)
{
	ScopedJSValue moduleObj(ctx_.get(), JS_Eval(ctx_.get(), script, strlen(script), "<input>",
						    JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY));

	if (JS_IsException(moduleObj.get())) {
		logger_->error("Failed to compile JavaScript module.");
		ScopedJSValue exception(ctx_.get(), JS_GetException(ctx_.get()));
		ScopedJSString str(ctx_.get(), JS_ToCString(ctx_.get(), exception.get()));
		logger_->error("Pending job exception: {}", str.get());
		return;
	} else if (!JS_IsModule(moduleObj.get())) {
		throw std::runtime_error("InvalidModuleError(EventScriptingContext::loadEventHandler)");
	}

	ScopedJSValue ret(ctx_.get(), JS_EvalFunction(ctx_.get(), JS_DupValue(ctx_.get(), moduleObj.get())));
	if (JS_IsException(ret.get())) {
		logger_->error("Failed to execute JavaScript module.");
		ScopedJSValue exception(ctx_.get(), JS_GetException(ctx_.get()));
		ScopedJSString str(ctx_.get(), JS_ToCString(ctx_.get(), exception.get()));
		logger_->error("Pending job exception: {}", str.get());
		return;
	}

	JSModuleDef *m = static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(moduleObj.get()));

	(void)m;
	ScopedJSValue evalRes(ctx_.get(), JS_EvalFunction(ctx_.get(), JS_DupValue(ctx_.get(), moduleObj.get())));

	if (JS_IsException(evalRes.get())) {
		logger_->error("Failed to execute JavaScript function.");
		ScopedJSValue exception(ctx_.get(), JS_GetException(ctx_.get()));
		ScopedJSString str(ctx_.get(), JS_ToCString(ctx_.get(), exception.get()));
		logger_->error("Pending job exception: {}", str.get());
		return;
	}

	ScopedJSValue ns(ctx_.get(), JS_GetModuleNamespace(ctx_.get(), m));
	eventHandlerNs_ = std::move(ns);
}

ScopedJSValue EventScriptingContext::getDefaultValue() const
{
	ScopedJSValue val(ctx_.get(), JS_GetPropertyStr(ctx_.get(), eventHandlerNs_.get(), "default"));
	return val;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
