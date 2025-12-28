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
#include <filesystem>
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

struct TemporaryFile {
	std::filesystem::path tempDir;
	std::filesystem::path path;

	TemporaryFile(std::string name)
	{
		tempDir = std::filesystem::temp_directory_path() / "live-stream-segmenter-test";
		std::filesystem::create_directories(tempDir);
		path = tempDir / name;
	}

	~TemporaryFile()
	{
		std::filesystem::remove(path);
		std::filesystem::remove_all(path.parent_path());
	}
};

TEST(EventScriptingContextTest, ReturnString)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	context.setupContext();
	context.loadEventHandler(R"(export default "42";)");
	Scripting::ScopedJSValue value = context.getModuleProperty("default");
	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "42");
}

TEST(EventScriptingContextTest, ReturnInt64)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	context.setupContext();
	context.loadEventHandler(R"(export default 42;)");
	Scripting::ScopedJSValue value = context.getModuleProperty("default");
	ASSERT_TRUE(value.asInt64().has_value());
	ASSERT_EQ(value.asInt64(), 42);
}

TEST(EventScriptingContextTest, Ini)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	context.setupContext();
	context.loadEventHandler(R"(
		import { parse } from "builtin:ini";
		const iniString = "[section]\nkey=value";
		export default JSON.stringify(parse(iniString));
	)");
	Scripting::ScopedJSValue value = context.getModuleProperty("default");
	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), R"({"section":{"key":"value"}})");
}

TEST(EventScriptingContextTest, Dayjs)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	context.setupContext();
	context.loadEventHandler(R"(
		import { dayjs } from "builtin:dayjs";
		const date = dayjs("2025-01-01");
		export default date.format("YYYY-MM-DD");
	)");
	Scripting::ScopedJSValue value = context.getModuleProperty("default");
	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "2025-01-01");
}

TEST(EventScriptingContextTest, DbToString)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	TemporaryFile tempFile("test.sqlite3");
	Scripting::ScriptingDatabase db(runtime, ctx, logger, tempFile.path);
	context.setupContext();
	db.setupContext();
	context.loadEventHandler(R"(export default db.toString();)");
	Scripting::ScopedJSValue value = context.getModuleProperty("default");
	auto extractedValue = value.asString();
	ASSERT_TRUE(extractedValue.has_value());
	ASSERT_EQ(extractedValue.value(), "[object ScriptingDatabase]");
}

TEST(EventScriptingContextTest, DbQuerySimple)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	TemporaryFile tempFile("test.sqlite3");
	Scripting::ScriptingDatabase db(runtime, ctx, logger, tempFile.path);
	context.setupContext();
	db.setupContext();
	context.loadEventHandler(R"(export default JSON.stringify(db.query("SELECT 1"));)");
	Scripting::ScopedJSValue value = context.getModuleProperty("default");
	auto extractedValue = value.asString();
	ASSERT_TRUE(extractedValue.has_value());
	ASSERT_EQ(extractedValue.value(), "[{\"1\":1}]");
}
