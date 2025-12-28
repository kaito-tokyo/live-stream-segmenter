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

#include "ScriptingRuntime.hpp"

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

struct SqliteStmtDeleter {
	void operator()(sqlite3_stmt *stmt) const { sqlite3_finalize(stmt); }
};

using ScopedStmt = std::unique_ptr<sqlite3_stmt, SqliteStmtDeleter>;

const static JSClassDef kClassDef = {
	.class_name = "ScriptingDatabase",
};

const static JSCFunctionListEntry kClassFuncList[] = {
	JS_CFUNC_DEF("execute", 1, ScriptingDatabase::execute),
	JS_CFUNC_DEF("query", 1, ScriptingDatabase::query),
	JS_CFUNC_DEF("toString", 0, ScriptingDatabase::toString),
};

ScriptingDatabase *unwrap(JSContext *ctx, JSValueConst this_val)
{
	JSRuntime *rt = JS_GetRuntime(ctx);
	auto runtime = static_cast<ScriptingRuntime *>(JS_GetRuntimeOpaque(rt));
	JSClassID classId = runtime->getClassId<ScriptingDatabase>();
	return reinterpret_cast<ScriptingDatabase *>(JS_GetOpaque(this_val, classId));
}

void bindArgs(JSContext *ctx, sqlite3_stmt *stmt, int argc, JSValueConst *argv)
{
	// SQLite binds are 1-based.
	// JS args: argv[1] corresponds to ?1, argv[2] to ?2...
	for (int i = 1; i < argc; i++) {
		int bindIdx = i;
		int tag = JS_VALUE_GET_TAG(argv[i]);

		if (tag == JS_TAG_INT) {
			int64_t val;
			JS_ToInt64(ctx, &val, argv[i]);
			sqlite3_bind_int64(stmt, bindIdx, val);
		} else if (tag == JS_TAG_FLOAT64) {
			double val;
			JS_ToFloat64(ctx, &val, argv[i]);
			sqlite3_bind_double(stmt, bindIdx, val);
		} else if (tag == JS_TAG_STRING) {
			ScopedJSString str(ctx, JS_ToCString(ctx, argv[i]));
			sqlite3_bind_text(stmt, bindIdx, str.get(), -1, SQLITE_TRANSIENT);
		} else if (tag == JS_TAG_BOOL) {
			sqlite3_bind_int(stmt, bindIdx, JS_ToBool(ctx, argv[i]));
		} else if (JS_IsNull(argv[i]) || JS_IsUndefined(argv[i])) {
			sqlite3_bind_null(stmt, bindIdx);
		} else {
			size_t len;
			uint8_t *buf = JS_GetArrayBuffer(ctx, &len, argv[i]);

			if (buf) {
				sqlite3_bind_blob(stmt, bindIdx, buf, len, SQLITE_TRANSIENT);
			} else {
				sqlite3_bind_null(stmt, bindIdx);
			}
		}
	}
}

} // anonymous namespace

ScriptingDatabase::ScriptingDatabase(std::shared_ptr<ScriptingRuntime> runtime, std::shared_ptr<JSContext> ctx,
				     std::shared_ptr<const Logger::ILogger> logger, const std::filesystem::path &dbPath)
	: runtime_(std::move(runtime)),
	  ctx_(std::move(ctx)),
	  logger_(std::move(logger)),
	  db_(openSqlite3(dbPath), sqlite3_close_v2)
{
	if (!runtime_) {
		throw std::runtime_error("ScriptingRuntimeIsNullError(ScriptingDatabase::ScriptingDatabase)");
	}

	if (!ctx_) {
		throw std::runtime_error("ContextIsNullError(ScriptingDatabase::ScriptingDatabase)");
	}

	if (!db_) {
		throw std::runtime_error("DBIsNullError(ScriptingDatabase::ScriptingDatabase)");
	}
}

ScriptingDatabase::~ScriptingDatabase() = default;

void ScriptingDatabase::setupContext()
{
	JSClassID classId = runtime_->registerCustomClass<ScriptingDatabase>(&kClassDef);

	JSValue dbObj = JS_NewObjectClass(ctx_.get(), classId);
	JS_SetOpaque(dbObj, this);

	JS_SetPropertyFunctionList(ctx_.get(), dbObj, kClassFuncList, std::size(kClassFuncList));

	JSValue globalObj = JS_GetGlobalObject(ctx_.get());
	JS_SetPropertyStr(ctx_.get(), globalObj, "db", dbObj);
	JS_FreeValue(ctx_.get(), globalObj);
}

JSValue ScriptingDatabase::query(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	if (argc < 1)
		return JS_ThrowTypeError(ctx, "SQL string required");

	ScriptingDatabase *self = unwrap(ctx, this_val);
	if (!self)
		return JS_ThrowInternalError(ctx, "Invalid database object");

	ScopedJSString sqlStr(ctx, JS_ToCString(ctx, argv[0]));
	if (!sqlStr.get())
		return JS_EXCEPTION;

	sqlite3_stmt *stmtRaw = nullptr;
	int rc = sqlite3_prepare_v3(self->db_.get(), sqlStr.get(), -1, 0, &stmtRaw, nullptr);

	if (rc != SQLITE_OK) {
		return JS_ThrowInternalError(ctx, "SQL Error: %s", sqlite3_errmsg(self->db_.get()));
	}

	ScopedStmt stmt(stmtRaw);

	bindArgs(ctx, stmt.get(), argc, argv);

	JSValue resultArr = JS_NewArray(ctx);
	int rowIdx = 0;

	while ((rc = sqlite3_step(stmt.get())) == SQLITE_ROW) {
		JSValue rowObj = JS_NewObject(ctx);
		int colCount = sqlite3_column_count(stmt.get());

		for (int i = 0; i < colCount; i++) {
			const char *colName = sqlite3_column_name(stmt.get(), i);
			int colType = sqlite3_column_type(stmt.get(), i);
			JSValue val;

			switch (colType) {
			case SQLITE_INTEGER:
				val = JS_NewInt64(ctx, sqlite3_column_int64(stmt.get(), i));
				break;
			case SQLITE_FLOAT:
				val = JS_NewFloat64(ctx, sqlite3_column_double(stmt.get(), i));
				break;
			case SQLITE_TEXT:
				val = JS_NewString(ctx,
						   reinterpret_cast<const char *>(sqlite3_column_text(stmt.get(), i)));
				break;
			case SQLITE_NULL:
				val = JS_NULL;
				break;
			default:
				val = JS_NULL;
				break;
			}

			JS_SetPropertyStr(ctx, rowObj, colName, val);
		}
		JS_SetPropertyUint32(ctx, resultArr, rowIdx++, rowObj);
	}

	if (rc != SQLITE_DONE) {
		JS_FreeValue(ctx, resultArr);
		return JS_ThrowInternalError(ctx, "Execution Error: %s", sqlite3_errmsg(self->db_.get()));
	}

	return resultArr;
}

JSValue ScriptingDatabase::execute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
	if (argc < 1)
		return JS_ThrowTypeError(ctx, "SQL string required");

	ScriptingDatabase *self = unwrap(ctx, this_val);
	if (!self)
		return JS_ThrowInternalError(ctx, "Invalid database object");

	ScopedJSString sqlStr(ctx, JS_ToCString(ctx, argv[0]));
	if (!sqlStr.get())
		return JS_EXCEPTION;

	sqlite3_stmt *stmtRaw = nullptr;
	int rc = sqlite3_prepare_v3(self->db_.get(), sqlStr.get(), -1, 0, &stmtRaw, nullptr);

	if (rc != SQLITE_OK) {
		return JS_ThrowInternalError(ctx, "SQL Error: %s", sqlite3_errmsg(self->db_.get()));
	}

	ScopedStmt stmt(stmtRaw);

	bindArgs(ctx, stmt.get(), argc, argv);

	rc = sqlite3_step(stmt.get());

	if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
		return JS_ThrowInternalError(ctx, "Execute Error: %s", sqlite3_errmsg(self->db_.get()));
	}

	// Return an object with changes info
	JSValue resultObj = JS_NewObject(ctx);
	JS_SetPropertyStr(ctx, resultObj, "changes", JS_NewInt64(ctx, sqlite3_changes(self->db_.get())));
	JS_SetPropertyStr(ctx, resultObj, "lastInsertId", JS_NewInt64(ctx, sqlite3_last_insert_rowid(self->db_.get())));

	return resultObj;
}

JSValue ScriptingDatabase::toString(JSContext *ctx, JSValueConst, int, JSValueConst *)
{
	return JS_NewString(ctx, "[object ScriptingDatabase]");
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
