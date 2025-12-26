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

#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <curl/curl.h>

namespace KaitoTokyo::CurlHelper {

class CurlUrlSearchParams {
public:
	explicit CurlUrlSearchParams(CURL *curl)
		: curl_(curl ? curl : throw std::invalid_argument("InitError(CurlUrlSearchParams)"))
	{
	}

	~CurlUrlSearchParams() noexcept = default;

	CurlUrlSearchParams(const CurlUrlSearchParams &) = delete;
	CurlUrlSearchParams &operator=(const CurlUrlSearchParams &) = delete;

	void append(const std::string &name, const std::string &value) { params_.emplace_back(name, value); }

	std::string toString() const
	{
		std::ostringstream oss;
		for (std::size_t i = 0; i < params_.size(); i++) {
			if (i > 0) {
				oss << "&";
			}

			const std::string &key = params_[i].first;
			const std::string &value = params_[i].second;

			std::unique_ptr<char, decltype(&curl_free)> escapedKey(
				curl_easy_escape(curl_, key.c_str(), static_cast<int>(key.length())), curl_free);
			std::unique_ptr<char, decltype(&curl_free)> escapedValue(
				curl_easy_escape(curl_, value.c_str(), static_cast<int>(value.length())), curl_free);
			if (!escapedKey || !escapedValue) {
				throw std::runtime_error("EncodeError(CurlUrlSearchParams)");
			}

			oss << escapedKey.get() << "=" << escapedValue.get();
		}
		return oss.str();
	}

private:
	CURL *const curl_;
	std::vector<std::pair<std::string, std::string>> params_;
};

} // namespace KaitoTokyo::CurlHelper
