/* Copyright (c) 2017-2024 hors<horsicq@gmail.com>
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
#include "xtar.h"

XTAR::XTAR(QIODevice *pDevice) : XArchive(pDevice)
{

}

bool XTAR::isValid(PDSTRUCT *pPdStruct)
{
    return false;
}

bool XTAR::isValid(QIODevice *pDevice)
{
    XTAR xtar(pDevice);

    return xtar.isValid();
}

quint64 XTAR::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return 0;
}

QList<XArchive::RECORD> XTAR::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    return listResult;
}

QString XTAR::getFileFormatExt()
{
    return "tar";
}

QList<XBinary::MAPMODE> XTAR::getMapModesList(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XTAR::posix_header XTAR::read_posix_header(qint64 nOffset)
{
    posix_header record = {};

    read_array(nOffset + offsetof(posix_header, name), record.name, sizeof(record.name));
    read_array(nOffset + offsetof(posix_header, mode), record.mode, sizeof(record.mode));
    read_array(nOffset + offsetof(posix_header, uid), record.uid, sizeof(record.uid));
    read_array(nOffset + offsetof(posix_header, gid), record.gid, sizeof(record.gid));
    read_array(nOffset + offsetof(posix_header, size), record.size, sizeof(record.size));
    read_array(nOffset + offsetof(posix_header, mtime), record.mtime, sizeof(record.mtime));
    read_array(nOffset + offsetof(posix_header, chksum), record.chksum, sizeof(record.chksum));
    read_array(nOffset + offsetof(posix_header, typeflag), record.typeflag, sizeof(record.typeflag));
    read_array(nOffset + offsetof(posix_header, linkname), record.linkname, sizeof(record.linkname));
    read_array(nOffset + offsetof(posix_header, magic), record.magic, sizeof(record.magic));
    read_array(nOffset + offsetof(posix_header, version), record.version, sizeof(record.version));
    read_array(nOffset + offsetof(posix_header, uname), record.uname, sizeof(record.uname));
    read_array(nOffset + offsetof(posix_header, gname), record.gname, sizeof(record.gname));
    read_array(nOffset + offsetof(posix_header, devmajor), record.devmajor, sizeof(record.devmajor));
    read_array(nOffset + offsetof(posix_header, devminor), record.devminor, sizeof(record.devminor));
    read_array(nOffset + offsetof(posix_header, prefix), record.prefix, sizeof(record.prefix));

    return record;
}
