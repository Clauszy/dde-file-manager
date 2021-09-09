/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     luzhen<luzhen@uniontech.com>
 *
 * Maintainer: luzhen<luzhen@uniontech.com>
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
#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include "dfm-framework/dfm_framework_global.h"
#include <QObject>

DPF_BEGIN_NAMESPACE

// TODO(mozart): not used yet, should do more job.
class Framework : public QObject
{
    Q_OBJECT

    Q_DISABLE_COPY(Framework)
public:
    explicit Framework(QObject *parent = nullptr);

    bool initialize();
};

#endif // FRAMEWORK_H


DPF_END_NAMESPACE
