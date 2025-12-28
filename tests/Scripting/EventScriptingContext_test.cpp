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

#include <filesystem>
#include <iostream>
#include <random>
#include <string_view>

#include <EventScriptingContext.hpp>

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveStreamSegmenter;

class PrintLogger : public Logger::ILogger {
public:
	void log(LogLevel, std::string_view message) const noexcept override { std::cout << message << std::endl; }
	const char *getPrefix() const noexcept override { return ""; }
};

struct TemporaryFile {
	std::filesystem::path tempDir;
	std::filesystem::path path;

	TemporaryFile(const std::string &name)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dist(0, 999999);
		std::string uniqueSuffix = std::to_string(dist(gen));
		tempDir = std::filesystem::temp_directory_path() / ("live-stream-segmenter-test-" + uniqueSuffix);
		std::filesystem::create_directories(tempDir);
		path = tempDir / name;
	}
	TemporaryFile(const TemporaryFile &) = delete;
	TemporaryFile &operator=(const TemporaryFile &) = delete;
	TemporaryFile(TemporaryFile &&other) noexcept : tempDir(std::move(other.tempDir)), path(std::move(other.path))
	{
		other.tempDir.clear();
		other.path.clear();
	}
	TemporaryFile &operator=(TemporaryFile &&other) noexcept
	{
		if (this != &other) {
			std::error_code ec;
			if (!tempDir.empty())
				std::filesystem::remove_all(tempDir, ec);
			tempDir = std::move(other.tempDir);
			path = std::move(other.path);
			other.tempDir.clear();
			other.path.clear();
		}
		return *this;
	}
	~TemporaryFile()
	{
		if (!tempDir.empty()) {
			std::error_code ec;
			std::filesystem::remove_all(tempDir, ec);
		}
	}
};

class EventScriptingContextTest : public ::testing::Test {
protected:
	std::shared_ptr<Logger::ILogger> logger;
	std::shared_ptr<Scripting::ScriptingRuntime> runtime;
	std::shared_ptr<JSContext> ctx;
	std::unique_ptr<Scripting::EventScriptingContext> context;

	std::unique_ptr<TemporaryFile> tempFile;
	std::unique_ptr<Scripting::ScriptingDatabase> db;

	void SetUp() override
	{
		logger = std::make_shared<PrintLogger>();
		runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
		ctx = runtime->createContextRaw();
		context = std::make_unique<Scripting::EventScriptingContext>(runtime, ctx, logger);
		context->setupContext();
	}

	void useDatabase(const std::string &filename = "test.sqlite3")
	{
		tempFile = std::make_unique<TemporaryFile>(filename);
		db = std::make_unique<Scripting::ScriptingDatabase>(runtime, ctx, logger, tempFile->path);
		db->setupContext();
	}

	void useLocalStorage()
	{
		if (!db) {
			useDatabase("test_localstorage.sqlite3");
		}
		context->setupLocalStorage();
	}

	Scripting::ScopedJSValue eval(const char *script)
	{
		context->loadEventHandler(script);
		return context->getModuleProperty("default");
	}
};

TEST_F(EventScriptingContextTest, ReturnString)
{
	auto value = eval(R"(export default "42";)");
	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "42");
}

TEST_F(EventScriptingContextTest, ReturnInt64)
{
	auto value = eval(R"(export default 42;)");
	ASSERT_TRUE(value.asInt64().has_value());
	ASSERT_EQ(value.asInt64(), 42);
}

TEST_F(EventScriptingContextTest, Ini)
{
	auto value = eval(R"(
		import { parse } from "builtin:ini";
		const iniString = "[section]\nkey=value";
		export default JSON.stringify(parse(iniString));
	)");
	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), R"({"section":{"key":"value"}})");
}

TEST_F(EventScriptingContextTest, Dayjs)
{
	auto value = eval(R"(
		import { dayjs } from "builtin:dayjs";
		const date = dayjs("2025-01-01");
		export default date.format("YYYY-MM-DD");
	)");
	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "2025-01-01");
}

TEST_F(EventScriptingContextTest, DbToString)
{
	useDatabase();
	auto value = eval(R"(export default db.toString();)");

	auto extractedValue = value.asString();
	ASSERT_TRUE(extractedValue.has_value());
	ASSERT_EQ(extractedValue.value(), "[object ScriptingDatabase]");
}

TEST_F(EventScriptingContextTest, DbExecuteInsert)
{
	useDatabase();
	auto value = eval(R"(
		db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);");
		const res1 = db.execute("INSERT INTO users (name) VALUES ('Alice');");
		const res2 = db.execute("INSERT INTO users (name) VALUES ('Bob');");
		export default JSON.stringify([res1, res2]);
	)");

	auto extractedValue = value.asString();
	ASSERT_TRUE(extractedValue.has_value());
	ASSERT_EQ(extractedValue.value(), "[{\"changes\":1,\"lastInsertId\":1},{\"changes\":1,\"lastInsertId\":2}]");
}

TEST_F(EventScriptingContextTest, DbQueryWithParams)
{
	useDatabase();
	auto value = eval(R"(
		db.execute("CREATE TABLE items (name TEXT, price INTEGER);");
		db.execute("INSERT INTO items VALUES (?, ?);", "Apple", 100);
		db.execute("INSERT INTO items VALUES (?, ?);", "Banana", 200);
		db.execute("INSERT INTO items VALUES (?, ?);", "Cherry", 300);

		const result = db.query("SELECT name FROM items WHERE price > ?;", 150);
		export default JSON.stringify(result);
	)");

	auto extractedValue = value.asString();
	ASSERT_TRUE(extractedValue.has_value());
	ASSERT_EQ(extractedValue.value(), "[{\"name\":\"Banana\"},{\"name\":\"Cherry\"}]");
}

TEST_F(EventScriptingContextTest, DbQueryTypes)
{
	useDatabase();
	auto value = eval(R"(
		db.execute("CREATE TABLE types (i INTEGER, f REAL, t TEXT, n TEXT);");
		db.execute("INSERT INTO types VALUES (?, ?, ?, ?);", 42, 3.14, "hello", null);
		export default JSON.stringify(db.query("SELECT * FROM types;")[0]);
	)");

	auto extractedValue = value.asString();
	ASSERT_TRUE(extractedValue.has_value());
	ASSERT_EQ(extractedValue.value(), "{\"i\":42,\"f\":3.14,\"t\":\"hello\",\"n\":null}");
}

TEST_F(EventScriptingContextTest, DbTransaction)
{
	useDatabase();
	auto value = eval(R"(
		db.execute("CREATE TABLE data (val INTEGER);");
		db.execute("BEGIN TRANSACTION;");
		db.execute("INSERT INTO data VALUES (1);");
		db.execute("INSERT INTO data VALUES (2);");
		db.execute("COMMIT;");
		export default JSON.stringify(db.query("SELECT count(*) as c FROM data;"));
	)");

	auto extractedValue = value.asString();
	ASSERT_TRUE(extractedValue.has_value());
	ASSERT_EQ(extractedValue.value(), "[{\"c\":2}]");
}

// -----------------------------------------------------------------------------
// LocalStorage Tests
// -----------------------------------------------------------------------------

TEST_F(EventScriptingContextTest, LocalStorage_BasicSetGet)
{
	useLocalStorage();
	auto value = eval(R"(
		localStorage.setItem("key1", "value1");
		const val = localStorage.getItem("key1");
		export default val;
	)");

	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "value1");
}

TEST_F(EventScriptingContextTest, LocalStorage_Overwrite)
{
	useLocalStorage();
	auto value = eval(R"(
		localStorage.setItem("key1", "initial");
		localStorage.setItem("key1", "updated");
		export default localStorage.getItem("key1");
	)");

	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "updated");
}

TEST_F(EventScriptingContextTest, LocalStorage_RemoveItem)
{
	useLocalStorage();
	auto value = eval(R"(
		localStorage.setItem("todelete", "val");
		localStorage.removeItem("todelete");
		const val = localStorage.getItem("todelete");
		export default val === null ? "IS_NULL" : "NOT_NULL";
	)");

	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "IS_NULL");
}

TEST_F(EventScriptingContextTest, LocalStorage_Length)
{
	useLocalStorage();
	auto value = eval(R"(
		localStorage.clear();
		localStorage.setItem("a", "1");
		localStorage.setItem("b", "2");
		localStorage.setItem("c", "3");
		localStorage.removeItem("b");
		export default localStorage.length;
	)");

	ASSERT_TRUE(value.asInt64().has_value());
	ASSERT_EQ(value.asInt64(), 2);
}

TEST_F(EventScriptingContextTest, LocalStorage_Key)
{
	useLocalStorage();
	auto value = eval(R"(
		localStorage.clear();
		localStorage.setItem("uniqueKey", "val");
		export default localStorage.key(0);
	)");

	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), "uniqueKey");
}

TEST_F(EventScriptingContextTest, LocalStorage_Clear)
{
	useLocalStorage();
	auto value = eval(R"(
		localStorage.setItem("a", "1");
		localStorage.setItem("b", "2");
		localStorage.clear();
		export default localStorage.length;
	)");

	ASSERT_TRUE(value.asInt64().has_value());
	ASSERT_EQ(value.asInt64(), 0);
}

TEST_F(EventScriptingContextTest, LocalStorage_TypeCoercion)
{
	useLocalStorage();
	auto value = eval(R"(
		localStorage.setItem(123, 456);
		const val = localStorage.getItem("123");
		const isString = (typeof val === 'string');
		export default JSON.stringify({ isString, content: val });
	)");

	ASSERT_TRUE(value.asString().has_value());
	ASSERT_EQ(value.asString(), R"({"isString":true,"content":"456"})");
}

TEST(EventScriptingContextTest_NoFixture, LocalStorage_Persistence)
{
	std::shared_ptr<Logger::ILogger> logger = std::make_shared<PrintLogger>();
	TemporaryFile tempFile("test_localstorage_persist.sqlite3");

	auto createEnv = [&](auto &context_out, auto &db_out) {
		auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
		std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
		context_out = std::make_unique<Scripting::EventScriptingContext>(runtime, ctx, logger);
		db_out = std::make_unique<Scripting::ScriptingDatabase>(runtime, ctx, logger, tempFile.path);

		context_out->setupContext();
		db_out->setupContext();
		context_out->setupLocalStorage();
	};

	// First session: Write
	{
		std::unique_ptr<Scripting::EventScriptingContext> context;
		std::unique_ptr<Scripting::ScriptingDatabase> db;
		createEnv(context, db);

		context->loadEventHandler(R"(localStorage.setItem("persistentKey", "persistentValue");)");
	}

	// Second session: Read
	{
		std::unique_ptr<Scripting::EventScriptingContext> context;
		std::unique_ptr<Scripting::ScriptingDatabase> db;
		createEnv(context, db);

		context->loadEventHandler(R"(export default localStorage.getItem("persistentKey");)");

		Scripting::ScopedJSValue value = context->getModuleProperty("default");
		ASSERT_TRUE(value.asString().has_value());
		ASSERT_EQ(value.asString(), "persistentValue");
	}
}
