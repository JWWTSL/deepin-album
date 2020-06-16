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
#include "slideeffectplayer.h"
#include "application.h"
#include "controller/configsetter.h"
#include <QDebug>
#include <QFileInfo>
#include <QPainter>
#include <QTimerEvent>
#include <QDesktopWidget>
#include <QGuiApplication>
#include <QScreen>
#include <DIconButton>

namespace {

const QString DURATION_SETTING_GROUP = "SLIDESHOWDURATION";
const QString DURATION_SETTING_KEY = "Duration";
const int ANIMATION_DURATION  = 1000;
const int SLIDER_DURATION  = 3000;
const int ANIMATION_DURATION_4K  = 2300;
const int SLIDER_DURATION_4K  = 7000;
//const int ANIMATION_DURATION  = 2500;
//const int SLIDER_DURATION  = 7000;

} // namespace

SlideEffectPlayer::SlideEffectPlayer(QObject *parent)
    : QObject(parent)
{
//    QDesktopWidget *desktopWidget = QApplication::desktop();
//    m_screenrect = desktopWidget->screenGeometry();
    QScreen *screen = QGuiApplication::primaryScreen ();
    QRect m_screenrect = QRect(0, 0, 0, 0);
    qreal m_ratio = 1;
    m_screenrect = screen->availableGeometry() ;
    m_ratio = screen->devicePixelRatio();
    if ((((qreal)m_screenrect.width())*m_ratio) > 3000 || (((qreal)m_screenrect.height())*m_ratio) > 3000) {
        b_4k = true;
    }
    connect(dApp->signalM, &SignalManager::updateButton, this, [ = ] {
        killTimer(m_tid);
        m_tid = 0;
    });
    connect(dApp->signalM, &SignalManager::sigStartTimer, this, [ = ] {
        if (!b_4k)
            m_tid = startTimer(SLIDER_DURATION);
        else
            m_tid = startTimer(SLIDER_DURATION_4K);
    });

}

SlideEffectPlayer::~SlideEffectPlayer()
{
    if (m_thread.isRunning()) {
        m_thread.quit();
    }

    if (m_effect) {
        delete m_effect;
        m_effect = nullptr;
    }
}

void SlideEffectPlayer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != m_tid || m_pausing)
        return;
//    if(m_current == m_paths.size()-1){
//        emit dApp->signalM->updateButton();
//        emit dApp->signalM->updatePauseButton();
//    }

    if (bneedupdatepausebutton) {
        emit dApp->signalM->updateButton();
        emit dApp->signalM->updatePauseButton();
        bneedupdatepausebutton = false;
        return;
    }
    if (! startNext()) {
        stop();
    }
    if (bfirstrun) {
        killTimer(m_tid);
        bfirstrun = false;
        if (!b_4k)
            m_tid = startTimer(SLIDER_DURATION);
        else
            m_tid = startTimer(SLIDER_DURATION_4K);
    }
}

int SlideEffectPlayer::duration() const
{
    return dApp->setter->value(DURATION_SETTING_GROUP,
                               DURATION_SETTING_KEY).toInt() * 1000;
}

void SlideEffectPlayer::setFrameSize(int width, int height)
{
    m_w = width;
    m_h = height;
}

void SlideEffectPlayer::setImagePaths(const QStringList &paths)
{
    m_paths = paths;
    m_current = 0;
}

void SlideEffectPlayer::setCurrentImage(const QString &path)
{
    //lmh0427优化判断位置
    m_current = m_paths.indexOf(path);
}


int SlideEffectPlayer::currentImageIndex() const
{
    return m_current;
}

QString SlideEffectPlayer::currentImagePath() const
{
    return m_paths[m_current];
}

bool SlideEffectPlayer::isRunning() const
{
    return m_running;
}

void SlideEffectPlayer::start()
{
    if (m_paths.isEmpty())
        return;

    bfirstrun = true;
    cacheNext();
    cachePrevious();
    m_running = true;
    if (!b_4k)
        m_tid = startTimer(ANIMATION_DURATION );
    else
        m_tid = startTimer(ANIMATION_DURATION_4K );
}

void SlideEffectPlayer::pause()
{
    if (m_effect) {
        m_pausing = !m_pausing;
        m_effect->pause();
    }
}

bool SlideEffectPlayer::startNext()
{
    if (m_paths.isEmpty())
        return false;
    QSize fSize(m_w, m_h);
    if (! fSize.isValid()) {
        qWarning() << "Invalid frame size!";
        return false;
    }

    if (1 == m_paths.length()) {
        return false;
    }

    int current = m_current + 1;
    if (current == m_paths.length()) {
        current = 0;
    }

    if (m_cacheImages.value(m_paths[current]).isNull()) {
        //return false;
        //lmh0427，如果还未加载，采用主线程去调用
        QImage img = utils::image::getRotatedImage(m_paths[current]);
        m_cacheImages[m_paths[current]] = img;
    }

    if (nullptr != m_effect) {
        m_effect->deleteLater();
        m_effect = nullptr;
    }

    const QString oldPath = m_paths[m_current];

    //LMH0428处理逻辑与上一张变更，防止切换上一张的时候，从下一张图片切回本张图片
    QString newPath;
    if (m_current >= m_paths.length() - 1) {
        newPath = m_paths[0];
    } else {
        newPath = m_paths[m_current + 1];
    }

    m_effect = SlideEffect::create("");
    m_effect->moveToThread(&m_thread);
//    m_effect = SlideEffect::create("enter_from_right");
//    if ((m_screenrect.width()*m_ratio) < 3000 && (m_screenrect.height()*m_ratio) < 3000) {
    if (!b_4k) {
        m_effect->setDuration(ANIMATION_DURATION);
        m_effect->setAllMs(SLIDER_DURATION);
    } else {
//        qDebug() << "------------------4K";
        m_effect->setDuration(ANIMATION_DURATION_4K);
        m_effect->setAllMs(SLIDER_DURATION_4K);
    }
    m_effect->setSize(fSize);

    using namespace utils::image;

    QImage oldImg = m_cacheImages.value(oldPath);
    QImage newImg = m_cacheImages.value(newPath);

// The "newPath" would be the next "oldPath", so there is no need to remove it now
    m_cacheImages.remove(oldPath);

//    qDebug() << m_cacheImages;
    m_effect->setImages(oldImg, newImg);

    if (!m_thread.isRunning()) {
        m_thread.start();
    }

    connect(m_effect, &SlideEffect::frameReady, this, [ = ] (const QImage & img) {
        if (m_running) {
            Q_EMIT frameReady(img);
        }
    }/*, Qt::DirectConnection*/);
    //LMH0428下一张数量的增加，幻灯片处理完了才新增
    connect(m_effect, &SlideEffect::stopped, this, [ = ]  {
        if (m_paths.length() > 1)
        {
            m_current++;
            if (m_current == m_paths.length()) {
                m_current = 0;
            }
        }
        cacheNext();
    }/*, Qt::DirectConnection*/);

    QMetaObject::invokeMethod(m_effect, "start");

    return true;
}

bool SlideEffectPlayer::startPrevious()
{
    if (m_paths.isEmpty()) {
        return false;
    }

    QSize fSize(m_w, m_h);
    if (! fSize.isValid()) {
        return false;
    }

    if (1 == m_paths.length()) {
        return false;
    }

    int current = m_current - 1;
    if (current == -1) {
        current = m_paths.length() - 1;
    }

    if (m_cacheImages.value(m_paths[current]).isNull()) {
        //return false;
        //lmh0427，如果还未加载，采用主线程去调用
        QImage img = utils::image::getRotatedImage(m_paths[current]);
        m_cacheImages[m_paths[current]] = img;
    }

    if (nullptr != m_effect) {
        m_effect->deleteLater();
        m_effect = nullptr;
    }

    const QString oldPath = m_paths[m_current];

    QString newPath;
    if (m_current <= 0) {
        newPath = m_paths[m_paths.length() - 1];
    } else {
        newPath = m_paths[m_current - 1];
    }

    m_effect = SlideEffect::create("enter_from_left");
    m_effect->moveToThread(&m_thread);

    if (!b_4k) {
        m_effect->setDuration(ANIMATION_DURATION);
        m_effect->setAllMs(SLIDER_DURATION);
    } else {
        qDebug() << "------------------4K";
        m_effect->setDuration(ANIMATION_DURATION_4K);
        m_effect->setAllMs(SLIDER_DURATION_4K);
    }
    m_effect->setSize(fSize);

    using namespace utils::image;
    qDebug() << "m_cacheImages.value";
    QImage oldImg = m_cacheImages.value(oldPath);
    QImage newImg = m_cacheImages.value(newPath);
    // The "newPath" would be the next "oldPath", so there is no need to remove it now
    m_cacheImages.remove(oldPath);

    m_effect->setImages(oldImg, newImg);
    if (!m_thread.isRunning()) {
        m_thread.start();
    }
    connect(m_effect, &SlideEffect::frameReady, this, [ = ] (const QImage & img) {
        if (m_running) {
            Q_EMIT frameReady(img);
        }
    }/*, Qt::DirectConnection*/);

    connect(m_effect, &SlideEffect::stopped, this, [ = ]  {
        if (m_paths.length() > 1)
        {
            m_current --;
            if (m_current == -1) {
                m_current = m_paths.length() - 1;
            }
        }
        cachePrevious();
    }/*, Qt::DirectConnection*/);

    QMetaObject::invokeMethod(m_effect, "start");
    return true;
}

void SlideEffectPlayer::cacheNext()
{
    int current = m_current;
    current ++;
    if (current == m_paths.length()) {
        if (bfirstrun) {
            //current = m_paths.length() - 1;
            //bneedupdatepausebutton = true;
            current = 0;
        } else {
            current = 0;
        }
    }

    QString path = m_paths[current];

    if (m_cacheImages.value(path).isNull()) {
        CacheThread *t = new CacheThread(path);
        connect(t, &CacheThread::cached,
        this, [ = ] (const QString path, const QImage img) {
            qDebug() << "m_cacheImages  next: " << path;
            m_cacheImages.insert(path, img);
        });
        connect(t, &CacheThread::finished, t, &CacheThread::deleteLater);
        t->start();
    }
}

void SlideEffectPlayer::cachePrevious()
{
    int current = m_current;
    current--;
    if (-1 == current) {
        current = m_paths.length() - 1;
    }

    QString path = m_paths[current];

    if (m_cacheImages.value(path).isNull()) {
        CacheThread *t = new CacheThread(path);
        connect(t, &CacheThread::cached,
        this, [ = ] (const QString path, const QImage img) {
            qDebug() << "m_cacheImages previous: " << path;
            m_cacheImages.insert(path, img);
        });
        connect(t, &CacheThread::finished, t, &CacheThread::deleteLater);
        t->start();
    }
}

void SlideEffectPlayer::stop()
{
    if (!isRunning())
        return;
    if (m_effect) {
        m_effect->deleteLater();
        m_effect = nullptr;
    }
    if (m_thread.isRunning()) {
        m_thread.quit();
        m_thread.wait();
//        m_thread.start();
    }

    killTimer(m_tid);
    m_tid = 0;
    m_running = false;
    m_cacheImages.clear();

    Q_EMIT finished();
}

