/*
 * Live Stream Segmenter - Controller Module
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>

#include <QObject>

#include <ILogger.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

class YouTubeStreamSegmenter : public QObject {
	Q_OBJECT

public:
	explicit YouTubeStreamSegmenter(std::shared_ptr<const Logger::ILogger> logger);
	~YouTubeStreamSegmenter() override;

	YouTubeStreamSegmenter(const YouTubeStreamSegmenter &) = delete;
	YouTubeStreamSegmenter &operator=(const YouTubeStreamSegmenter &) = delete;
	YouTubeStreamSegmenter(YouTubeStreamSegmenter &&) = delete;
	YouTubeStreamSegmenter &operator=(YouTubeStreamSegmenter &&) = delete;

	void startContinuousSession();
	void stopContinuousSession();
	void segmentCurrentSession();

private:
	std::shared_ptr<const Logger::ILogger> logger_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
