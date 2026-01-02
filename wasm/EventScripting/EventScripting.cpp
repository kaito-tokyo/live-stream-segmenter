#include <emscripten/bind.h>

#include <memory>
#include <optional>

#include <KaitoTokyo/Logger/ILogger.hpp>
#include <KaitoTokyo/Logger/NullLogger.hpp>

#include <EventScriptingContext.hpp>
#include <ScriptingDatabase.hpp>
#include <ScriptingRuntime.hpp>

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveStreamSegmenter;

std::string evaluateFunction(const std::string& script, const std::string& functionName, const std::string& eventObject) {
	std::shared_ptr<const Logger::ILogger> logger = std::make_shared<Logger::NullLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>();
	runtime->setLogger(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	Scripting::ScriptingDatabase database(runtime, ctx, logger, "database.sqlite3", true);
	context.setupContext();
	database.setupContext();
	context.setupLocalStorage();
	context.loadEventHandler(script.c_str());
	return context.executeFunction(functionName.c_str(), eventObject.c_str());
}

EMSCRIPTEN_BINDINGS(event_scripting_module) {
	emscripten::function("evaluateFunction", &evaluateFunction);
}
