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

#include "ScriptingDatabase.hpp"

#include <iterator>

#include <sqlite3.h>
#include <fmt/format.h>

#include "ScopedQuickJs.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

namespace {

sqlite3 *openSqlite3(const std::filesystem::path &dbPath)
{
	sqlite3 *db = nullptr;
	std::u8string dbPathU8 = dbPath.u8string();
	const char *dbPathCStr = reinterpret_cast<const char *>(dbPathU8.c_str());
	if (sqlite3_open_v2(dbPathCStr, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
		throw std::runtime_error(
			fmt::format("InitError(ScriptingDatabase::ScriptingDatabase):{}", sqlite3_errmsg(db)));
	}
	return db;
}

const static JSClassDef kClassDef = {
	.class_name = "ScriptingDatabase",
};

const static JSCFunctionListEntry kClassFuncList[] = {
	JS_CFUNC_DEF("execute", 1, ScriptingDatabase::execute),
	JS_CFUNC_DEF("query", 1, ScriptingDatabase::query),
};

} // anonymous namespace

ScriptingDatabase::ScriptingDatabase(const std::filesystem::path &dbPath)
{
	db_.reset(openSqlite3(dbPath));
}

void ScriptingDatabase::addIntrinsicsDb(std::shared_ptr<JSContext> ctx)
{
	JSRuntime *rt = JS_GetRuntime(ctx.get());

	if (classId_) {
		throw std::runtime_error("DoubleAddError(ScriptingDatabase::addIntrinsicsDb)");
	}

	JS_NewClassID(rt, &classId_);
	JS_NewClass(rt, classId_, &kClassDef);

	ScopedJSValue dbObj(ctx.get(), JS_NewObjectClass(ctx.get(), classId_));
	JS_SetOpaque(dbObj.get(), this);

	JS_SetPropertyFunctionList(ctx.get(), dbObj.get(), kClassFuncList, std::size(kClassFuncList));

	ScopedJSValue globalObj(ctx.get(), JS_GetGlobalObject(ctx.get()));
	JS_SetPropertyStr(ctx.get(), globalObj.get(), "db", dbObj.get());
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
