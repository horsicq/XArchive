/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xjetbbs.h"

#include <QPointer>

XJETBBS::XJETBBS(QIODevice *pDevice) : XLHA(pDevice)
{
}

// Only the three tags U3's FUN_0055a390 accepts, and only onto decoders that
// already exist.  Anything else fails closed rather than being decoded on an
// assumption.
XBinary::HANDLE_METHOD XJETBBS::_jetbbsMethodToHandle(const QString &sMethod)
{
    if (sMethod == QLatin1String("-mg0-")) return HANDLE_METHOD_STORE;
    if (sMethod == QLatin1String("-mg4-")) return HANDLE_METHOD_LZH4;
    if (sMethod == QLatin1String("-mg5-")) return HANDLE_METHOD_LZH5;
    return HANDLE_METHOD_UNKNOWN;
}

// "-mg" then one of 0/4/5 then '-'.  U3 tests the method character with the
// bitmask 0x31, i.e. exactly {0, 4, 5}.
bool XJETBBS::_isMemberTag(const QByteArray &baHeader)
{
    if (baHeader.size() < 21) return false;
    if (baHeader.mid(2, 3) != QByteArray("-mg", 3)) return false;
    if (baHeader.at(6) != '-') return false;
    const char cMethod = baHeader.at(5);
    return (cMethod == '0') || (cMethod == '4') || (cMethod == '5');
}

bool XJETBBS::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    // Same contract as XLHA::isValid: the caller still owns this device, so its
    // cursor is snapshotted and restored around the probe.
    QPointer<XJETBBS> guardedArchive(this);
    QIODevice *pSourceDevice = getDevice();
    const qint64 nSavedPos = pSourceDevice ? pSourceDevice->pos() : -1;

    // Smallest possible archive: a level 0/1 base header plus the one-byte
    // end-of-archive marker.
    if (XBinary::isPdStructNotCanceled(pPdStruct) && (getSize() >= 24)) {
        const QByteArray baPrefix = read_array(0, 22);
        if (guardedArchive && (baPrefix.size() == 22) && _isMemberTag(baPrefix)) {
            const quint8 nHeaderSize = static_cast<quint8>(baPrefix.at(0));
            const quint8 nLevel = static_cast<quint8>(baPrefix.at(20));
            // Levels 2 and 3 replace the checksum byte with part of a wider
            // header-size field, so the checksum test below does not apply
            // there.  No such JetBBS archive has been seen; refuse rather than
            // guess, which is also what keeps a two-character tag safe.
            if ((nHeaderSize >= 21) && (nLevel <= 1)) {
                const QByteArray baHeader =
                    read_array(0, 2 + static_cast<qint64>(nHeaderSize));
                if (guardedArchive &&
                    (baHeader.size() == (2 + static_cast<qint64>(nHeaderSize))) &&
                    _isHeaderChecksumValid(baHeader)) {
                    // And the header must actually parse as a member.
                    LHA_MEMBER member = {};
                    bResult = _readMember(0, &member, pPdStruct);
                }
            }
        }
    }

    if (pSourceDevice && (nSavedPos >= 0)) {
        pSourceDevice->seek(nSavedPos);
    }

    return guardedArchive && bResult;
}

bool XJETBBS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XJETBBS archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XJETBBS::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XJETBBS(pDevice);
}

QList<QString> XJETBBS::getSearchSignatures()
{
    QList<QString> listResult;
    // Bytes 0 and 1 are a length and a checksum; byte 5 is the method digit.
    listResult.append("....'-mg'..2D");
    return listResult;
}

XBinary::FT XJETBBS::getFileType()
{
    return FT_JETBBS;
}

QString XJETBBS::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XJETBBS::getFileFormatExtsString()
{
    return QStringLiteral("JetBBS archive (*.dat)");
}

QString XJETBBS::getMIMEString()
{
    return QStringLiteral("application/x-lzh-compressed");
}

QString XJETBBS::getVersion()
{
    // The method characters, matching how XLHA and XSAR report their tag.
    return read_ansiString(3, 3);
}

XBinary::ARCHIVERECORD XJETBBS::infoCurrent(UNPACK_STATE *pState,
                                            PDSTRUCT *pPdStruct)
{
    // The base does the guarding, the source-ownership check and the whole
    // member parse; only the method mapping is re-spelled here.  Adding a
    // second UNPACK_OPERATION_GUARD around this call would deadlock the nested
    // form, so there is deliberately none.
    ARCHIVERECORD result = XLHA::infoCurrent(pState, pPdStruct);
    if (result.mapProperties.isEmpty()) return result;
    if (static_cast<HANDLE_METHOD>(
            result.mapProperties.value(FPART_PROP_HANDLEMETHOD).toUInt()) !=
        HANDLE_METHOD_UNKNOWN) {
        return result;
    }

    QPointer<XJETBBS> guardedArchive(this);
    LHA_MEMBER member = {};
    if (!pState || !_readMember(pState->nCurrentOffset, &member, pPdStruct) ||
        !guardedArchive) {
        return result;
    }
    // A symbolic link has no payload to decode; the base deliberately leaves it
    // unknown and that must survive the re-spelling.
    if (member.bSymbolicLink) return result;
    const HANDLE_METHOD handleMethod = _jetbbsMethodToHandle(member.sMethod);
    if (handleMethod != HANDLE_METHOD_UNKNOWN) {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethod);
    }
    return result;
}

QList<XBinary::FPART> XJETBBS::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult = XLHA::getFileParts(nFileParts, nLimit, pPdStruct);
    if (!(nFileParts & FILEPART_STREAM) || listResult.isEmpty()) {
        return listResult;
    }

    // Re-walk the members in the same order the base does so a stream part can
    // be matched to the member it came from.  _isMemberTag rejects the "-pms-"
    // PMA SFX envelope, so the walk always starts at offset 0 here.
    QPointer<XJETBBS> guardedArchive(this);
    QList<LHA_MEMBER> listMembers;
    qint64 nOffset = 0;
    const qint64 nFileSize = getSize();
    while (guardedArchive && (nOffset < nFileSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        LHA_MEMBER member = {};
        if (!_readMember(nOffset, &member, pPdStruct) || !guardedArchive) break;
        if (member.nRecordSize <= 0) break;
        listMembers.append(member);
        nOffset += member.nRecordSize;
        if (listMembers.size() > 1000000) break;
    }
    if (!guardedArchive) return listResult;

    qint32 nMemberIndex = 0;
    for (qint32 i = 0; i < listResult.size(); ++i) {
        if (listResult.at(i).filePart != FILEPART_STREAM) continue;
        if (nMemberIndex >= listMembers.size()) break;
        const LHA_MEMBER &member = listMembers.at(nMemberIndex);
        ++nMemberIndex;
        if (member.bSymbolicLink) continue;
        if (static_cast<HANDLE_METHOD>(
                listResult.at(i).mapProperties.value(FPART_PROP_HANDLEMETHOD)
                    .toUInt()) != HANDLE_METHOD_UNKNOWN) {
            continue;
        }
        const HANDLE_METHOD handleMethod = _jetbbsMethodToHandle(member.sMethod);
        if (handleMethod != HANDLE_METHOD_UNKNOWN) {
            listResult[i].mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                               handleMethod);
        }
    }
    return listResult;
}
