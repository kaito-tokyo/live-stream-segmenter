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

#include "EventScriptEngine.hpp"

#include <quickjs.h>

extern "C" const uint32_t qjsc_dayjs_bundle_size;
extern "C" const uint8_t qjsc_dayjs_bundle[];
extern "C" const uint32_t qjsc_ini_bundle_size;
extern "C" const uint8_t qjsc_ini_bundle[];

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

EventScriptEngine::EventScriptEngine(const std::shared_ptr<const Logger::ILogger> &logger) : logger_(logger) {}

EventScriptEngine::~EventScriptEngine() = default;

namespace {

struct JSRuntimeDeleter {
	void operator()(JSRuntime *rt) { JS_FreeRuntime(rt); }
};
using JSRuntimePtr = std::unique_ptr<JSRuntime, JSRuntimeDeleter>;

struct JSContextDeleter {
	void operator()(JSContext *ctx) { JS_FreeContext(ctx); }
};
using JSContextPtr = std::unique_ptr<JSContext, JSContextDeleter>;

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

} // anonymous namespace

void EventScriptEngine::eval(const char *script)
{
	JSRuntimePtr rt(JS_NewRuntime());
	if (!rt) {
		logger_->error("Failed to create QuickJS runtime.");
		return;
	}

	JSContextPtr ctx(JS_NewContextRaw(rt.get()));
	if (!ctx) {
		logger_->error("Failed to create QuickJS context.");
		return;
	}

	JS_AddIntrinsicBaseObjects(ctx.get());
	JS_AddIntrinsicJSON(ctx.get());
	JS_AddIntrinsicDate(ctx.get());
	JS_AddIntrinsicRegExpCompiler(ctx.get());
	JS_AddIntrinsicRegExp(ctx.get());
	JS_AddIntrinsicEval(ctx.get());
	JS_AddIntrinsicPromise(ctx.get());

	{
		ScopedJSValue dayjs_obj(ctx.get(), JS_ReadObject(ctx.get(), qjsc_dayjs_bundle, qjsc_dayjs_bundle_size,
								 JS_READ_OBJ_BYTECODE));

		if (JS_IsException(dayjs_obj.get())) {
			ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
			ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), exception));
			logger_->error("Failed to read dayjs bundle: {}", str.get());
			return;
		}

		ScopedJSValue dayjs_res(ctx.get(), JS_EvalFunction(ctx.get(), JS_DupValue(ctx.get(), dayjs_obj.get())));

		if (JS_IsException(dayjs_res.get())) {
			ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
			ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), exception));
			logger_->error("Failed to evaluate dayjs bundle: {}", str.get());
			return;
		}
	}

	{
		ScopedJSValue ini_obj(ctx.get(), JS_ReadObject(ctx.get(), qjsc_ini_bundle, qjsc_ini_bundle_size,
							       JS_READ_OBJ_BYTECODE));

		if (JS_IsException(ini_obj.get())) {
			ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
			ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), exception));
			logger_->error("Failed to read ini bundle: {}", str.get());
			return;
		}

		ScopedJSValue ini_res(ctx.get(), JS_EvalFunction(ctx.get(), JS_DupValue(ctx.get(), ini_obj.get())));
		if (JS_IsException(ini_res.get())) {
			ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
			ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), exception));
			logger_->error("Failed to evaluate ini bundle: {}", str.get());
			return;
		}
	}

	ScopedJSValue module_obj(ctx.get(), JS_Eval(ctx.get(), script, strlen(script), "<input>",
						    JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY));

	if (JS_IsException(module_obj)) {
		ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
		ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), exception));
		logger_->error("JavaScript exception: {}", str.get());
		return;
	}

	if (JS_ResolveModule(ctx.get(), module_obj) < 0) {
		ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
		ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), exception));
		logger_->error("Module resolution failed: {}", str.get());
		return;
	}

	JSModuleDef *m = (JSModuleDef *)JS_VALUE_GET_PTR(module_obj.get());

	ScopedJSValue eval_res(ctx.get(), JS_EvalFunction(ctx.get(), JS_DupValue(ctx.get(), module_obj)));

	if (JS_IsException(eval_res)) {
		logger_->error("Failed to execute JavaScript function.");
		ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
		ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), exception));
		logger_->error("Pending job exception: {}", str.get());
		return;
	}

	JSContext *pctx;
	int err;
	while ((err = JS_ExecutePendingJob(rt.get(), &pctx)) != 0) {
		if (err < 0) {
			ScopedJSValue exception(pctx, JS_GetException(pctx));
			ScopedJSString str(pctx, JS_ToCString(pctx, exception));
			logger_->error("Pending job exception: {}", str.get());
		}
	}

	ScopedJSValue ns(ctx.get(), JS_GetModuleNamespace(ctx.get(), m));

	ScopedJSValue result(ctx.get(), JS_GetPropertyStr(ctx.get(), ns, "result"));

	if (JS_IsFunction(ctx.get(), result.get())) {
		logger_->info("Found function. Executing...");

		// 1. 引数の準備
		// 引数がない場合は argc=0, argv=nullptr で構いません。
		// 引数を渡す場合は JSValue の配列を作成します。
		int argc = 0;
		JSValue *argv = nullptr;

		// 2. 関数の実行 (JS_Call)
		// 第3引数は 'this' です。通常の関数呼び出しなら JS_UNDEFINED でOKです。
		ScopedJSValue result2(ctx.get(), JS_Call(ctx.get(), result.get(), JS_UNDEFINED, argc, argv));

		// 3. 実行時エラーのチェック
		if (JS_IsException(result2)) {
			ScopedJSValue exception(ctx.get(), JS_GetException(ctx.get()));
			ScopedJSString errStr(ctx.get(), JS_ToCString(ctx.get(), exception));
			logger_->error("Error executing function: {}", errStr.get());
			return;
		}

		// 4. ジョブキューの処理 (非同期処理への対応)
		// 関数実行によって新たなPromiseジョブなどが生成された場合に備えてループを回します。
		JSContext *pctx;
		int err;
		while ((err = JS_ExecutePendingJob(rt.get(), &pctx)) != 0) {
			if (err < 0) {
				ScopedJSValue exception(pctx, JS_GetException(pctx));
				ScopedJSString str(pctx, JS_ToCString(pctx, exception));
				logger_->error("Pending job exception during execution: {}", str.get());
			}
		}

		// 5. 戻り値の処理
		// JavaScriptの関数が返した値を利用できます
		ScopedJSString resStr(ctx.get(), JS_ToCString(ctx.get(), result2.get()));
		logger_->info("Function returned: {}", resStr.get());

		// もし戻り値が Promise オブジェクトの場合 (async functionなど)、
		// ここで得られるのは Promise そのものです。
		// 解決された値が必要な場合は、JS側で await して値を返すか、
		// C++側で Promise の .then を呼び出すなどの追加実装が必要です。
	} else {
		logger_->info("result is not a function or not found.");
		ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), result.get()));
		logger_->info("result value: {}", str.get());
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
