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
#include "baseutils.h"
#include "imageutils.h"
#include "application.h"
#include <stdio.h>
#include <fcntl.h>
#include <fstream>
#include <linux/fs.h>
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
//#include <QDBusInterface>
//#include <QDBusPendingCall>
//#include <QDBusPendingCallWatcher>
#include <QDir>
#include <QFontMetrics>
#include <QFileInfo>
#include <QImage>
#include <QMimeData>
#include <QProcess>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QTextStream>
#include <QtMath>

#include <DApplication>
#include <DDesktopServices>
#include <QImageReader>
#include <QMimeDatabase>

DWIDGET_USE_NAMESPACE

namespace utils {

namespace base {

const QString DATETIME_FORMAT_NORMAL = "yyyy.MM.dd";
const QString DATETIME_FORMAT_EXIF = "yyyy:MM:dd HH:mm:ss";

QString sizeToHuman(const qlonglong bytes)
{
    qlonglong sb = 1024;
    if (bytes < sb) {
        return QString::number(bytes) + " B";
    } else if (bytes < sb * sb) {
        QString vs = QString::number((double)bytes / sb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            return QString::number((int)vs.toDouble()) + " KB";
        } else {
            return vs + " KB";
        }
    } else if (bytes < sb * sb * sb) {
        QString vs = QString::number((double)bytes / sb / sb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            return QString::number((int)vs.toDouble()) + " MB";
        } else {
            return vs + " MB";
        }
    } else {
        return QString::number(bytes);
    }
}

QString timeToString(const QDateTime &time, bool normalFormat)
{
    if (normalFormat)
        return time.toString(DATETIME_FORMAT_NORMAL);
    else
        return time.toString(DATETIME_FORMAT_EXIF);
}

int stringWidth(const QFont &f, const QString &str)
{
    QFontMetrics fm(f);
    return fm.boundingRect(str).width();
}

int stringHeight(const QFont &f, const QString &str)
{
    QFontMetrics fm(f);
    return fm.boundingRect(str).height();
}

QDateTime stringToDateTime(const QString &time)
{
    QDateTime dt = QDateTime::fromString(time, DATETIME_FORMAT_EXIF);
    if (! dt.isValid()) {
        dt = QDateTime::fromString(time, DATETIME_FORMAT_NORMAL);
    }
    return dt;
}

void showInFileManager(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path)) {
        return;
    }
    QString m_Path = static_cast<QString>(path);

    QStringList spc {"#", "&", "@", "!", "?"};
    for (QString c : spc) {
        m_Path.replace(c,  QUrl::toPercentEncoding(c));
    }
    QUrl url = QUrl::fromUserInput(/*"\"" + */m_Path/* + "\""*/);
    url.setPath(m_Path, QUrl::TolerantMode);
    Dtk::Widget::DDesktopServices::showFileItem(url);
}

void copyOneImageToClipboard(const QString &path)
{
    QImage img(path);
    Q_ASSERT(!img.isNull());
    QClipboard *cb = QApplication::clipboard();
    cb->setImage(img, QClipboard::Clipboard);
}

void copyImageToClipboard(const QStringList &paths)
{
    //  Get clipboard
    QClipboard *cb = qApp->clipboard();

    // Ownership of the new data is transferred to the clipboard.
    QMimeData *newMimeData = new QMimeData();

//    // Copy old mimedata
//    const QMimeData* oldMimeData = cb->mimeData();
//    for ( const QString &f : oldMimeData->formats())
//        newMimeData->setData(f, oldMimeData->data(f));

//    // Copy file (gnome)
    QByteArray gnomeFormat = QByteArray("copy\n");
    QString text;
    QList<QUrl> dataUrls;
    for (QString path : paths) {
        if (!path.isEmpty())
            text += path + '\n';
        dataUrls << QUrl::fromLocalFile(path);
        gnomeFormat.append(QUrl::fromLocalFile(path).toEncoded()).append("\n");
    }

    newMimeData->setText(text.endsWith('\n') ? text.left(text.length() - 1) : text);
    newMimeData->setUrls(dataUrls);
    gnomeFormat.remove(gnomeFormat.length() - 1, 1);
    newMimeData->setData("x-special/gnome-copied-files", gnomeFormat);

    // Copy Image Date
    QImage img(paths.first());
    Q_ASSERT(!img.isNull());
    newMimeData->setImageData(img);

    // Set the mimedata
    cb->setMimeData(newMimeData, QClipboard::Clipboard);
}

QString getFileContent(const QString &file)
{
    QFile f(file);
    QString fileContent = "";
    if (f.open(QFile::ReadOnly)) {
        fileContent = QLatin1String(f.readAll());
        f.close();
    }
    return fileContent;
}

bool writeTextFile(QString filePath, QString content)
{
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in << content << endl;
        file.close();
        return true;
    }

    return false;
}

bool trashFile(const QString &file)
{
#ifdef QT_GUI_LIB
    QString trashPath;
    QString trashInfoPath;
    QString trashFilesPath;

    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    // There maby others location for trash like $HOME/.trash or
    // $XDG_DATA_HOME/Trash, but our stupid FileManager coder said we should
    // assume that the trash lcation is $HOME/.local/share/Trash,so...
    trashPath = home + "/.local/share/Trash";
    trashInfoPath = trashPath + "/info";
    trashFilesPath = trashPath + "/files";
    if (! QDir(trashFilesPath).exists()) {
        QDir().mkpath(trashFilesPath);
    }
    if (! QDir(trashInfoPath).exists()) {
        QDir().mkpath(trashInfoPath);
    }

    QFileInfo originalInfo(file);
    if (! originalInfo.exists()) {
        qWarning() << "File doesn't exists, can't move to trash";
        return false;
    }
    // Info for restore
    QString infoStr;
    infoStr += "[Trash Info]\nPath=";
    infoStr += QString(originalInfo.absoluteFilePath().toUtf8().toPercentEncoding("/"));
    infoStr += "\nDeletionDate=";
    infoStr += QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss.zzzZ");
    infoStr += "\n";

    QString trashname = originalInfo.fileName();
    QString infopath = trashInfoPath + "/" + trashname + ".trashinfo";
    QString filepath = trashFilesPath + "/" + trashname;
    int nr = 1;
    while (QFileInfo(infopath).exists() || QFileInfo(filepath).exists()) {
        nr++;
        trashname = originalInfo.baseName() + "." + QString::number(nr);
        if (!originalInfo.completeSuffix().isEmpty()) {
            trashname += QString(".") + originalInfo.completeSuffix();
        }
        infopath = trashInfoPath + "/" + trashname + ".trashinfo";
        filepath = trashFilesPath + "/" + trashname;
    }
    QFile infoFile(infopath);
    if (infoFile.open(QIODevice::WriteOnly)) {
        infoFile.write(infoStr.toUtf8());
        infoFile.close();

        if (!QDir().rename(originalInfo.absoluteFilePath(), filepath)) {
            qWarning() << "move to trash failed!";
            return false;
        }
    } else {
        return false;
    }
    // Remove thumbnail
    utils::image::removeThumbnail(file);
    return true;
#else
    Q_UNUSED(file);
    qWarning() << "Trash in server-mode not supported";
    return false;
#endif
}

bool trashFiles(const QStringList &files)
{
    bool v = true;
    for (QString file : files) {
        if (! trashFile(file))
            v = false;
    }

    return v;
}

/*!
 * \brief wrapStr
 * Split info string by Space
 * \param str
 * \param font
 * \param maxWidth
 * \return
 */
QString wrapStr(const QString &str, const QFont &font, int maxWidth)
{
    QFontMetrics fm(font);
    QString ns;
    QString ss;
    for (int i = 0; i < str.length(); i ++) {
        if (/*str.at(i).isSpace()||*/ fm.boundingRect(ss).width() > maxWidth) {
            ss = QString();
            ns += "\n";
        }
        ns += str.at(i);
        ss += str.at(i);
    }
    return ns;
}


QString SpliteText(const QString &text, const QFont &font, int nLabelSize)
{
//    QFontMetrics fm(font);
//    int nTextSize = fm.width(text);
//    if (nTextSize > nLabelSize) {
//        int nPos = 0;
//        long nOffset = 0;
//        for (int i = 0; i < text.size(); i++) {
//            nOffset += fm.width(text.at(i));
//            if (nOffset >= nLabelSize) {
//                nPos = i;
//                break;
//            }
//        }

//        nPos = (nPos - 1 < 0) ? 0 : nPos - 1;

//        QString qstrLeftData = text.left(nPos);
//        QString qstrMidData = text.mid(nPos);
//        return qstrLeftData + "\n" + SpliteText(qstrMidData, font, nLabelSize);
//    }
//    return text;
    //      递归有风险
//LMH0424，之前是递归，现在改了算法，判断换行
    QFontMetrics fm(font);
    double dobuleTextSize = fm.width(text);
    double dobuleLabelSize = nLabelSize;
    if (dobuleTextSize > dobuleLabelSize && dobuleLabelSize > 0 && dobuleTextSize < 10000) {
        double splitCount = dobuleTextSize / dobuleLabelSize;
        int nCount = int(splitCount + 1);
        QString textSplite;
        QString textTotal = text;
        for (int index = 0; index < nCount; ++index) {
            int nPos = 0;
            long nOffset = 0;
            for (int i = 0; i < text.size(); i++) {
                nOffset += fm.width(text.at(i));
                if (nOffset >= nLabelSize) {
                    nPos = i;
                    break;
                }
            }
            nPos = (nPos - 1 < 0) ? 0 : nPos - 1;
            QString qstrLeftData;
            if (nCount - 1 == index) {
                qstrLeftData = textTotal;
                textSplite += qstrLeftData;
            } else {
                qstrLeftData = textTotal.left(nPos);
                textSplite += qstrLeftData + "\n";
            }
            textTotal = textTotal.mid(nPos);
        }
        return textSplite;
    } else {
        return text;
    }
}


QString symFilePath(const QString &path)
{
    QFileInfo fileInfo(path);
    if (fileInfo.isSymLink()) {
        return fileInfo.symLinkTarget();
    } else {
        return path;
    }
}

QString hash(const QString &str)
{
    return QString(QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex());
}

bool onMountDevice(const QString &path)
{
    return (path.startsWith("/media/") || path.startsWith("/run/media/"));
}

bool mountDeviceExist(const QString &path)
{
    QString mountPoint;
    if (path.startsWith("/media/")) {
        const int sp = path.indexOf("/", 7) + 1;
        const int ep = path.indexOf("/", sp) + 1;
        mountPoint = path.mid(0, ep);
    } else if (path.startsWith("/run/media/")) {
        const int sp = path.indexOf("/", 11) + 1;
        const int ep = path.indexOf("/", sp) + 1;
        mountPoint = path.mid(0, ep);
    }
    return QFileInfo(mountPoint).exists();
}
bool isCommandExist(const QString &command)
{
    QProcess *proc = new QProcess;
    QString cm = QString("which %1\n").arg(command);
    proc->start(cm);
    proc->waitForFinished(1000);
    if (proc->exitCode() == 0) {
        return true;
    } else {
        return false;
    }
}

bool checkMimeData(const QMimeData *mimeData)
{
    if (!mimeData->hasUrls()) {
        return false;
    }
    QList<QUrl> urlList = mimeData->urls();
    if (1 > urlList.size()) {
        return false;
    }
    using namespace utils::image;
    for (QUrl url : urlList) {
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isDir()) {
            auto finfos =  getImagesInfo(path, false);
            for (auto finfo : finfos) {
                if (imageSupportRead(finfo.absoluteFilePath())) {
                    QFileInfo info(finfo.absoluteFilePath());
                    QMimeDatabase db;
                    QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
                    QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
                    QString str = info.suffix().toLower();
                    if (str.isEmpty()) {
                        if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
                            if (utils::image::supportedImageFormats().contains("*." + str, Qt::CaseInsensitive)) {
                                return true;
                            } else if (str.isEmpty()) {
                                return true;
                            }
                        }
                    } else {
                        if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
                            if (utils::image::supportedImageFormats().contains("*." + str, Qt::CaseInsensitive)) {
                                return true;
                            }
                        }
                    }
                }
            }
        } else if (imageSupportRead(path)) {
//            paths << path;
            QFileInfo info(path);
            QMimeDatabase db;
            QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
            QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
            QString str = info.suffix().toLower();
            if (str.isEmpty()) {
                if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
                    if (utils::image::supportedImageFormats().contains("*." + str, Qt::CaseInsensitive)) {
                        return true;
                    } else if (str.isEmpty()) {
                        return true;
                    }
                }
            } else {
                if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
                    if (utils::image::supportedImageFormats().contains("*." + str, Qt::CaseInsensitive)) {
                        return true;
                    }
                }
            }
            return false;
        }
    }
    return false;
}


QPixmap renderSVG(const QString &filePath, const QSize &size)
{
    QImageReader reader;
    QPixmap pixmap;
    reader.setFileName(filePath);
    if (reader.canRead()) {
        const qreal ratio = dApp->devicePixelRatio();
        reader.setScaledSize(size * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
    } else {
        pixmap.load(filePath);
    }

    return pixmap;
}

QString mkMutiDir(const QString path)   //创建多级目录
{
    QDir dir(path);
    if ( dir.exists(path)) {
        return path;
    }
    QString parentDir = mkMutiDir(path.mid(0, path.lastIndexOf('/')));
    QString dirname = path.mid(path.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if ( !dirname.isEmpty() )
        parentPath.mkpath(dirname);
    return parentDir + "/" + dirname;
}

}  // namespace base

}  // namespace utils
