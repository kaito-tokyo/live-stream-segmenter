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

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <ILogger.hpp>

#include "ScriptingDatabase.hpp"
#include "ScriptingRuntime.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Scripting {

class EventScriptingContext {
public:
	EventScriptingContext(std::shared_ptr<ScriptingRuntime> runtime, std::shared_ptr<Logger::ILogger> logger,
			      const std::filesystem::path &dbPath);
	~EventScriptingContext();

	void loadEventHandler(const char *script);
	ScopedJSValue getModuleProperty(const char *property) const;

private:
	std::shared_ptr<ScriptingRuntime> runtime_;
	const std::shared_ptr<const Logger::ILogger> logger_;

	const std::shared_ptr<JSContext> ctx_;
	std::shared_ptr<ScriptingDatabase> database_;

	ScopedJSValue eventHandlerNs_;

	void loadModule(std::uint32_t size, const std::uint8_t *buf);
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Scripting
