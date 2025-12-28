/*
 * Live Stream Segmenter - Scripting Module Tests
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

#include <gtest/gtest.h>

#include <iostream>
#include <string_view>

#include <EventScriptingContext.hpp>

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveStreamSegmenter;

class PrintLogger : public Logger::ILogger {
public:
	void log(LogLevel, std::string_view message) const noexcept override
	{
		std::cout << message << std::endl;
	}

	const char *getPrefix() const noexcept override { return ""; }
};

TEST(EventScriptingContextTest, EvalValidScript)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	Scripting::ScriptingRuntime scriptingRuntime(logger);
	Scripting::EventScriptingContext context(scriptingRuntime, "test.sqlite3");
	std::shared_ptr<JSContext> ctx = context.ctx_;
	context.loadEventHandler(R"(export default "42";)");
	Scripting::ScopedJSValue value = context.getDefaultValue();
	Scripting::ScopedJSString str(ctx.get(), JS_ToCString(ctx.get(), value.get()));
	logger->info("Default value: {}", str.get());
}

TEST(EventScriptingContextTest, Ini)
{
	// std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	// Scripting::EventScriptEngine engine("test.sqlite3", logger);
	// engine.eval(R"(
	// 	import { parse } from "builtin:ini";
	// 	export const result = JSON.stringify(parse(`[section]
	// 	key=value`));
	// )");
}

TEST(EventScriptingContextTest, Sqlite)
{
	// std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	// Scripting::EventScriptEngine engine("test.sqlite3", logger);
	// engine.eval(R"(
	// 	db.execute(`CREATE TABLE IF NOT EXISTS test_table (
	// 		id INTEGER PRIMARY KEY AUTOINCREMENT,
	// 		name TEXT NOT NULL
	// 	);`);
	// )");
}
