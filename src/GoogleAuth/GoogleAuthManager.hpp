/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * KaitoTokyo GoogleAuth Library
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

#include <memory>
#include <string_view>

#include <curl/curl.h>

#include <CurlHandle.hpp>
#include <ILogger.hpp>

#include "GoogleAuthResponse.hpp"
#include "GoogleOAuth2ClientCredentials.hpp"

namespace KaitoTokyo::GoogleAuth {

class GoogleAuthManager {
public:
	GoogleAuthManager(std::shared_ptr<CurlHelper::CurlHandle> curl, GoogleOAuth2ClientCredentials clientCredentials,
			  std::shared_ptr<const Logger::ILogger> logger);

	~GoogleAuthManager() noexcept;

	GoogleAuthManager(const GoogleAuthManager &) = delete;
	GoogleAuthManager &operator=(const GoogleAuthManager &) = delete;
	GoogleAuthManager(GoogleAuthManager &&) = delete;
	GoogleAuthManager &operator=(GoogleAuthManager &&) = delete;

	[[nodiscard]]
	GoogleAuthResponse fetchFreshAuthResponse(std::string refreshToken) const;

private:
	const GoogleOAuth2ClientCredentials clientCredentials_;
	const std::shared_ptr<const Logger::ILogger> logger_;

	const std::shared_ptr<CurlHelper::CurlHandle> curl_;
};

} // namespace KaitoTokyo::GoogleAuth
