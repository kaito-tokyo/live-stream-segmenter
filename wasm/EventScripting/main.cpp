#include <iostream>
#include <memory>
#include <optional>

#include <EventScriptingContext.hpp>
#include <ILogger.hpp>
#include <ScriptingDatabase.hpp>
#include <ScriptingRuntime.hpp>

using namespace KaitoTokyo;
using namespace KaitoTokyo::LiveStreamSegmenter;

class PrintLogger : public Logger::ILogger {
public:
	void log(LogLevel, std::string_view message) const noexcept override { std::cout << message << std::endl; }
	const char *getPrefix() const noexcept override { return ""; }
};

int main() {
	std::shared_ptr<const Logger::ILogger> logger = std::make_shared<PrintLogger>();
	auto runtime = std::make_shared<Scripting::ScriptingRuntime>(logger);
	std::shared_ptr<JSContext> ctx = runtime->createContextRaw();
	Scripting::EventScriptingContext context(runtime, ctx, logger);
	Scripting::ScriptingDatabase database(runtime, ctx, logger, "database.sqlite3", true);
	context.setupContext();
	database.setupContext();
	context.setupLocalStorage();
	context.loadEventHandler(R"(export default "1";)");
	std::optional<std::string> result = context.getModuleProperty("default").asString();
	if (result.has_value()) {
		logger->info("Module default export: {}", *result);
	} else {
		logger->error("Failed to get module default export as string");
	}

	return 0;
}
