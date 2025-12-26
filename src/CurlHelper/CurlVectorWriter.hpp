/*
 * KaitoTokyo CurlHelper Library
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

#include <curl/curl.h>

#include <cstddef>
#include <limits>
#include <vector>

namespace KaitoTokyo::CurlHelper {

using CurlVectorWriterBuffer = std::vector<char>;

inline std::size_t CurlVectorWriter(void *contents, std::size_t size, std::size_t nmemb, void *userp) noexcept
{
	if (size != 0 && nmemb > (std::numeric_limits<std::size_t>::max() / size)) {
		return 0; // Signal error
	}

	std::size_t totalSize = size * nmemb;
	try {
		auto *vec = static_cast<CurlVectorWriterBuffer *>(userp);
		vec->insert(vec->end(), static_cast<char *>(contents), static_cast<char *>(contents) + totalSize);
	} catch (...) {
		return 0; // Signal error
	}
	return totalSize;
}

} // namespace KaitoTokyo::CurlHelper
