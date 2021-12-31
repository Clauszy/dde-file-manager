/*
 * Copyright (C) 2021 Uniontech Software Technology Co., Ltd.
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
#include "sidebar.h"
#include "views/sidebarwidget.h"
#include "views/sidebaritem.h"
#include "utils/sidebarhelper.h"

#include "services/filemanager/windows/windowsservice.h"
#include "dfm-base/widgets/dfmwindow/filemanagerwindow.h"
#include "dfm-base/base/urlroute.h"
#include "dfm-base/base/standardpaths.h"

#include <dfm-framework/framework.h>

DPSIDEBAR_USE_NAMESPACE
DSB_FM_USE_NAMESPACE
DFMBASE_USE_NAMESPACE

namespace GlobalPrivate {
static WindowsService *windowService { nullptr };
}   // namespace GlobalPrivate

void SideBar::initSideBar(SideBarWidget *sidebar)
{
    Q_ASSERT_X(sidebar, "SideBar", "SideBar is NULL");

    QUrl &&homeUrl = UrlRoute::pathToReal(QDir::home().path());
    QUrl &&desktopUrl = UrlRoute::pathToReal(StandardPaths::location(StandardPaths::kDesktopPath));
    QUrl &&videosUrl = UrlRoute::pathToReal(StandardPaths::location(StandardPaths::kVideosPath));
    QUrl &&musicUrl = UrlRoute::pathToReal(StandardPaths::location(StandardPaths::kMusicPath));
    QUrl &&picturesUrl = UrlRoute::pathToReal(StandardPaths::location(StandardPaths::kPicturesPath));
    QUrl &&documentsUrl = UrlRoute::pathToReal(StandardPaths::location(StandardPaths::kDocumentsPath));
    QUrl &&downloadsUrl = UrlRoute::pathToReal(StandardPaths::location(StandardPaths::kDownloadsPath));

    // TODO(zhangs): follow icons is error, fix it
    QIcon &&homeIcon = QIcon::fromTheme(StandardPaths::iconName(StandardPaths::kHomePath));
    QIcon &&desktopIcon = QIcon::fromTheme(StandardPaths::iconName(StandardPaths::kDesktopPath));
    QIcon &&videosIcon = QIcon::fromTheme(StandardPaths::iconName(StandardPaths::kVideosPath));
    QIcon &&musicIcon = QIcon::fromTheme(StandardPaths::iconName(StandardPaths::kMusicPath));
    QIcon &&picturesIcon = QIcon::fromTheme(StandardPaths::iconName(StandardPaths::kPicturesPath));
    QIcon &&documentsIcon = QIcon::fromTheme(StandardPaths::iconName(StandardPaths::kDocumentsPath));
    QIcon &&downloadsIcon = QIcon::fromTheme(StandardPaths::iconName(StandardPaths::kDownloadsPath));

    auto homeItem = new SideBarItem(homeIcon, QObject::tr("Home"), "core", homeUrl);
    homeItem->setFlags(homeItem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled));

    auto desktopitem = new SideBarItem(desktopIcon, QObject::tr("Desktop"), "core", desktopUrl);
    desktopitem->setFlags(desktopitem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled));

    auto videoitem = new SideBarItem(videosIcon, QObject::tr("Video"), "core", videosUrl);
    videoitem->setFlags(videoitem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled));

    auto musicitem = new SideBarItem(musicIcon, QObject::tr("Music"), "core", musicUrl);
    musicitem->setFlags(musicitem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled));

    auto picturesitem = new SideBarItem(picturesIcon, QObject::tr("Pictures"), "core", picturesUrl);
    picturesitem->setFlags(picturesitem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled));

    auto documentsitem = new SideBarItem(documentsIcon, QObject::tr("Documents"), "core", documentsUrl);
    documentsitem->setFlags(documentsitem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled));

    auto downloadsitem = new SideBarItem(downloadsIcon, QObject::tr("Downloads"), "core", downloadsUrl);
    downloadsitem->setFlags(downloadsitem->flags() & ~(Qt::ItemIsEditable | Qt::ItemIsDragEnabled));

    sidebar->addItem(homeItem);
    sidebar->addItem(desktopitem);
    sidebar->addItem(videoitem);
    sidebar->addItem(musicitem);
    sidebar->addItem(picturesitem);
    sidebar->addItem(documentsitem);
    sidebar->addItem(downloadsitem);
}

void SideBar::initialize()
{
    auto &ctx = dpfInstance.serviceContext();
    Q_ASSERT_X(ctx.loaded(WindowsService::name()), "SideBar", "WindowService not loaded");
    GlobalPrivate::windowService = ctx.service<WindowsService>(WindowsService::name());
    connect(GlobalPrivate::windowService, &WindowsService::windowOpened, this, &SideBar::onWindowOpened, Qt::DirectConnection);
    connect(GlobalPrivate::windowService, &WindowsService::windowClosed, this, &SideBar::onWindowClosed, Qt::DirectConnection);
}

bool SideBar::start()
{
    return true;
}

dpf::Plugin::ShutdownFlag SideBar::stop()
{
    return kSync;
}

void SideBar::onWindowOpened(quint64 windId)
{
    auto window = GlobalPrivate::windowService->findWindowById(windId);
    Q_ASSERT_X(window, "SideBar", "Cannot find window by id");
    auto sidebar = new SideBarWidget;
    initSideBar(sidebar);
    window->installSideBar(sidebar);
    SideBarHelper::addSideBar(windId, sidebar);
}

void SideBar::onWindowClosed(quint64 winId)
{
    SideBarHelper::removeSideBar(winId);
}