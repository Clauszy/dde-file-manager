/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     liuzhangjian<liuzhangjian@uniontech.com>
 *
 * Maintainer: liuzhangjian<liuzhangjian@uniontech.com>
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
#ifndef ANYTHINGSEARCH_H
#define ANYTHINGSEARCH_H

#include "search/searcher/abstractsearcher.h"

#include <QSharedPointer>
#include <QTime>
#include <QMutex>

class ComDeepinAnythingInterface;
class AnythingSearcher : public AbstractSearcher
{
    Q_OBJECT
    friend class TaskCommander;
    friend class TaskCommanderPrivate;

private:
    explicit AnythingSearcher(const QUrl &url, const QString &keyword, QObject *parent = nullptr);
    virtual ~AnythingSearcher() override;

    static bool isSupported(const QUrl &url);
    bool search() override;
    void stop() override;
    bool hasItem() const override;
    QStringList takeAll() override;
    void tryNotify();

private:
    ComDeepinAnythingInterface *anythingInterface = nullptr;
    QAtomicInt status = kReady;
    QStringList allResults;
    mutable QMutex mutex;

    //计时
    QTime notifyTimer;
    int lastEmit = 0;
};

#endif   // ANYTHINGSEARCH_H