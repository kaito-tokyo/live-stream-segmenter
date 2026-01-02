/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Live Stream Segmenter - Store Module
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

#include "AuthStore.hpp"

#include <fstream>
#include <stdexcept>

#include <QString>

#include <qtkeychain/keychain.h>
#include <nlohmann/json.hpp>

#include <obs-frontend-api.h>

#include <KaitoTokyo/ObsBridgeUtils/ObsUnique.hpp>

namespace KaitoTokyo::LiveStreamSegmenter::Store {

namespace {

const QString kKeychainService = "tokyo.kaito.live-stream-segmenter";

} // anonymous namespace

AuthStore::AuthStore(QObject *parent) : QObject(parent) {}

AuthStore::~AuthStore() noexcept = default;

void AuthStore::setLogger(std::shared_ptr<const Logger::ILogger> logger)
{
	std::scoped_lock lock(mutex_);
	logger_ = std::move(logger);
}

void AuthStore::setGoogleOAuth2ClientCredentials(GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials)
{
	std::scoped_lock lock(mutex_);
	googleOAuth2ClientCredentials_ = std::move(googleOAuth2ClientCredentials);
}

GoogleAuth::GoogleOAuth2ClientCredentials AuthStore::getGoogleOAuth2ClientCredentials() const
{
	std::scoped_lock lock(mutex_);
	return googleOAuth2ClientCredentials_;
}

void AuthStore::setGoogleTokenState(GoogleAuth::GoogleTokenState tokenState)
{
	std::scoped_lock lock(mutex_);
	googleTokenState_ = std::move(tokenState);
}

GoogleAuth::GoogleTokenState AuthStore::getGoogleTokenState() const
{
	std::scoped_lock lock(mutex_);
	return googleTokenState_;
}

void AuthStore::save()
{
	GoogleAuth::GoogleOAuth2ClientCredentials googleOAuth2ClientCredentials;
	GoogleAuth::GoogleTokenState googleTokenState;
	{
		std::scoped_lock lock(mutex_);
		googleOAuth2ClientCredentials = googleOAuth2ClientCredentials_;
		googleTokenState = googleTokenState_;
	}

	googleTokenState.access_token.clear();

	nlohmann::json jGoogleOAuth2ClientCredentials = googleOAuth2ClientCredentials;
	nlohmann::json jGoogleTokenState = googleTokenState;

	auto writeGoogleOAuth2ClientCredentialsJob = new QKeychain::WritePasswordJob(kKeychainService, this);
	writeGoogleOAuth2ClientCredentialsJob->setAutoDelete(true);
	writeGoogleOAuth2ClientCredentialsJob->setKey("googleOAuth2ClientCredentials");
	writeGoogleOAuth2ClientCredentialsJob->setTextData(
		QString::fromStdString(jGoogleOAuth2ClientCredentials.dump()));
	connect(writeGoogleOAuth2ClientCredentialsJob, &QKeychain::Job::finished, this, &AuthStore::onWriteFinished);
	writeGoogleOAuth2ClientCredentialsJob->start();

	auto writeGoogleTokenStateJob = new QKeychain::WritePasswordJob(kKeychainService, this);
	writeGoogleTokenStateJob->setAutoDelete(true);
	writeGoogleTokenStateJob->setKey("googleTokenState");
	writeGoogleTokenStateJob->setTextData(QString::fromStdString(jGoogleTokenState.dump()));
	connect(writeGoogleTokenStateJob, &QKeychain::Job::finished, this, &AuthStore::onWriteFinished);
	writeGoogleTokenStateJob->start();
}

void AuthStore::onWriteFinished(QKeychain::Job *job)
{
	std::shared_ptr<const Logger::ILogger> logger;
	{
		std::scoped_lock lock(mutex_);
		logger = logger_;
	}

	if (logger) {
		std::string key = job->key().toStdString();
		if (job->error()) {
			std::string error = job->errorString().toStdString();
			logger->error("KeychainWriteError(AuthStore::onWriteFinished)",
				       {{"key", key}, {"error", error}});
		} else {
			logger->info("KeychainWriteSuccess", {{"key", key}});
		}
	}
}

void AuthStore::restore()
{
	auto readGoogleOAuth2ClientCredentialsJob = new QKeychain::ReadPasswordJob(kKeychainService, this);
	readGoogleOAuth2ClientCredentialsJob->setAutoDelete(true);
	readGoogleOAuth2ClientCredentialsJob->setKey("googleOAuth2ClientCredentials");
	connect(readGoogleOAuth2ClientCredentialsJob, &QKeychain::Job::finished, this,
		&AuthStore::onReadGoogleOAuth2ClientCredentialsFinished);
	readGoogleOAuth2ClientCredentialsJob->start();

	auto readGoogleTokenStateJob = new QKeychain::ReadPasswordJob(kKeychainService, this);
	readGoogleTokenStateJob->setAutoDelete(true);
	readGoogleTokenStateJob->setKey("googleTokenState");
	connect(readGoogleTokenStateJob, &QKeychain::Job::finished, this, &AuthStore::onReadGoogleTokenStateFinished);
	readGoogleTokenStateJob->start();
}

void AuthStore::onReadGoogleOAuth2ClientCredentialsFinished(QKeychain::Job *job)
try {
	auto readJob = static_cast<QKeychain::ReadPasswordJob *>(job);

	if (readJob->error() == QKeychain::Error::EntryNotFound) {
		std::scoped_lock lock(mutex_);
		if (logger_) {
			logger_->info("NoGoogleOAuth2ClientCredentialsInKeychain");
		}
		return;
	} else if (readJob->error()) {
		std::scoped_lock lock(mutex_);
		if (logger_) {
			std::string error = readJob->errorString().toStdString();
			logger_->error("KeychainReadError", {{"error", error}});
		}
		return;
	}

	std::string jsonStr = readJob->textData().toStdString();
	nlohmann::json j = nlohmann::json::parse(jsonStr);

	std::scoped_lock lock(mutex_);
	try {
		j.get_to(googleOAuth2ClientCredentials_);

		if (logger_) {
			logger_->info("RestoredGoogleOAuth2ClientCredentials");
		}
	} catch (const std::exception &e) {
		if (logger_) {
			logger_->error("KeychainJsonContentError", {{"what", e.what()}});
		}
		googleOAuth2ClientCredentials_ = {};
	} catch (...) {
		if (logger_) {
			logger_->error("KeychainUnknownError");
		}
		googleOAuth2ClientCredentials_ = {};
	}
} catch (const std::exception &e) {
	std::scoped_lock lock(mutex_);
	if (logger_) {
		logger_->error("KeychainJsonParseError", {{"what", e.what()}});
	}
	googleOAuth2ClientCredentials_ = {};
	return;
} catch (...) {
	std::scoped_lock lock(mutex_);
	if (logger_) {
		logger_->error("KeychainUnknownError");
	}
	googleOAuth2ClientCredentials_ = {};
	return;
}

void AuthStore::onReadGoogleTokenStateFinished(QKeychain::Job *job)
try {
	auto readJob = static_cast<QKeychain::ReadPasswordJob *>(job);

	if (readJob->error() == QKeychain::Error::EntryNotFound) {
		std::scoped_lock lock(mutex_);
		if (logger_) {
			logger_->info("NoGoogleTokenStateInKeychain");
		}
		return;
	} else if (readJob->error()) {
		std::scoped_lock lock(mutex_);
		if (logger_) {
			std::string error = readJob->errorString().toStdString();
			logger_->error("KeychainReadError", {{"error", error}});
		}
		return;
	}

	std::string jsonStr = readJob->textData().toStdString();
	nlohmann::json j = nlohmann::json::parse(jsonStr);

	std::scoped_lock lock(mutex_);
	try {
		j.get_to(googleTokenState_);
		googleTokenState_.access_token.clear();

		if (logger_) {
			logger_->info("RestoredGoogleTokenState");
		}
	} catch (const std::exception &e) {
		if (logger_) {
			logger_->error("KeychainJsonParseError", {{"what", e.what()}});
		}
		googleTokenState_ = {};
	} catch (...) {
		if (logger_) {
			logger_->error("KeychainUnknownError");
		}
		googleTokenState_ = {};
	}
} catch (const std::exception &e) {
	std::scoped_lock lock(mutex_);
	if (logger_) {
		logger_->error("KeychainJsonParseError", {{"what", e.what()}});
	}
	googleTokenState_ = {};
	return;
} catch (...) {
	std::scoped_lock lock(mutex_);
	if (logger_) {
		logger_->error("KeychainUnknownError");
	}
	googleTokenState_ = {};
	return;
}

} // namespace KaitoTokyo::LiveStreamSegmenter::Store
