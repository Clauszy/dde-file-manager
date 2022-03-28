/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangyu<zhangyub@uniontech.com>
 *
 * Maintainer: zhangyu<zhangyub@uniontech.com>
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
#ifndef MENU_DEFINES_H
#define MENU_DEFINES_H

#include "dfm_common_service_global.h"

DSC_BEGIN_NAMESPACE

namespace MenuParamKey {
// file menu params for initialize
static constexpr char kCurrentDir[] = "currentDir" ;
static constexpr char kFocusFile[] = "focusFile" ;
static constexpr char kSelectFiles[] = "selectFiles";
static constexpr char kOnDesktop[] = "onDesktop";
static constexpr char kIsEmptyArea[] = "isEmptyArea";
}

namespace ActionPropertyKey {
// key for action property
static constexpr char kActionID[] = "actionID";

}

DSC_END_NAMESPACE

#endif // MENU_DEFINES_H
