/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XARCHIVECONSOLE_H
#define XARCHIVECONSOLE_H

#include <QCommandLineOption>
#include <QStringList>

#include "xarchives.h"
#include "xfmodel_header.h"
#include "xfmodel_table.h"
#include "xftree_model.h"
#include "xoptions.h"

class QCommandLineParser;
class QCoreApplication;

// Archive front end for console applications: the listing/extraction/test
// modes, the options that configure them (password, code page, error policy,
// probe budget, member filters) and the record enumeration/rendering they
// need.
//
// This used to live inside XScanEngineConsole, which made every archive
// listing detail reachable only from a scan-engine front end.  None of it
// depends on a scan engine, so it belongs with the archive readers it
// describes; XScanEngineConsole now delegates to this class and applications
// that are archive tools first (XFileUnpacker) compose it directly.
//
// It accepts three command grammars -- the project's own long options, 7-Zip's
// verb form, and Info-ZIP unzip/zipinfo -- all of which parse into one neutral
// COMMAND and run through one set of executors.  See DIALECT for how an
// invocation is assigned to a grammar.
class XArchiveConsole : public QObject {
    Q_OBJECT

public:
    // Which command grammar an invocation is written in.  Selection order:
    //   1. argv[0] basename ("7z"/"7za"/"7zr", "unzip", "zipinfo") -- copying
    //      or symlinking the executable under one of those names gives drop-in
    //      compatibility with no extra typing;
    //   2. an explicit "unzip"/"zipinfo" selector token;
    //   3. a bare 7-Zip verb as the first argument, when no file of that name
    //      exists (a real file always wins, and "--" forces the file reading);
    //   4. otherwise DIALECT_NATIVE, so every existing invocation is unchanged.
    enum DIALECT {
        DIALECT_NATIVE = 0,
        DIALECT_SEVENZIP,
        DIALECT_UNZIP,
        DIALECT_ZIPINFO
    };

    enum VERB {
        VERB_NONE = 0,  // no archive command; the caller keeps its own modes
        VERB_LIST,
        VERB_EXTRACT,
        VERB_TEST,
        VERB_STDOUT,  // write the selected members to standard output
        VERB_FORMATS, // report the container types this build can open
        // Container-agnostic viewers.  They need XFormats only, never a scan
        // engine, so an archive tool can carry them without one.
        VERB_INFO,
        VERB_ENTROPY,
        VERB_STRUCT,
        VERB_SHOWSTRUCTS
    };

    // Output shape for the viewers above (the archive listing has its own
    // richer LISTFORMAT).
    enum RESULTFORMAT {
        RESULTFORMAT_TEXT = 0,
        RESULTFORMAT_XML,
        RESULTFORMAT_JSON,
        RESULTFORMAT_CSV,
        RESULTFORMAT_TSV,
        RESULTFORMAT_PLAINTEXT
    };

    // What to do when a member's output path already exists on disk.
    enum OVERWRITE {
        OVERWRITE_ALWAYS = 0,  // 7-Zip -aoa / -y, unzip -o (the historical default)
        OVERWRITE_SKIP,        // 7-Zip -aos, unzip -n
        OVERWRITE_RENAME       // 7-Zip -aou: extract next to it as name_2, name_3, ...
    };

    enum LISTFORMAT {
        LISTFORMAT_NATIVE = 0,     // this project's aligned table
        LISTFORMAT_TECHNICAL,      // 7-Zip "l -slt" / our --verbose property dump
        LISTFORMAT_UNZIP,          // unzip -l
        LISTFORMAT_UNZIP_VERBOSE,  // unzip -v
        LISTFORMAT_ZIPINFO,        // unzip -Z / zipinfo
        LISTFORMAT_JSON            // our own: the full record property map
    };

    // One invocation, independent of the grammar it was written in.
    struct COMMAND {
        VERB verb;
        DIALECT dialect;
        LISTFORMAT listFormat;
        RESULTFORMAT resultFormat;
        OVERWRITE overwrite;
        QStringList listTargets;   // archives to operate on
        QStringList listIncludes;  // member wildcards to keep (empty = all)
        QStringList listExcludes;  // member wildcards to drop
        QString sOutputDirectory;
        QString sStruct;  // -S/--struct selector, e.g. "Hash" or "Hash#MD5"
        XBinary::FT fileType;
        bool bFlatten;     // 7-Zip "e", unzip -j: drop stored directory components
        bool bVerbose;
        bool bQuiet;
        bool bIgnoreCase;  // unzip -C, 7-Zip -ssc-
        COMMAND();
    };

    explicit XArchiveConsole(QObject *pParent = nullptr);

    // Complete console entry point for applications that are archive tools
    // rather than scan front ends: it owns the QCommandLineParser for the
    // native dialect and dispatches the 7-Zip / Info-ZIP ones.  XFileUnpacker
    // uses this instead of XScanEngineConsole, which drags in a signature
    // engine an unpacker does not need.
    int process(QCoreApplication &app, const QString &sDescription);

    // ---- dialects ---------------------------------------------------------
    static DIALECT detectDialect(const QString &sProgramName, const QStringList &listArguments);
    static bool isSevenZipVerb(const QString &sToken);
    // Entry point for the foreign grammars.  Returns false when the invocation
    // is native and the caller should continue with its own QCommandLineParser;
    // otherwise the command has run and *pnResult holds the exit code.
    bool processForeignDialect(const QStringList &listArguments, qint32 *pnResult);

    // ---- native command line ----------------------------------------------
    // Registers every archive option on pParser.  Call before parser.process().
    // Returns false if any option could not be registered (a duplicate name);
    // the caller must not run with a half-registered command line.
    bool addOptions(QCommandLineParser *pParser);
    // A scan-engine front end owns its own command line and already defines
    // -b/-F/-i/-e/-S/-w and the output-format switches.  Embedded mode
    // therefore registers only the archive commands, and only their long
    // spellings, so a POSIX letter chosen here can never take a letter the
    // host already gave a different meaning.
    void setEmbedded(bool bEmbedded);
    bool isEmbedded() const;
    // Embedded entry point: runs whichever archive command the host's parser
    // saw.  Returns true when one of them consumed the invocation; *pnResult is
    // written only for a non-successful command, so an earlier failure in the
    // host is never masked.
    bool processModes(const QCommandLineParser *pParser, const QStringList &listArgs, XBinary::FT fileType, bool bVerbose, qint32 *pnResult);
    // Reads the archive options into this object.  Returns false (and, when
    // pcrResult is given, the code the process should exit with) if one of
    // them is malformed; the diagnostic is already printed.
    bool applyOptions(const QCommandLineParser *pParser, XOptions::CR *pcrResult);
    // Builds one COMMAND from the parsed native command line, resolving the
    // operation letter, the operands and every modifier.  Returns false on a
    // usage error (two operations, an unusable operand, a malformed value);
    // the diagnostic is already printed.
    bool buildCommand(const QCommandLineParser *pParser, COMMAND *pCommand, XOptions::CR *pcrResult);
    // Runs whichever archive modes the command line selected.  Returns true
    // when at least one of them consumed the invocation; *pnResult is written
    // only for a non-successful mode, so an earlier failure is never masked.

    // ---- executors --------------------------------------------------------
    XOptions::CR execute(const COMMAND &command);
    XOptions::CR listArchives(const QStringList &listFileNames, XBinary::FT fileType, bool bVerbose);
    XOptions::CR listArchives(const COMMAND &command);
    XOptions::CR extractArchives(const QString &sResultDirectory, const QStringList &listFileNames, XBinary::FT fileType, bool bVerbose);
    XOptions::CR extractArchives(const COMMAND &command);
    XOptions::CR testArchives(const COMMAND &command);
    XOptions::CR writeMembersToStdout(const COMMAND &command);
    XOptions::CR listSupportedFormats(const COMMAND &command);
    // XFormats-backed viewers, carried so an archive front end does not have to
    // borrow a scan-engine console for them.
    XOptions::CR showFileInfo(const QString &sFileName, const COMMAND &command);
    XOptions::CR showFileEntropy(const QString &sFileName, const COMMAND &command);
    XOptions::CR showFileStruct(const QString &sFileName, const COMMAND &command);
    XOptions::CR showStructsOverview(const COMMAND &command);

    // ---- state ------------------------------------------------------------
    void setUnpackProperties(const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties);
    QMap<XBinary::UNPACK_PROP, QVariant> getUnpackProperties() const;
    // Automatic format-probe budget per target, in milliseconds; 0 disables it.
    void setProbeTimeout(qint64 nMilliseconds);
    qint64 getProbeTimeout() const;
    // Sticky: set when any target exhausted its probe budget.  The budget
    // covers automatic format probing only, so a front end reports it once for
    // the whole run rather than per file.
    bool isProbeTimeoutOccurred() const;
    void clearProbeTimeoutOccurred();

    // ---- record helpers ---------------------------------------------------
    // Collect full archive records through the streaming unpack API so every
    // property the format parser filled (method, timestamps, CRC, ownership,
    // ...) is available, not just the handful the legacy flat RECORD struct
    // carries.
    static QList<XBinary::ARCHIVERECORD> getRecords(XBinary *pArchive, const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties,
                                                    XBinary::PDSTRUCT *pPdStruct, bool *pbComplete = nullptr);
    // Human-readable, aligned archive listing with a format/size summary line
    // and (when bVerbose) a full per-record property dump.  Only columns the
    // format actually populates are shown, so each archive type surfaces its
    // own metadata.
    static QString formatList(XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize, bool bVerbose);
    // Info-ZIP renderings, so "unzip -l/-v" and "zipinfo" output is familiar to
    // scripts and to people rather than merely equivalent.
    static QString formatListUnzip(const QString &sArchiveName, const QList<XBinary::ARCHIVERECORD> &listRecords, bool bVerbose);
    static QString formatListZipInfo(const QString &sArchiveName, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize);
    // Every property the parser filled, as JSON.  Neither 7-Zip nor Info-ZIP
    // offers a machine-readable listing, so this is strictly ours.
    static QString formatListJson(const QString &sArchiveName, XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize);

    // Member selection shared by every verb and dialect.  An empty include
    // list means "everything"; excludes always win.  Patterns are matched
    // against the stored member name with '/' separators.
    static bool isMemberSelected(const QString &sMemberName, const QStringList &listIncludes, const QStringList &listExcludes, bool bIgnoreCase);
    static QList<XBinary::ARCHIVERECORD> filterRecords(const QList<XBinary::ARCHIVERECORD> &listRecords, const COMMAND &command);

    static QString getRecordName(const XBinary::ARCHIVERECORD &record);
    static bool isRecordFolder(const XBinary::ARCHIVERECORD &record);
    static bool isRecordSizePresent(const XBinary::ARCHIVERECORD &record);
    static qint64 getRecordSize(const XBinary::ARCHIVERECORD &record);
    static QString getRecordModified(const XBinary::ARCHIVERECORD &record);
    static QString getRecordCRC(const XBinary::ARCHIVERECORD &record);
    static QString getRecordAttr(const XBinary::ARCHIVERECORD &record);
    static QString getRecordRatio(const XBinary::ARCHIVERECORD &record);
    static QString getPropertyName(XBinary::FPART_PROP prop);
    static QString getPropertyValueString(const XBinary::ARCHIVERECORD &record, XBinary::FPART_PROP prop);

    // Usage text for the foreign dialects (7-Zip "7z" with no arguments,
    // "unzip -h").  Kept next to the parsers so the two cannot drift.
    static QString getDialectHelp(DIALECT dialect, const QString &sProgramName);

private:
    enum PACKEDSTATE {
        PACKEDSTATE_VALUE = 0,  // the member's own packed size
        PACKEDSTATE_NONE,       // no payload to account for (directory entry)
        PACKEDSTATE_UNKNOWN     // not derivable from this container
    };

    static bool hasAuthoritativeExternalStreamingReader(XBinary::FT fileType);
    static bool isRecordSharingContainerStream(const XBinary::ARCHIVERECORD &record);
    static PACKEDSTATE getRecordPacked(const XBinary::ARCHIVERECORD &record, qint64 *pnPacked);
    static QString getRecordPackedString(const XBinary::ARCHIVERECORD &record);
    static QString getModeString(quint32 nMode, bool bIsFolder);
    static QString getCellValue(const XBinary::ARCHIVERECORD &record, qint32 nColId);

    // Hand parsers for the foreign grammars.  QCommandLineParser cannot express
    // 7-Zip's "-o<dir>"/"-i!<pattern>" attached values nor Info-ZIP's
    // "-opts[modifiers]" clusters, so those two are parsed directly.  Both route
    // any "--word[=value]" token to applyLongOption(), which is why every
    // project-specific option stays available inside a foreign dialect.
    bool parseSevenZip(const QStringList &listArguments, COMMAND *pCommand, XOptions::CR *pcrResult);
    bool parseUnzip(const QStringList &listArguments, DIALECT dialect, COMMAND *pCommand, XOptions::CR *pcrResult);
    bool applyLongOption(const QString &sToken, COMMAND *pCommand, XOptions::CR *pcrResult);
    static bool addOptionChecked(QCommandLineParser *pParser, const QCommandLineOption &option);
    // Same option with every single-character name removed.
    static QCommandLineOption longOnly(const QCommandLineOption &option);
    bool addEmbeddedOptions(QCommandLineParser *pParser);

    // Resolve the type to hand to the archive backends, under the probe
    // budget.  bValidateArchiveType keeps the listing path from accepting a
    // detected type no archive backend can open; the extraction path takes the
    // preferred archive type as-is.
    XBinary::FT detectFileType(QIODevice *pDevice, XBinary::FT fileType, bool bValidateArchiveType, XBinary::PDSTRUCT *pPdStruct);
    // Unpack properties for one command: the parsed base plus the per-command
    // overwrite policy.
    QMap<XBinary::UNPACK_PROP, QVariant> buildUnpackProperties(const COMMAND &command) const;

    // Option set follows the POSIX Utility Syntax Guidelines: single-character
    // options, groupable when they take no argument, option-arguments as
    // separate (or attached) tokens, "--" ends the options, order between
    // options is irrelevant, and repeatable options accumulate.  Operation
    // letters follow POSIX tar -- "-t" is the table of contents, "-x" extracts
    // -- with GNU tar's spellings where POSIX has none (-C, -f, -k, -O, -W).
    //
    // Every long option this tool shipped before is kept as an alias on the
    // option that replaced it, so existing scripts and the test suite keep
    // working unchanged.

    // Operations. At most one per invocation; more is a usage error.
    QCommandLineOption m_clList;           // -t -l --list --listarchive --showarchive
    QCommandLineOption m_clExtract;        // -x --extract
    QCommandLineOption m_clExtractTo;      // --extractarchive <directory> (legacy: -x plus -C)
    QCommandLineOption m_clVerify;         // -W --verify --test --testarchive
    QCommandLineOption m_clToStdout;       // -O --to-stdout --stdout
    QCommandLineOption m_clInfo;           // -i --info
    QCommandLineOption m_clEntropy;        // -e --entropy
    QCommandLineOption m_clStruct;         // -s --struct <name>
    QCommandLineOption m_clStructs;        // -S --structs --showstructs
    // Long-only on purpose: -L sits one shift key from -l/list for a command
    // that is typed once in a while, not in scripts.
    QCommandLineOption m_clFormats;        // --formats --listformats

    // Modifiers.
    QCommandLineOption m_clDirectory;      // -C --directory <directory>
    QCommandLineOption m_clFile;           // -f --file <file>
    QCommandLineOption m_clExclude;        // -X --exclude <pattern>
    QCommandLineOption m_clInclude;        // --include <pattern>
    QCommandLineOption m_clKeep;           // -k --keep-old-files
    QCommandLineOption m_clOverwrite;      // --overwrite <mode>
    QCommandLineOption m_clFlatten;        // -j --flatten
    QCommandLineOption m_clIgnoreCase;     // -I --ignore-case
    QCommandLineOption m_clPassword;       // -P --password <password>
    QCommandLineOption m_clPasswordStdin;  // --password-stdin
    QCommandLineOption m_clPasswordHex;    // -H --password-hex <hex>
    QCommandLineOption m_clCodePage;       // --codepage <number>
    QCommandLineOption m_clProbeTimeout;   // --probe-timeout <milliseconds>
    QCommandLineOption m_clStopOnError;    // --stop-on-error --stoponerror
    QCommandLineOption m_clFileType;       // -F --filetype <type>
    QCommandLineOption m_clFormat;         // -o --format <layout>
    // -v belongs to QCommandLineParser::addVersionOption(); verbosity keeps the
    // -b this project's other consoles already use.
    QCommandLineOption m_clVerbose;        // -b --verbose
    QCommandLineOption m_clQuiet;          // -q --quiet
    QCommandLineOption m_clNoColor;        // -N --no-color --nocolor

    // Pre-POSIX output-format switches, kept as long-only aliases of -o.
    QCommandLineOption m_clAsXml;
    QCommandLineOption m_clAsJson;
    QCommandLineOption m_clAsCsv;
    QCommandLineOption m_clAsTsv;
    QCommandLineOption m_clAsPlainText;

    QMap<XBinary::UNPACK_PROP, QVariant> m_mapUnpackProperties;
    bool m_bEmbedded;
    qint64 m_nProbeTimeout;
    bool m_bProbeTimeoutOccurred;
};

#endif  // XARCHIVECONSOLE_H
