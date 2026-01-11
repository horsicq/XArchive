/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "xapks.h"

XAPKS::XAPKS(QIODevice *pDevice) : XAPK(pDevice)
{
}

XAPKS::~XAPKS()
{
}

bool XAPKS::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XAPKS xapks(getDevice());

    if (xapks.XAPK::isValid(pPdStruct)) {
        QList<XArchive::RECORD> listArchiveRecords = xapks.getRecords(10, pPdStruct);
        bResult = isValid(&listArchiveRecords, pPdStruct);
    }

    return bResult;
}

bool XAPKS::isValid(QIODevice *pDevice)
{
    XAPKS xapks(pDevice);

    return xapks.isValid();
}

bool XAPKS::isValid(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // // APKS is an APK with additional metadata, check for BundleConfig.pb file
    // bool bHasBundleConfig = false;
    // bool bHasSignatures = false;

    // if (pListRecords) {
    //     for (qint32 i = 0; i < pListRecords->count(); i++) {
    //         QString sFileName = pListRecords->at(i).spInfo.sRecordName;

    //         if (sFileName == "BundleConfig.pb") {
    //             bHasBundleConfig = true;
    //         }

    //         if (sFileName == "META-INF/MANIFEST.MF" || sFileName.startsWith("META-INF/") && sFileName.endsWith(".SF")) {
    //             bHasSignatures = true;
    //         }
    //     }
    // }

    // // APKS format requires bundle configuration and signatures
    // return (bHasBundleConfig && bHasSignatures);

    return false;  // TODO
}

QString XAPKS::getFileFormatExt()
{
    return "apks";
}

QString XAPKS::getMIMEString()
{
    return "application/vnd.android.package-archive";
}

XBinary::FT XAPKS::getFileType()
{
    return FT_APKS;
}
