/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo CurlHelper Library
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

namespace KaitoTokyo::CurlHelper {

class CurlSlistHandle {
public:
       CurlSlistHandle() = default;
       ~CurlSlistHandle() noexcept
       {
	       curl_slist_free_all(slist_);
       }

       CurlSlistHandle(const CurlSlistHandle &) = delete;
       CurlSlistHandle &operator=(const CurlSlistHandle &) = delete;

       CurlSlistHandle(CurlSlistHandle &&other) noexcept
       {
	       slist_ = other.slist_;
	       other.slist_ = nullptr;
       }

       CurlSlistHandle &operator=(CurlSlistHandle &&other) noexcept
       {
	       if (this != &other) {
		       curl_slist_free_all(slist_);
		       slist_ = other.slist_;
		       other.slist_ = nullptr;
	       }
	       return *this;
       }

       void append(const char *str)
       {
	       slist_ = curl_slist_append(slist_, str);
       }

       curl_slist *get() const noexcept { return slist_; }

private:
       curl_slist *slist_ = nullptr;
};

} // namespace KaitoTokyo::CurlHelper
