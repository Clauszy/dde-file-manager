/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
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
#include "universalutils.h"

#include <DDBusSender>
#include <QCoreApplication>
#include <QApplication>
#include <QThread>
#include <QDebug>

DFMBASE_BEGIN_NAMESPACE

/*!
 * \brief send a messsage to Notification Center
 * \param msg
 */
void UniversalUtils::notifyMessage(const QString &msg)
{
    DDBusSender()
            .service("org.freedesktop.Notifications")
            .path("/org/freedesktop/Notifications")
            .interface("org.freedesktop.Notifications")
            .method(QString("Notify"))
            .arg(QObject::tr("dde-file-manager"))
            .arg(static_cast<uint>(0))
            .arg(QString("media-eject"))
            .arg(msg)
            .arg(QString())
            .arg(QStringList())
            .arg(QVariantMap())
            .arg(5000)
            .call();
}

/*!
 * \brief send a messsage to Notification Center
 * \param title
 * \param msg
 */
void UniversalUtils::notifyMessage(const QString &title, const QString &msg)
{
    DDBusSender()
            .service("org.freedesktop.Notifications")
            .path("/org/freedesktop/Notifications")
            .interface("org.freedesktop.Notifications")
            .method(QString("Notify"))
            .arg(QObject::tr("dde-file-manager"))
            .arg(static_cast<uint>(0))
            .arg(QString("media-eject"))
            .arg(title)
            .arg(msg)
            .arg(QStringList())
            .arg(QVariantMap())
            .arg(5000)
            .call();
}

/*!
 * \brief Determine if the current user is active or not
 * \return "State" property, we need it's "active" usunally
 */
QString UniversalUtils::userLoginState()
{
    QString state;
    QDBusInterface loginManager("org.freedesktop.login1",
                                "/org/freedesktop/login1/user/self",
                                "org.freedesktop.login1.User",
                                QDBusConnection::systemBus());
    QVariant replay = loginManager.property(("State"));
    if (replay.isValid())
        state = replay.toString();
    return state;
}

/*!
 * \brief UniversalUtils::inMainThread
 * \return true if in main thread
 */
bool UniversalUtils::inMainThread()
{
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}

/*!
 * \brief FileUtils::blockShutdown 调用dbus去设置阻塞睡眠
 * \param replay 输入参数，dbus回复
 */
void UniversalUtils::blockShutdown(QDBusReply<QDBusUnixFileDescriptor> &replay)
{
    qInfo() << " create dbus to block computer shut down!!!";
    if (replay.value().isValid()) {
        qWarning() << "current qt dbus replyBlokShutDown is using!";
        return;
    }

    QDBusInterface loginManager("org.freedesktop.login1",
                                "/org/freedesktop/login1",
                                "org.freedesktop.login1.Manager",
                                QDBusConnection::systemBus());

    QList<QVariant> arg;
    arg << QString("shutdown:sleep:")   // what
        << qApp->applicationDisplayName()   // who
        << QObject::tr("Files are being processed")   // why
        << QString("block");   // mode

    replay = loginManager.callWithArgumentList(QDBus::Block, "Inhibit", arg);
    qInfo() << " create over dbus to block computer shut down!!!";
}

DFMBASE_END_NAMESPACE