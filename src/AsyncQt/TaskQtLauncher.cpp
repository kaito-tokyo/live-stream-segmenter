/*
 * KaitoTokyo AsyncQt Library
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

#include "TaskQtLauncher.hpp"

namespace KaitoTokyo::AsyncQt {

namespace {

Async::Task<void> wrapTask(Async::Task<void> task, std::shared_ptr<const Logger::ILogger> logger,
			   TaskQtLauncher *launcher)
{
	try {
		co_await task;
	} catch (const std::exception &e) {
		logger->error("error=Error\tlocation=TaskQtLauncher::wrapTask\tmessage={}", e.what());
	} catch (...) {
		logger->error("error=UnknownError\tlocation=TaskQtLauncher::wrapTask");
	}

	launcher->deleteLater();
}

} // anonymous namespace

void TaskQtLauncher::launch(Async::Task<void> task, std::shared_ptr<const Logger::ILogger> logger, QObject *parent)
{
	if (!task)
		throw std::invalid_argument("TaskNullError(TaskQtLauncher::launch)");

	if (!logger)
		throw std::invalid_argument("LoggerNullError(TaskQtLauncher::launch)");

	auto *launcher = new TaskQtLauncher(std::move(task), std::move(logger), parent);
	launcher->start();
}

TaskQtLauncher::~TaskQtLauncher() = default;

TaskQtLauncher::TaskQtLauncher(Async::Task<void> task, std::shared_ptr<const Logger::ILogger> logger, QObject *parent)
	: QObject(parent)
{
	wrappedTask_ = wrapTask(std::move(task), std::move(logger), this);
}

void TaskQtLauncher::start()
{
	wrappedTask_.start();
}

} // namespace KaitoTokyo::AsyncQt
