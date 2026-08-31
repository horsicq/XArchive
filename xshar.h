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
#ifndef XSHAR_H
#define XSHAR_H

#include "xarchive.h"

// A reader for classic Bourne-shell archives (shar).  Members are recovered by
// a bounded, deterministic scan of the shell script: each "cat > name <<D" or
// "sed 's/^X//' > name <<D" here-document is exposed as one stored file, and a
// here-document carrying an embedded uuencoded payload is decoded in place.
class XSHAR : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    explicit XSHAR(QIODevice *pDevice = nullptr);
    ~XSHAR();

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QList<QString> getSearchSignatures() override;
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    enum MEMBER_KIND {
        MEMBER_KIND_CAT = 0,
        MEMBER_KIND_SED,
        MEMBER_KIND_UU
    };

    enum TOKEN_TYPE {
        TOKEN_WORD = 0,
        TOKEN_HEREDOC,
        TOKEN_HEREDOC_DASH,
        TOKEN_REDIR_OUT,
        TOKEN_REDIR_APPEND,
        TOKEN_AND,
        TOKEN_SEMI,
        TOKEN_OTHER
    };

    struct SHAR_TOKEN {
        TOKEN_TYPE type;
        QByteArray baRaw;  // exact source bytes for a word (quotes preserved)
    };

    struct SHAR_MEMBER_START {
        bool bIsMember;
        MEMBER_KIND kind;
        QByteArray baStripPrefix;  // bytes removed from each sed body line
        QByteArray baDelim;        // here-document terminator (unquoted)
        bool bDash;                // <<- (strip leading TABs before comparing)
        QByteArray baRawName;      // redirect target as written (quotes preserved)
    };

    struct SHAR_ENTRY {
        MEMBER_KIND kind;
        QString sFileName;
        QByteArray baRawTarget;
        QByteArray baStripPrefix;
        QByteArray baDelim;
        bool bDash;
        bool bUnsafeTarget;
        qint64 nHeaderOffset;   // member-start line offset
        qint64 nHeaderSize;     // member-start line length (incl. newline)
        qint64 nBodyOffset;     // first raw body byte
        qint64 nBodyEndOffset;  // offset of the terminator line (end of raw body)
        qint64 nDecodedSize;    // emitted byte count
        qint64 nDeclaredSize;   // size from "test N -ne wc -c" check, or -1
    };

    struct SHAR_UNPACK_CONTEXT {
        QList<SHAR_ENTRY> listEntries;
        qint32 nCurrentRecord;
    };

    bool _readPhysicalLine(qint64 nOffset, QByteArray *pLine, qint64 *pNextOffset, bool *pHadNewline, bool *pTooLong, PDSTRUCT *pPdStruct);
    bool _findMarker(PDSTRUCT *pPdStruct);
    bool _scanArchive(QList<SHAR_ENTRY> *pEntries, PDSTRUCT *pPdStruct);
    bool _readMemberBody(qint64 nBodyOffset, const QByteArray &baDelim, bool bDash, MEMBER_KIND kind, const QByteArray &baStripPrefix, qint64 nCap,
                         qint64 *pnBodyEndOffset, qint64 *pnNextOffset, qint64 *pnDecodedSize, QByteArray *pTextOutput, QList<QByteArray> *pUuLines,
                         bool *pbTerminated, bool *pbTooLong, bool *pbOversize, PDSTRUCT *pPdStruct);
    bool _decodeMember(const SHAR_ENTRY &entry, qint64 nCap, QByteArray *pOutput, PDSTRUCT *pPdStruct);

    static QList<SHAR_TOKEN> _tokenize(const QByteArray &line);
    static bool _parseMemberStart(const QByteArray &line, SHAR_MEMBER_START *pResult);
    static bool _parseSedProgram(const QByteArray &program, QByteArray *pStripPrefix);
    static QByteArray _unquoteWord(const QByteArray &raw);
    static QByteArray _unquoteDelimiter(const QByteArray &raw);
    static bool _makeSafeName(const QByteArray &rawTarget, QString *pName);
    static bool _parseUuBegin(const QByteArray &line, QString *pName);
    static bool _uudecodeBody(const QList<QByteArray> &listLines, qint64 nCap, QByteArray *pOutput, qint64 *pnSize);
    static bool _matchSizeTest(const QByteArray &line, const QString &sName, qint64 *pnSize);
    static bool _entryMatches(const SHAR_ENTRY &left, const SHAR_ENTRY &right);
    static QByteArray _stripTrailingCR(const QByteArray &line);

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XSHAR_H
