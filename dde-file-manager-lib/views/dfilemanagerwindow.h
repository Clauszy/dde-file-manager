#ifndef DFILEMANAGERWINDOW_H
#define DFILEMANAGERWINDOW_H

#include "durl.h"
#include "dfmglobal.h"
#include "dfmabstracteventhandler.h"
#include "shutil/filebatchprocess.h"


#include <DMainWindow>
#include <atomic>
#include <tuple>
#include <array>

#define DEFAULT_WINDOWS_WIDTH 960
#define DEFAULT_WINDOWS_HEIGHT 540
#define LEFTSIDEBAR_MAX_WIDTH 200
#define LEFTSIDEBAR_MIN_WIDTH 48
#define TITLE_FIXED_HEIGHT 40

class DTitleBar;
class DLeftSideBar;
class DToolBar;
class DDetailView;
class QStatusBar;
class QFrame;
class QHBoxLayout;
class QVBoxLayout;
class QSplitter;
class QResizeEvent;
class DSplitter;

class ExtendView;
class QStackedLayout;
class QPushButton;

class DStatusBar;
class DFMEvent;
class DFMUrlBaseEvent;
class TabBar;
class Tab;
class RecordRenameBarState;


class DFMUrlListBaseEvent;

DFM_BEGIN_NAMESPACE
class DFMBaseView;
DFM_END_NAMESPACE

DWIDGET_USE_NAMESPACE
DFM_USE_NAMESPACE

class DFileManagerWindowPrivate;
class DFileManagerWindow : public DMainWindow, public DFMAbstractEventHandler
{
    Q_OBJECT
public:
    explicit DFileManagerWindow(QWidget *parent = 0);
    explicit DFileManagerWindow(const DUrl &fileUrl, QWidget *parent = 0);
    virtual ~DFileManagerWindow();

    DUrl currentUrl() const;
    bool isCurrentUrlSupportSearch(const DUrl& currentUrl);

    DToolBar* getToolBar() const;
    DFMBaseView *getFileView() const;
    DLeftSideBar *getLeftSideBar() const;

    quint64 windowId();

    bool tabAddable() const;
    void hideRenameBar() noexcept;
    void requestToSelectUrls();

signals:
    void aboutToClose();
    void positionChanged(const QPoint &pos);
    void currentUrlChanged();

public slots:
    void moveCenter(const QPoint &cp);
    void moveTopRight();
    void moveCenterByRect(QRect rect);
    void moveTopRightByRect(QRect rect);

    bool cd(const DUrl &fileUrl, bool canFetchNetwork = true);

    bool openNewTab(DUrl fileUrl);
    void switchToView(DFMBaseView *view);
    void onTabAddableChanged(bool addable);
    void onCurrentTabChanged(int tabIndex);
    void onRequestCloseTab(const int index, const bool& remainState);
    void closeCurrentTab(quint64 winId);
    void showNewTabButton();
    void hideNewTabButton();
    void showEmptyTrashButton();
    void hideEmptyTrashButton();
    void onNewTabButtonClicked();
    void requestEmptyTrashFiles();
    void onTrashStateChanged();

    void onShowRenameBar(const DFMUrlListBaseEvent& event)noexcept;
    void onTabBarCurrentIndexChange(const int& index)noexcept;
    void onReuqestCacheRenameBarState() const;

protected:
    void closeEvent(QCloseEvent* event)  Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;

    bool fmEvent(const QSharedPointer<DFMEvent> &event, QVariant *resultData = 0) Q_DECL_OVERRIDE;
    QObject *object() const Q_DECL_OVERRIDE;

    virtual void handleNewView(DFMBaseView *view);

    void initData();
    void initUI();

    void initTitleFrame();
    void initTitleBar();
    void initSplitter();

    void initLeftSideBar();

    void initRightView();

    void initRenameBarState();

    void initToolBar();
    void initTabBar();
    void initViewLayout();

    void initCentralWidget();
    void initConnect();

private:
    Tab* m_currentTab{ nullptr };
    std::atomic<bool> m_tabBarIndexChangeFlag{ false };//###: when the index of tabbar changed hide RenameBar through the value.

    QScopedPointer<DFileManagerWindowPrivate> d_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(d_ptr), DFileManagerWindow)

public:
    static std::unique_ptr<RecordRenameBarState>  renameBarState;//###: record pattern of RenameBar and the string of QLineEdit's content.
    static std::atomic<bool> flagForNewWindowFromTab;           //###: open a new window form a already has tab, this will be true.
                                                               //and after opening new window this will be back to false.
};

#endif // DFILEMANAGERWINDOW_H
