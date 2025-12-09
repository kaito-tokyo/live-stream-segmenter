
/*
 * MIT License
 * 
 * Copyright (c) 2025 Kaito Udagawa
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

#include <string>
#include <vector>

namespace KaitoTokyo::LiveStreamSegmenter::API {

/**
 * @brief Represents a YouTube stream key and its associated metadata.
 * Contains stream ID, title, stream name, resolution, and frame rate.
 */
struct YouTubeStreamKey {
	std::string id;
	std::string title;
	std::string streamName;

	std::string resolution;
	std::string frameRate;
};

/**
 * @brief Parameters for creating or updating a YouTube broadcast.
 * Includes title, description, scheduled start time, and privacy status.
 */
struct YouTubeBroadcastParams {
	std::string title;
	std::string description;
	std::string scheduledStartTime;
	std::string privacyStatus;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::API
