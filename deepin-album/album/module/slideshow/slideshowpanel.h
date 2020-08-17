#ifndef SLIDESHOWPANEL_H
#define SLIDESHOWPANEL_H

#include "imageanimation.h"
#include "controller/viewerthememanager.h"
#include "controller/signalmanager.h"

#include <DMenu>
#include <DIconButton>
#include <DLabel>
#include <QHBoxLayout>
#include <DFloatingWidget>
#include <QShortcut>

class SlideShowBottomBar : public DFloatingWidget
{
    Q_OBJECT
public:
    explicit SlideShowBottomBar(QWidget *parent = nullptr);
public:
    DIconButton *m_preButton;
    DIconButton *m_nextButton;
    DIconButton *m_playpauseButton;
    DIconButton *m_cancelButton;
    int a = 0;
private slots:
//    void onThemeChanged(ViewerThemeManager::AppTheme theme);
signals:
    void showPrevious();
    void showPause();
    void showNext();
    void showCancel();
};

class SlideShowPanel : public QWidget
{
    Q_OBJECT
public:
    enum MenuItemId {
        IdStopslideshow,
        IdPlayOrPause,
        IdPlay,
        IdPause
    };
    explicit SlideShowPanel(QWidget *parent = nullptr);
    void initConnections();
    void initMenu();
    void appendAction(int id, const QString &text, const QString &shortcut);
    void backToLastPanel();
    void showNormal();
    void showFullScreen();
signals:

public slots:
    void startSlideShow(const SignalManager::ViewInfo &vinfo, bool inDB);
    void onMenuItemClicked(QAction *action);
    void onThemeChanged(ViewerThemeManager::AppTheme dark);
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
private:
    SlideShowBottomBar *slideshowbottombar;
    ImageAnimation *m_animation;
    DMenu *m_menu;
    QShortcut *m_sEsc;
    SignalManager::ViewInfo m_vinfo;
    QColor m_bgColor;
    bool m_isMaximized;
    int m_hideCursorTid;
};

#endif // SLIDESHOWPANEL_H
