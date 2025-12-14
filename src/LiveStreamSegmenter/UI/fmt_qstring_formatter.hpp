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

#pragma once

#include <string>

#include <QString>

#include <fmt/format.h>

template<> struct fmt::formatter<QString> : fmt::formatter<std::string> {
	template<typename FormatContext>
	auto format(const QString &qstr, FormatContext &ctx) const -> decltype(ctx.out())
	{
		return fmt::formatter<std::string>::format(qstr.toStdString(), ctx);
	}
};
