/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     huanyu<huanyub@uniontech.com>
 *
 * Maintainer: zhengyouge<zhengyouge@uniontech.com>
 *             zhangyu<zhangyub@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "localmenu.h"

#include "dfm-base/base/urlroute.h"

#include <QUrl>
#include <QDir>

DFMBASE_BEGIN_NAMESPACE

LocalMenu::LocalMenu(QObject *parent)
    : AbstractMenu(parent)
{
}

QMenu *LocalMenu::build(QWidget *parent, AbstractMenu::MenuMode mode,
                        const QUrl &rootUrl, const QUrl &foucsUrl,
                        const QList<QUrl> &selected, QVariant customData)
{
    QString path = UrlRoute::urlToPath(rootUrl);
    if (!QDir(path).exists())
        return nullptr;
    return AbstractMenu::build(parent, mode, rootUrl, foucsUrl, selected, customData);
}

DFMBASE_END_NAMESPACE