/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xcompanionfile.h"

#include <QDir>
#include <QFileInfo>

#include "xbinary.h"

QString XCompanionFile::sourcePath(QIODevice *pDevice)
{
    const QString sPath = XBinary::getDeviceFileName(pDevice);
    if (sPath.isEmpty()) return QString();

    const QFileInfo fileInfo(sPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) return QString();

    return fileInfo.absoluteFilePath();
}

QString XCompanionFile::sourceDirectory(QIODevice *pDevice)
{
    const QString sPath = sourcePath(pDevice);
    if (sPath.isEmpty()) return QString();

    return QFileInfo(sPath).absolutePath();
}

bool XCompanionFile::isBareName(const QString &sName)
{
    if (sName.isEmpty()) return false;
    if ((sName == QLatin1String(".")) || (sName == QLatin1String(".."))) return false;
    if (sName.contains(QLatin1Char('/')) || sName.contains(QLatin1Char('\\'))) return false;
    if (sName.contains(QLatin1Char(':'))) return false;

    return true;
}

QString XCompanionFile::resolveInAncestors(QIODevice *pDevice, const QString &sRelativePath, qint32 nMaxLevels)
{
    const QString sDirectory = sourceDirectory(pDevice);
    if (sDirectory.isEmpty()) return QString();

    return resolveInAncestorsOf(sDirectory, sRelativePath, nMaxLevels);
}

QString XCompanionFile::resolveInAncestorsOf(const QString &sDirectory, const QString &sRelativePath, qint32 nMaxLevels)
{
    if (sDirectory.isEmpty() || (nMaxLevels < 0)) return QString();

    const QStringList listParts = sRelativePath.split(QLatin1Char('/'));
    const qint32 nParts = listParts.size();
    if (nParts < 1) return QString();
    for (qint32 i = 0; i < nParts; i++) {
        if (!isBareName(listParts.at(i))) return QString();
    }

    QDir directory(sDirectory);
    if (!directory.exists()) return QString();

    for (qint32 nLevel = 0; nLevel <= nMaxLevels; nLevel++) {
        QString sCurrent = directory.absolutePath();
        bool bFound = true;
        for (qint32 i = 0; (i < nParts - 1) && bFound; i++) {
            const QString sWanted = listParts.at(i).toCaseFolded();
            const QDir current(sCurrent);
            const QFileInfo exactInfo(current.filePath(listParts.at(i)));
            if (exactInfo.exists() && exactInfo.isDir()) {
                sCurrent = exactInfo.absoluteFilePath();
                continue;
            }
            bFound = false;
            const QFileInfoList listDirs = current.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            const qint32 nCount = listDirs.size();
            for (qint32 j = 0; j < nCount; j++) {
                if (listDirs.at(j).fileName().toCaseFolded() == sWanted) {
                    sCurrent = listDirs.at(j).absoluteFilePath();
                    bFound = true;
                    break;
                }
            }
        }
        if (bFound) {
            const QString sResult = resolveInDirectory(sCurrent, listParts.at(nParts - 1));
            if (!sResult.isEmpty()) return sResult;
        }
        const QString sBefore = directory.absolutePath();
        if (!directory.cdUp() || (directory.absolutePath() == sBefore)) break;
    }

    return QString();
}

QString XCompanionFile::resolve(QIODevice *pDevice, const QString &sName)
{
    const QString sDirectory = sourceDirectory(pDevice);
    if (sDirectory.isEmpty()) return QString();

    return resolveInDirectory(sDirectory, sName);
}

QString XCompanionFile::resolveInDirectory(const QString &sDirectory, const QString &sName)
{
    if (sDirectory.isEmpty() || !isBareName(sName)) return QString();

    QDir directory(sDirectory);
    if (!directory.exists()) return QString();

    const QFileInfo exactInfo(directory.filePath(sName));
    if (exactInfo.exists() && exactInfo.isFile() && exactInfo.isReadable()) return exactInfo.absoluteFilePath();

    const QString sWanted = sName.toCaseFolded();
    const QFileInfoList listFiles = directory.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name);
    const qint32 nCount = listFiles.size();

    for (qint32 i = 0; i < nCount; i++) {
        if (listFiles.at(i).fileName().toCaseFolded() == sWanted) return listFiles.at(i).absoluteFilePath();
    }

    return QString();
}
