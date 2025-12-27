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

#include "YouTubeStreamSegmenter.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::Controller {

YouTubeStreamSegmenter::YouTubeStreamSegmenter(std::shared_ptr<const Logger::ILogger> logger)
	: QObject(nullptr), logger_(std::move(logger))
{
}

YouTubeStreamSegmenter::~YouTubeStreamSegmenter() = default;

void YouTubeStreamSegmenter::startContinuousSession() {
	logger_->info("Starting continuous YouTube live stream session");
}

void YouTubeStreamSegmenter::stopContinuousSession() {
	logger_->info("Stopping continuous YouTube live stream session");
}

void YouTubeStreamSegmenter::segmentCurrentSession() {
	logger_->info("Segmenting current YouTube live stream session");
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Controller
