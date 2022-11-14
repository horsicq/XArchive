/* Copyright (c) 2017-2022 hors<horsicq@gmail.com>
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
#include "xrar.h"

XRar::XRar(QIODevice *pDevice) : XArchive(pDevice) {
}

bool XRar::isValid() {
    bool bResult = false;

    if (getSize() > 20)  // TODO
    {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

        if (compareSignature(&memoryMap, "'RE~^'") || compareSignature(&memoryMap, "'Rar!'1A07")) {
            bResult = true;
        }
    }

    return bResult;
}

bool XRar::isValid(QIODevice *pDevice) {
    XRar xrar(pDevice);

    return xrar.isValid();
}

QString XRar::getVersion() {
    QString sResult;

    _MEMORY_MAP memoryMap = getMemoryMap();

    // TODO more
    if (compareSignature(&memoryMap, "'RE~^'")) {
        sResult = "1.4";
    } else if (compareSignature(&memoryMap, "'Rar!'1A0700")) {
        sResult = "4.X";
    } else if (compareSignature(&memoryMap, "'Rar!'1A070100")) {
        sResult = "5.X";
    }

    return sResult;
}

quint64 XRar::getNumberOfRecords(PDSTRUCT *pPdStruct) {
    return 0;  // TODO
}

QList<XArchive::RECORD> XRar::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) {
    Q_UNUSED(nLimit)

    QList<XArchive::RECORD> listResult;

    // TODO

    return listResult;
}

QString XRar::getFileFormatExt() {
    return "rar";
}

qint64 XRar::getFileFormatSize() {
    return XBinary::getFileFormatSize();
}

QString XRar::getFileFormatString() {
    QString sResult;

    sResult = QString("RAR(%1)").arg(getVersion());
    // TODO more info

    return sResult;
}
