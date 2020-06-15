/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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

#ifndef VIEWMAINWINDOW_H
#define VIEWMAINWINDOW_H

#include "frame/mainwidget.h"
#include "controller/viewerthememanager.h"
#include "dbmanager/dbmanager.h"
//#include "controller/exporter.h"
#include "controller/importer.h"

#include <DMainWindow>
#include <QWidget>
#include <QDebug>

DWIDGET_USE_NAMESPACE

#ifndef LITE_DIV
class Worker : public QObject
{
    Q_OBJECT
public:
    Worker() {}
    ~Worker() {}
public slots:
    void initRec()
    {
        DBManager::instance();
//        Exporter::instance();
        Importer::instance();
        qDebug() << "DBManager time";
    }
};
#endif

class ViewMainWindow : public  DMainWindow
{
    Q_OBJECT
public:
    // If manager is false, the Manager panel(eg.TimelinePanel) will not be
    // initialize to save resource and avoid DB file lock.
    ViewMainWindow(bool manager, QWidget *parent = nullptr);
    void onThemeChanged(ViewerThemeManager::AppTheme theme);
protected:
    void resizeEvent(QResizeEvent *e) override;
private:
    void moveFirstWindow();
    void moveCenter();
    bool windowAtEdge();
    MainWidget *m_mainWidget;
};

#endif // VIEWMAINWINDOW_H

