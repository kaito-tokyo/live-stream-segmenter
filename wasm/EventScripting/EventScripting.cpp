#include <emscripten/bind.h>

#include <memory>
#include <optional>

#include <EventScriptingContext.hpp>
#include <ILogger.hpp>
#include <NullLogger.hpp>
#include <ScriptingDatabase.hpp>
#include <ScriptingRuntime.hpp>

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveStreamSegmenter;

std::string evaluateFunction(std::string script, std::string functionName, std::string eventObject) {
	std::shared_ptr<const Logger::ILogger> logger = std::make_shared<Logger::NullLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	Scripting::ScriptingDatabase database(runtime, ctx, logger, "database.sqlite3", true);
	context.setupContext();
	database.setupContext();
	context.setupLocalStorage();
	context.loadEventHandler(script.c_str());
	return context.executeFunction(functionName.c_str(), eventObject.c_str());
}

EMSCRIPTEN_BINDINGS(my_module) {
	emscripten::function("evaluateFunction", &evaluateFunction);
}
