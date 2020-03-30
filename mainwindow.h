#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "albumview/albumview.h"
#include "allpicview/allpicview.h"
#include "timelineview/timelineview.h"
#include "dbmanager/dbmanager.h"
#include "searchview/searchview.h"
#include "controller/commandline.h"
#include "controller/exporter.h"
#include "widgets/dialogs/imginfodialog.h"
#include "module/slideshow/slideshowpanel.h"
//#include "plugin.h"
//#include "plugintest.h"
#include <QPluginLoader>

#include <DMainWindow>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <DSegmentedControl>
#include <DTitlebar>
#include <QStackedWidget>
#include <DSearchEdit>
#include <DLabel>
#include <QStatusBar>
#include <DTabBar>
#include <QButtonGroup>
#include <DSuggestButton>
#include <DProgressBar>


#define DEFAULT_WINDOWS_WIDTH   1300
#define DEFAULT_WINDOWS_HEIGHT  640
#define MIX_WINDOWS_WIDTH       1300
#define MIX_WINDOWS_HEIGHT      848
//#define TITLEBAR_BLANK_WIDTH    235
#define TITLEBAR_BLANK_WIDTH 365
#define TITLEBAR_BTNWIDGET_WIDTH 240
//#define TITLEBAR_BTNWIDGET_WIDTH 280
#define TITLEBAR_ICON_WIDTH 50

DWIDGET_USE_NAMESPACE

extern bool bfirstopen;
extern bool bfirstandviewimage;
//namespace Ui {
//class MainWindow;
//}

class MainWindow : public DMainWindow, public ImageEngineImportObject
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow() override;

    bool imageImported(bool success) override;
    void initConnections();
    //初始化UI
    void initUI();
    void initWaitDialog();
    void initShortcut();
    void initTitleBar();
    void initCentralWidget();
//    void initStatusBar();
    void setWaitDialogColor();
    void showCreateDialog(QStringList imgpaths);
    void onShowImageInfo(const QString &path);
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
//    void themeTypeChanged();

private:
    void initShortcutKey();
    void thumbnailZoomIn();
    void thumbnailZoomOut();
    QJsonObject createShorcutJson();
    void saveWindowState();
    void loadWindowState();
    void saveZoomRatio();
    void loadZoomRatio();
    void viewImageClose();

protected:
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
//    void timerEvent(QTimerEvent *e)Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;

signals:
    void sigTitleMenuImportClicked();
    bool sigImageImported(bool success);

private slots:

    void allPicBtnClicked();

    void timeLineBtnClicked();
    void albumBtnClicked();

    void onCreateAlbum(QStringList imagepaths);
#if 1
    void onViewCreateAlbum(QString imgpath);
#endif
    void onSearchEditFinished();
    void onTitleBarMenuClicked(QAction *action);
    void onUpdateAllpicsNumLabel();
    void onImprotBtnClicked();
    void onUpdateCentralWidget();
    void onNewAPPOpen(qint64 pid, const QStringList &arguments);
    void onLoadingFinished();
private:
//    Ui::MainWindow *ui;
//    QListWidget *m_listWidget;

    int m_allPicNum;
    int m_iCurrentView;
    bool m_bTitleMenuImportClicked;
    bool m_bImport = false;

    QWidget *m_titleBtnWidget;
    QWidget *m_ImgWidget;
    DMenu *m_pTitleBarMenu;
//    DPushButton* m_pAllPicBtn;
//    DPushButton* m_pTimeLineBtn;
//    DPushButton* m_pAlbumBtn;
    DSearchEdit *m_pSearchEdit;
    QStackedWidget *m_pCenterWidget;
    CommandLine *m_commandLine;

    AlbumView *m_pAlbumview;                    //相册照片界面视图
    AllPicView *m_pAllPicView;                  //所有照片界面视图
    TimeLineView *m_pTimeLineView;              //时间线界面视图
    SearchView *m_pSearchView;                  //搜索界面视图
    SlideShowPanel *m_slidePanel;               //幻灯片播放视图
//    DStatusBar* m_pStatusBar;
//    DLabel* m_pAllPicNumLabel;
//    DSlider* m_pSlider;
    DBManager *m_pDBManager;
    QMap<QString, ImgInfoDialog *> m_propertyDialogs{};
    int m_backIndex;
    int m_backIndex_fromSlide;
    int m_pSliderPos = 2;
    DPushButton *m_pItemButton;

    QButtonGroup *btnGroup;
    DPushButton *m_pAllPicBtn;
    DPushButton *m_pTimeBtn;
    DPushButton *m_pAlbumBtn;
//    DSuggestButton *m_pAllPicBtn;
//    DSuggestButton *m_pTimeBtn;
//    DSuggestButton *m_pAlbumBtn;
//    int timer;
    DDialog  *m_waitdailog;
    DProgressBar *m_importBar = nullptr;
    DLabel *m_waitlabel = nullptr;
    DLabel *m_countLabel = nullptr;

    QString       m_SearchKey;      //搜索框查询信息
};

#endif // MAINWINDOW_H
