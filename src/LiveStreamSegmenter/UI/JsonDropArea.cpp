/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 * SPDX-License-Identifier: MIT
 *
 * Live Stream Segmenter - UI Module
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

#include "JsonDropArea.hpp"

#include <QFileInfo>

namespace KaitoTokyo::LiveStreamSegmenter::UI {

namespace {

inline std::optional<QString> getLocalJsonFilePathIfExists(const QMimeData *mimeData)
{
	if (mimeData == nullptr || !mimeData->hasUrls())
		return std::nullopt;

	const QList<QUrl> urls = mimeData->urls();
	if (urls.size() != 1)
		return std::nullopt;

	const QUrl &firstUrl = urls.first();
	if (firstUrl.isEmpty() || !firstUrl.isLocalFile())
		return std::nullopt;

	QString localFile = firstUrl.toLocalFile();
	QFileInfo info(localFile);
	if (!info.exists() || !info.isFile() || info.suffix().compare("json", Qt::CaseInsensitive) != 0)
		return std::nullopt;

	return localFile;
}

} // anonymous namespace

JsonDropArea::JsonDropArea(QWidget *parent) : QLabel(parent)
{
	setAcceptDrops(true);
}

void JsonDropArea::dragEnterEvent(QDragEnterEvent *event)
{
	if (getLocalJsonFilePathIfExists(event->mimeData())) {
		event->acceptProposedAction();
	}
}

void JsonDropArea::dragMoveEvent(QDragMoveEvent *event)
{
	if (getLocalJsonFilePathIfExists(event->mimeData())) {
		event->acceptProposedAction();
	}
}

void JsonDropArea::dropEvent(QDropEvent *event)
{
	if (auto localFilePath = getLocalJsonFilePathIfExists(event->mimeData())) {
		event->acceptProposedAction();
		emit jsonFileDropped(*localFilePath);
	}
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
