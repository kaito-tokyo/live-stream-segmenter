/*
 * Live Stream Segmenter
 * Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; for more details see the file 
 * "LICENSE.GPL-3.0-or-later" in the distribution root.
 */

#include "JsonDropArea.hpp"

namespace KaitoTokyo::LiveStreamSegmenter::UI {

namespace {

inline bool isJsonFile(const QMimeData *mimeData)
{
	if (mimeData != nullptr && mimeData->hasUrls()) {
		const QList<QUrl> urls = mimeData->urls();
		if (urls.isEmpty()) {
			return false;
		} else {
			return urls.first().toLocalFile().endsWith(".json", Qt::CaseInsensitive);
		}
	} else {
		return false;
	}
}

} // anonymous namespace

JsonDropArea::JsonDropArea(QWidget *parent) : QLabel(parent)
{
	setAcceptDrops(true);
}

void JsonDropArea::dragEnterEvent(QDragEnterEvent *event)
{
	if (isJsonFile(event->mimeData())) {
		event->acceptProposedAction();
	}
}

void JsonDropArea::dragMoveEvent(QDragMoveEvent *event)
{
	if (isJsonFile(event->mimeData())) {
		event->acceptProposedAction();
	}
}

void JsonDropArea::dropEvent(QDropEvent *event)
{
    if (isJsonFile(event->mimeData())) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty()) {
            event->acceptProposedAction();
            emit fileDropped(urls.first().toLocalFile());
        }
    }
}

} // namespace KaitoTokyo::LiveStreamSegmenter::UI
