/*
 * Copyright (C) 2022 Uniontech Software Technology Co., Ltd.
 *
 * Author:     zhangsheng<zhangsheng@uniontech.com>
 *
 * Maintainer: max-lv<lvwujun@uniontech.com>
 *             lanxuesong<lanxuesong@uniontech.com>
 *             xushitong<xushitong@uniontech.com>
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
#ifndef DAEMONPLUGIN_ACCESSCONTROL_GLOBAL_H
#define DAEMONPLUGIN_ACCESSCONTROL_GLOBAL_H

#define DAEMONPAC_NAMESPACE daemonplugin_accesscontrol

#define DAEMONPAC_BEGIN_NAMESPACE namespace DAEMONPAC_NAMESPACE {
#define DAEMONPAC_END_NAMESPACE }
#define DAEMONPAC_USE_NAMESPACE using namespace DAEMONPAC_NAMESPACE;

DAEMONPAC_BEGIN_NAMESPACE

// error code of change disk password
enum DPCErrorCode {
    kNoError = 0,
    kAuthenticationFailed,
    kInitFailed,
    kDeviceLoadFailed,
    kPasswordChangeFailed,
    kPasswordWrong,
    kAccessDiskFailed,   // Unable to get the encrypted disk list
    kPasswordInconsistent   // Passwords of disks are different
};

DAEMONPAC_END_NAMESPACE

#endif   // DAEMONPLUGIN_ACCESSCONTROL_GLOBAL_H
