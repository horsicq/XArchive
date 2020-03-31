// copyright (c) 2017-2020 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#ifndef XCAB_H
#define XCAB_H

#include <QObject>
#include "xarchive.h"

class XCab : public XArchive
{
    Q_OBJECT

public:
#pragma pack(push)
#pragma pack(1)
    struct CFHEADER
    {
      quint8 signature[4];      // Cabinet file signature
      quint32 reserved1;        // reserved
      quint32 cbCabinet;        // size of this cabinet file in bytes
      quint32 reserved2;        // reserved
      quint32 coffFiles;        // offset of the first CFFILE entry
      quint32 reserved3;        // reserved
      quint8 versionMinor;      // cabinet file format version, minor
      quint8 versionMajor;      // cabinet file format version, major
      quint16 cFolders;         // number of CFFOLDER entries in this cabinet */
      quint16 cFiles;           // number of CFFILE entries in this cabinet */
      quint16 flags;            // cabinet file option indicators */
      quint16 setID;            // must be the same for all cabinets in a set */
      quint16 iCabinet;         // number of this cabinet file in a set */
//      u2  cbCFHeader;       /* (optional) size of per-cabinet reserved area */
//      u1  cbCFFolder;       /* (optional) size of per-folder reserved area */
//      u1  cbCFData;         /* (optional) size of per-datablock reserved area */
//      u1  abReserve[];      /* (optional) per-cabinet reserved area */
//      u1  szCabinetPrev[];  /* (optional) name of previous cabinet file */
//      u1  szDiskPrev[];     /* (optional) name of previous disk */
//      u1  szCabinetNext[];  /* (optional) name of next cabinet file */
//      u1  szDiskNext[];     /* (optional) name of next disk */
    };
#pragma pack(pop)

    explicit XCab(QIODevice *__pDevice=0);
    virtual bool isValid();
    virtual QString getVersion();
    virtual quint64 getNumberOfRecords();
    virtual QList<RECORD> getRecords(qint32 nLimit=-1);

    CFHEADER readHeader();
};

#endif // XCAB_H
