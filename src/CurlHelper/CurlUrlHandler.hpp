/*
Live Stream Segmenter

MIT License

Copyright (c) 2025 Kaito Udagawa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <curl/curl.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace KaitoTokyo::LiveStreamSegmenter::CurlHelper {

class CurlUrlHandle {
public:
	CurlUrlHandle() : handle_(curl_url())
	{
		if (!handle_) {
			throw std::runtime_error("InitError(CurlUrlHandle)");
		}
	}

	~CurlUrlHandle() noexcept
	{
		if (handle_) {
			curl_url_cleanup(handle_);
		}
	}

	CurlUrlHandle(const CurlUrlHandle &) = delete;
	CurlUrlHandle &operator=(const CurlUrlHandle &) = delete;
	CurlUrlHandle(CurlUrlHandle &&) = delete;
	CurlUrlHandle &operator=(CurlUrlHandle &&) = delete;

	void setUrl(const char *url)
	{
		CURLUcode uc = curl_url_set(handle_, CURLUPART_URL, url, 0);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("URLParseError(CurlUrlHandle):" + std::string(url));
		}
	}

	void appendQuery(const char *query)
	{
		CURLUcode uc = curl_url_set(handle_, CURLUPART_QUERY, query, CURLU_APPENDQUERY);
		if (uc != CURLUE_OK) {
			throw std::runtime_error("QueryAppendError(CurlUrlHandle):" + std::string(query));
		}
	}

	std::unique_ptr<char, decltype(&curl_free)> c_str() const
	{
		char *urlStr = nullptr;
		CURLUcode uc = curl_url_get(handle_, CURLUPART_URL, &urlStr, 0);
		if (uc != CURLUE_OK || !urlStr) {
			throw std::runtime_error("GetUrlError(CurlUrlHandle)");
		}
		return std::unique_ptr<char, decltype(&curl_free)>(urlStr, curl_free);
	}

private:
	CURLU *const handle_;
};

} // namespace KaitoTokyo::LiveStreamSegmenter::CurlHelper
