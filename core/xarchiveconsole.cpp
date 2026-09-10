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
#include "xarchiveconsole.h"
#include "xlegacyconsole.h"

#include <QBuffer>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QSet>
#include <QScopedPointer>
#include <QVector>
#include <QXmlStreamWriter>

#include <algorithm>
#include <cstdio>

#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif

XArchiveConsole::COMMAND::COMMAND()
    : verb(VERB_NONE),
      dialect(DIALECT_NATIVE),
      listFormat(LISTFORMAT_NATIVE),
      resultFormat(RESULTFORMAT_TEXT),
      overwrite(OVERWRITE_ALWAYS),
      packMethod(PACKMETHOD_DEFLATE),
      fileType(XBinary::FT_UNKNOWN),
      bFlatten(false),
      bVerbose(false),
      bQuiet(false),
      bIgnoreCase(false)
{
}

XArchiveConsole::XArchiveConsole(QObject *pParent)
    : QObject(pParent),
      // ---- operations (POSIX tar letters where they exist) ----
      m_clList(QStringList() << "l" << "list" << "listarchive" << "showarchive", "List archive contents."),
      m_clExtract(QStringList() << "x" << "extract", "Extract archive members."),
      m_clExtractTo(QStringList() << "extractarchive", "Extract all archive entries to <directory>.", "directory"),
      m_clVerify(QStringList() << "W" << "verify" << "test" << "testarchive", "Verify every archive member without writing files."),
      m_clToStdout(QStringList() << "O" << "to-stdout" << "stdout", "Write the selected members to standard output."),
      m_clInfo(QStringList() << "i" << "info", "Show file information; with no file, list the supported formats."),
      m_clEntropy(QStringList() << "e" << "entropy", "Show file entropy."),
      m_clStruct(QStringList() << "s" << "struct", "Show one named structure, e.g. 'Hash' or 'Hash#MD5'.", "name"),
      m_clStructs(QStringList() << "S" << "structs" << "showstructs", "Show every available structure."),
      m_clFormats(QStringList() << "formats" << "listformats", "List the container formats this build can open."),
      m_clCreate(QStringList() << "c" << "create", "Create an archive from the files that follow (ZIP only)."),
      m_clMethod(QStringList() << "method", "Compression for -c: deflate (default) or store.", "method", "deflate"),
      // ---- modifiers ----
      m_clDirectory(QStringList() << "C" << "o" << "directory", "Extract into <directory>.", "directory"),
      m_clManifest(QStringList() << "manifest",
                   "Write a JSON manifest of every extracted record, with all of its properties, to <file>.", "file"),
      m_clFile(QStringList() << "f" << "file", "Archive to operate on; repeatable, and operands work too.", "file"),
      m_clInclude(QStringList() << "include", "Keep only members matching <pattern>; repeatable. Operands do the same.", "pattern"),
      m_clKeep(QStringList() << "k" << "keep-old-files", "Keep existing destination files instead of replacing them."),
      m_clOverwrite(QStringList() << "overwrite", "Existing destination files: always (default), skip, or rename.", "mode", "always"),
      m_clFlatten(QStringList() << "j" << "flatten", "Drop stored directory components when extracting."),
      m_clIgnoreCase(QStringList() << "I" << "ignore-case", "Match member patterns case-insensitively."),
      m_clPassword(QStringList() << "P" << "password", "Archive password.", "password"),
      m_clPasswordStdin(QStringList() << "password-stdin", "Read the archive password as one UTF-8 line from standard input."),
      m_clPasswordHex(QStringList() << "H" << "password-hex", "Exact legacy archive password bytes as hexadecimal.", "hex"),
      m_clCodePage(QStringList() << "codepage", "Windows code page for legacy archive filenames and password bytes.", "number"),
      m_clProbeTimeout(QStringList() << "probe-timeout", "Maximum automatic archive-probe time per target in milliseconds (0 disables).", "milliseconds", "20000"),
      m_clMaxOutputSize(QStringList() << "max-output-size", "Maximum uncompressed bytes per member.", "bytes"),
      m_clMaxTotalOutputSize(QStringList() << "max-total-output-size", "Maximum aggregate uncompressed bytes per operation.", "bytes"),
      m_clMaxEntryCount(QStringList() << "max-entry-count", "Maximum archive member count.", "count"),
      m_clMaxMemoryOutputSize(QStringList() << "max-memory-output-size", "Maximum bytes for in-memory decoded output.", "bytes"),
      m_clFilesystem(QStringList() << "filesystem", "Read supported guest filesystem files inside virtual disk images."),
      m_clTransportOnly(QStringList() << "transport-only", "Decode UU/base64 payloads without opening the nested archive."),
      m_clStopOnError(QStringList() << "stop-on-error" << "stoponerror", "Abort extraction and roll the destination back when a member fails."),
      m_clFileType(QStringList() << "F" << "filetype", "Force the container type (e.g. PE, ELF, ZIP).", "type"),
      m_clFormat(QStringList() << "format", "Output format: text (default), json, xml, csv, or tsv.", "layout"),
      m_clVerbose(QStringList() << "b" << "verbose", "Show verbose output with detailed information."),
      m_clQuiet(QStringList() << "q" << "quiet", "Suppress progress and summary lines."),
      m_clNoColor(QStringList() << "N" << "no-color" << "nocolor", "Disable colour output."),
      // ---- pre-POSIX output switches, kept as long-only aliases of -o ----
      m_clAsXml(QStringList() << "xml", "Output results in XML format."),
      m_clAsJson(QStringList() << "json", "Output results in JSON format."),
      m_clAsCsv(QStringList() << "csv", "Output results in CSV format."),
      m_clAsTsv(QStringList() << "tsv", "Output results in TSV format."),
      m_clAsPlainText(QStringList() << "plaintext", "Output results as plain text."),
      m_listFormat(LISTFORMAT_NATIVE),
      m_resultFormat(RESULTFORMAT_TEXT),
      m_bListFormatSet(false),
      m_bResultFormatSet(false),
      m_nProbeTimeout(20000),
      m_bProbeTimeoutOccurred(false)
{
    // Best-effort extraction is the console default (like mainstream archive
    // tools): damaged or partially present archives yield every recoverable
    // member, and the skip tally is reported after the run.  --stop-on-error
    // restores the strict all-or-nothing transaction.
    m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CONTINUEONERROR, true);
}

bool XArchiveConsole::addOptions(QCommandLineParser *pParser)
{
    bool bAllRegistered = true;

    bAllRegistered = addOptionChecked(pParser, m_clList) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clExtract) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clExtractTo) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clVerify) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clToStdout) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clInfo) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clEntropy) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clStruct) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clStructs) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFormats) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clCreate) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clMethod) && bAllRegistered;

    bAllRegistered = addOptionChecked(pParser, m_clDirectory) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clManifest) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFile) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clInclude) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clKeep) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clOverwrite) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFlatten) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clIgnoreCase) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clPassword) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clPasswordStdin) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clPasswordHex) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clCodePage) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clProbeTimeout) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clMaxOutputSize) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clMaxTotalOutputSize) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clMaxEntryCount) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clMaxMemoryOutputSize) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFilesystem) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clTransportOnly) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clStopOnError) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFileType) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clFormat) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clVerbose) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clQuiet) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clNoColor) && bAllRegistered;

    bAllRegistered = addOptionChecked(pParser, m_clAsXml) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsJson) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsCsv) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsTsv) && bAllRegistered;
    bAllRegistered = addOptionChecked(pParser, m_clAsPlainText) && bAllRegistered;

    return bAllRegistered;
}

// QCommandLineParser::addOption() returns false and drops the option when a
// name is already taken, which loses a switch silently -- that is how -v was
// lost to addVersionOption() and --verbose stopped existing.  Refuse to run
// with a half-registered command line instead.
bool XArchiveConsole::addOptionChecked(QCommandLineParser *pParser, const QCommandLineOption &option)
{
    if (pParser->addOption(option)) {
        return true;
    }

    printf("Internal error: duplicate command line option '%s'\n", option.names().join(QChar('/')).toUtf8().data());

    return false;
}

bool XArchiveConsole::applyOutputLimit(const QString &sName, const QString &sValue, XOptions::CR *pcrResult)
{
    bool bValid = !sValue.isEmpty();
    for (const QChar c : sValue) {
        if ((c < QLatin1Char('0')) || (c > QLatin1Char('9'))) bValid = false;
    }
    bool bConverted = false;
    const qint64 nLimit = sValue.toLongLong(&bConverted);
    if (!bValid || !bConverted || nLimit < 0) {
        printf("Error: --%s requires a non-negative integer\n", sName.toUtf8().constData());
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }
    XBinary::UNPACK_PROP key = XBinary::UNPACK_PROP_UNKNOWN;
    if (sName == QLatin1String("max-output-size")) key = XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE;
    else if (sName == QLatin1String("max-total-output-size")) key = XBinary::UNPACK_PROP_MAX_TOTAL_OUTPUT_SIZE;
    else if (sName == QLatin1String("max-entry-count")) key = XBinary::UNPACK_PROP_MAX_ENTRY_COUNT;
    else if (sName == QLatin1String("max-memory-output-size")) key = XBinary::UNPACK_PROP_MAX_MEMORY_OUTPUT_SIZE;
    if (key == XBinary::UNPACK_PROP_UNKNOWN) {
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }
    m_mapUnpackProperties.insert(key, nLimit);
    return true;
}

bool XArchiveConsole::applyOptions(const QCommandLineParser *pParser, XOptions::CR *pcrResult)
{
    bool bProbeTimeoutValid = false;
    const qint64 nProbeTimeoutMs = pParser->value(m_clProbeTimeout).toLongLong(&bProbeTimeoutValid);

    if (!bProbeTimeoutValid || (nProbeTimeoutMs < 0)) {
        printf("Error: --probe-timeout requires a non-negative number of milliseconds\n");
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    m_nProbeTimeout = nProbeTimeoutMs;

    m_mapUnpackProperties.clear();
    if (pParser->isSet(m_clFilesystem)) m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_DISK_FILESYSTEM, true);
    if (pParser->isSet(m_clTransportOnly)) m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_TRANSPORT_ONLY, true);

    if (pParser->isSet(m_clMaxOutputSize) && !applyOutputLimit(QStringLiteral("max-output-size"), pParser->value(m_clMaxOutputSize), pcrResult)) return false;
    if (pParser->isSet(m_clMaxTotalOutputSize) && !applyOutputLimit(QStringLiteral("max-total-output-size"), pParser->value(m_clMaxTotalOutputSize), pcrResult)) return false;
    if (pParser->isSet(m_clMaxEntryCount) && !applyOutputLimit(QStringLiteral("max-entry-count"), pParser->value(m_clMaxEntryCount), pcrResult)) return false;
    if (pParser->isSet(m_clMaxMemoryOutputSize) && !applyOutputLimit(QStringLiteral("max-memory-output-size"), pParser->value(m_clMaxMemoryOutputSize), pcrResult)) return false;

    if (!pParser->isSet(m_clStopOnError)) {
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CONTINUEONERROR, true);
    }

    QString sArchivePassword;
    const qint32 nPasswordOptions = qint32(pParser->isSet(m_clPassword)) + qint32(pParser->isSet(m_clPasswordStdin)) + qint32(pParser->isSet(m_clPasswordHex));

    if (nPasswordOptions > 1) {
        printf("Error: use only one of -P/--password, --password-stdin, or -H/--password-hex\n");
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    if (pParser->isSet(m_clPasswordStdin)) {
        QFile passwordInput;

        if (!passwordInput.open(stdin, QIODevice::ReadOnly, QFileDevice::DontCloseHandle)) {
            printf("Error: cannot read archive password from standard input\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        QByteArray baPassword = passwordInput.readLine(1024 * 1024);

        if (!baPassword.endsWith('\n') && !passwordInput.atEnd()) {
            printf("Error: archive password is too long\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        if (baPassword.endsWith('\n')) baPassword.chop(1);
        if (baPassword.endsWith('\r')) baPassword.chop(1);

        sArchivePassword = QString::fromUtf8(baPassword);
    } else if (pParser->isSet(m_clPassword)) {
        sArchivePassword = pParser->value(m_clPassword);
    }

    if (!sArchivePassword.isEmpty()) {
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, sArchivePassword);
    }

    if (pParser->isSet(m_clPasswordHex)) {
        const QByteArray baHex = pParser->value(m_clPasswordHex).toLatin1();
        bool bHexValid = !baHex.isEmpty() && ((baHex.size() & 1) == 0) && (baHex.size() <= 2 * 1024 * 1024);

        for (char ch : baHex) {
            if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
                bHexValid = false;
                break;
            }
        }

        if (!bHexValid) {
            printf("Error: --password-hex requires an even, non-empty hexadecimal byte string\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD_BYTES, QByteArray::fromHex(baHex));
    }

    if (pParser->isSet(m_clCodePage)) {
        bool bCodePageValid = false;
        const quint32 nCodePage = pParser->value(m_clCodePage).toUInt(&bCodePageValid);

        if (!bCodePageValid || (nCodePage == 0)) {
            printf("Error: --codepage requires a non-zero numeric Windows code page\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CODEPAGE, nCodePage);
    }

    return true;
}

QString XArchiveConsole::getFormatValues()
{
    return QStringLiteral("text (default), json, xml, csv, or tsv");
}

bool XArchiveConsole::setFormat(const QString &sFormat)
{
    // Five encodings, nothing else. The technical rendering is a layout rather
    // than an encoding, so --verbose selects it instead of a --format value.
    if (sFormat == QLatin1String("text")) {
        m_resultFormat = RESULTFORMAT_TEXT;
        m_listFormat = LISTFORMAT_NATIVE;
    } else if (sFormat == QLatin1String("json")) {
        m_resultFormat = RESULTFORMAT_JSON;
        m_listFormat = LISTFORMAT_JSON;
    } else if (sFormat == QLatin1String("xml")) {
        m_resultFormat = RESULTFORMAT_XML;
        m_listFormat = LISTFORMAT_XML;
    } else if (sFormat == QLatin1String("csv")) {
        m_resultFormat = RESULTFORMAT_CSV;
        m_listFormat = LISTFORMAT_CSV;
    } else if (sFormat == QLatin1String("tsv")) {
        m_resultFormat = RESULTFORMAT_TSV;
        m_listFormat = LISTFORMAT_TSV;
    } else {
        return false;
    }

    m_bListFormatSet = true;
    m_bResultFormatSet = true;

    return true;
}

bool XArchiveConsole::buildCommand(const QCommandLineParser *pParser, COMMAND *pCommand, XOptions::CR *pcrResult)
{
    pCommand->dialect = DIALECT_NATIVE;
    pCommand->bVerbose = pParser->isSet(m_clVerbose);
    pCommand->bQuiet = pParser->isSet(m_clQuiet);
    pCommand->bFlatten = pParser->isSet(m_clFlatten);
    pCommand->bIgnoreCase = pParser->isSet(m_clIgnoreCase);
    pCommand->sStruct = pParser->value(m_clStruct);
    pCommand->fileType = pParser->isSet(m_clFileType) ? XBinary::ftStringToFileTypeId(pParser->value(m_clFileType)) : XBinary::FT_UNKNOWN;

    // Operands: the first is the archive, the rest select members -- the POSIX
    // tar shape ("tar -x -f a.tar member ..."), which unzip shares.  -f adds
    // archives explicitly, so operands are then all member patterns.
    QStringList listOperands = pParser->positionalArguments();
    pCommand->listTargets = pParser->values(m_clFile);

    for (const QString &sOperand : listOperands) {
        // POSIX guideline 13 gives "-" the meaning of standard input.  Format
        // probing needs to seek, so say that plainly instead of failing later
        // with a confusing "cannot open".
        if (sOperand == QLatin1String("-")) {
            printf("Error: reading the archive from standard input is not supported; give a file name\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        if (pCommand->listTargets.isEmpty()) {
            pCommand->listTargets.append(sOperand);
        } else {
            pCommand->listIncludes.append(sOperand);
        }
    }

    pCommand->listIncludes += pParser->values(m_clInclude);

    if (pParser->isSet(m_clDirectory)) {
        pCommand->sOutputDirectory = pParser->value(m_clDirectory);
    }

    if (pParser->isSet(m_clExtractTo)) {
        pCommand->sOutputDirectory = pParser->value(m_clExtractTo);
    }

    pCommand->sManifest = pParser->value(m_clManifest);

    const QString sMethod = pParser->value(m_clMethod);

    if (sMethod == QLatin1String("store")) pCommand->packMethod = PACKMETHOD_STORE;
    else if (sMethod == QLatin1String("deflate")) pCommand->packMethod = PACKMETHOD_DEFLATE;
    else {
        printf("Error: --method requires deflate or store\n");
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    // Overwrite policy: -k is the shorthand, --overwrite the explicit form.
    const QString sOverwrite = pParser->value(m_clOverwrite);

    if (pParser->isSet(m_clOverwrite)) {
        if (sOverwrite == QLatin1String("always")) pCommand->overwrite = OVERWRITE_ALWAYS;
        else if (sOverwrite == QLatin1String("skip")) pCommand->overwrite = OVERWRITE_SKIP;
        else if (sOverwrite == QLatin1String("rename")) pCommand->overwrite = OVERWRITE_RENAME;
        else {
            printf("Error: --overwrite requires always, skip, or rename\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
    }

    if (pParser->isSet(m_clKeep)) {
        pCommand->overwrite = OVERWRITE_SKIP;
    }

    // --format carries the five output encodings; the one-switch-per-format
    // spellings below are aliases for the same setter.
    if (pParser->isSet(m_clFormat) && !setFormat(pParser->value(m_clFormat))) {
        printf("Error: --format requires %s\n", getFormatValues().toUtf8().data());
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    // The pre-POSIX one-switch-per-format spellings feed the same setter.
    QString sAliasFormat;

    if (pParser->isSet(m_clAsXml)) sAliasFormat = QStringLiteral("xml");
    else if (pParser->isSet(m_clAsJson)) sAliasFormat = QStringLiteral("json");
    else if (pParser->isSet(m_clAsCsv)) sAliasFormat = QStringLiteral("csv");
    else if (pParser->isSet(m_clAsTsv)) sAliasFormat = QStringLiteral("tsv");
    else if (pParser->isSet(m_clAsPlainText)) sAliasFormat = QStringLiteral("text");

    if (!pParser->isSet(m_clFormat) && !sAliasFormat.isEmpty()) {
        setFormat(sAliasFormat);
    }

    if (m_bListFormatSet) pCommand->listFormat = m_listFormat;
    if (m_bResultFormatSet) pCommand->resultFormat = m_resultFormat;

    if (!m_bListFormatSet && pCommand->bVerbose) {
        pCommand->listFormat = LISTFORMAT_TECHNICAL;
    }

    // Resolve the operation.  POSIX leaves the order of options free, so the
    // operation is decided by which letter is present, never by position; two
    // of them is a usage error rather than a silent precedence rule.
    struct OPERATION {
        const QCommandLineOption *pOption;
        VERB verb;
        const char *pszName;
    };

    const OPERATION operations[] = {
        {&m_clList, VERB_LIST, "-l/--list"},
        {&m_clExtract, VERB_EXTRACT, "-x/--extract"},
        {&m_clExtractTo, VERB_EXTRACT, "--extractarchive"},
        {&m_clVerify, VERB_TEST, "-W/--verify"},
        {&m_clToStdout, VERB_STDOUT, "-O/--to-stdout"},
        {&m_clInfo, VERB_INFO, "-i/--info"},
        {&m_clEntropy, VERB_ENTROPY, "-e/--entropy"},
        {&m_clStruct, VERB_STRUCT, "-s/--struct"},
        {&m_clStructs, VERB_SHOWSTRUCTS, "-S/--structs"},
        {&m_clFormats, VERB_FORMATS, "--formats"},
        {&m_clCreate, VERB_CREATE, "-c/--create"},
    };

    const qint32 nNumberOfOperations = qint32(sizeof(operations) / sizeof(operations[0]));
    QString sSelectedName;

    for (qint32 i = 0; i < nNumberOfOperations; i++) {
        if (!pParser->isSet(*(operations[i].pOption))) {
            continue;
        }

        // -x and --extractarchive are the same operation spelled two ways, so
        // naming both is not a conflict.
        if ((pCommand->verb != VERB_NONE) && (pCommand->verb != operations[i].verb)) {
            printf("Error: %s and %s select different operations; use one\n", sSelectedName.toUtf8().data(), operations[i].pszName);
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }

        pCommand->verb = operations[i].verb;
        sSelectedName = QString(operations[i].pszName);
    }

    // No operation letter: identify the operand, which is the useful answer
    // for a bare target in a tool whose job is opening unknown containers.
    if ((pCommand->verb == VERB_NONE) && !pCommand->listTargets.isEmpty()) {
        pCommand->verb = VERB_INFO;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Dialects
// ---------------------------------------------------------------------------

bool XArchiveConsole::isSevenZipVerb(const QString &sToken)
{
    return (sToken == QLatin1String("l")) || (sToken == QLatin1String("x")) || (sToken == QLatin1String("e")) || (sToken == QLatin1String("t")) ||
           (sToken == QLatin1String("i"));
}

XArchiveConsole::DIALECT XArchiveConsole::detectDialect(const QString &sProgramName, const QStringList &listArguments)
{
    const QString sName = QFileInfo(sProgramName).completeBaseName().toLower();

    if ((sName == QLatin1String("7z")) || (sName == QLatin1String("7za")) || (sName == QLatin1String("7zr"))) {
        return DIALECT_SEVENZIP;
    }

    if (!listArguments.isEmpty()) {
        const QString sFirst = listArguments.at(0);

        // A real file always wins over a verb.  "l" and "x" are legal file
        // names, and opening one must not turn into a command because of where
        // it happens to sit on the line.  "--" forces the file reading.
        if (isSevenZipVerb(sFirst) && !QFileInfo::exists(sFirst)) {
            return DIALECT_SEVENZIP;
        }
    }

    return DIALECT_NATIVE;
}

bool XArchiveConsole::processForeignDialect(const QStringList &listArguments, qint32 *pnResult)
{
    if (listArguments.isEmpty()) {
        return false;
    }

    const QString sProgramName = listArguments.at(0);
    QStringList listTokens = listArguments.mid(1);
    const DIALECT dialect = detectDialect(sProgramName, listTokens);

    if (dialect == DIALECT_NATIVE) {
        return false;
    }

    COMMAND command;
    command.dialect = dialect;

    XOptions::CR crResult = XOptions::CR_SUCCESS;
    bool bParsed = false;

    bParsed = parseSevenZip(listTokens, &command, &crResult);

    if (!bParsed) {
        if (pnResult) *pnResult = crResult;
        return true;
    }

    if (command.verb == VERB_NONE) {
        printf("%s", getDialectHelp(dialect, sProgramName).toUtf8().data());
        if (pnResult) *pnResult = XOptions::CR_SUCCESS;
        return true;
    }

    const XOptions::CR crExecute = execute(command);

    if (pnResult) {
        if (crExecute != XOptions::CR_SUCCESS) {
            *pnResult = crExecute;
        } else if (m_bProbeTimeoutOccurred) {
            *pnResult = XOptions::CR_PROBETIMEOUT;
        } else {
            *pnResult = XOptions::CR_SUCCESS;
        }
    }

    return true;
}

// Options this project adds on top of the foreign grammars.  Neither 7-Zip nor
// Info-ZIP uses GNU-style long options -- 7-Zip spells its stop marker as a
// bare "--" -- so the whole "--word" namespace is free in every dialect.
bool XArchiveConsole::applyLongOption(const QString &sToken, COMMAND *pCommand, XOptions::CR *pcrResult)
{
    QString sName = sToken.mid(2);
    QString sValue;
    const qint32 nEqual = sName.indexOf(QChar('='));

    if (nEqual >= 0) {
        sValue = sName.mid(nEqual + 1);
        sName = sName.left(nEqual);
    }

    if (sName == QLatin1String("verbose")) {
        pCommand->bVerbose = true;
    } else if (sName == QLatin1String("quiet")) {
        pCommand->bQuiet = true;
    } else if (sName == QLatin1String("nocolor")) {
        XOptions::setNoColor(true);
    } else if (sName == QLatin1String("flatten")) {
        pCommand->bFlatten = true;
    } else if (sName == QLatin1String("ignore-case")) {
        pCommand->bIgnoreCase = true;
    } else if (sName == QLatin1String("filesystem")) {
        if (nEqual >= 0) {
            printf("Error: --filesystem does not take a value\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_DISK_FILESYSTEM, true);
    } else if ((sName == QLatin1String("stoponerror")) || (sName == QLatin1String("stop-on-error"))) {
        m_mapUnpackProperties.remove(XBinary::UNPACK_PROP_CONTINUEONERROR);
    } else if (sName == QLatin1String("json")) {
        pCommand->listFormat = LISTFORMAT_JSON;
    } else if (sName == QLatin1String("include")) {
        if (!sValue.isEmpty()) pCommand->listIncludes.append(sValue);
    } else if (sName == QLatin1String("exclude")) {
        if (!sValue.isEmpty()) pCommand->listExcludes.append(sValue);
    } else if (sName == QLatin1String("outdir")) {
        pCommand->sOutputDirectory = sValue;
    } else if (sName == QLatin1String("manifest")) {
        pCommand->sManifest = sValue;
    } else if (sName == QLatin1String("password")) {
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, sValue);
    } else if (sName == QLatin1String("codepage")) {
        bool bValid = false;
        const quint32 nCodePage = sValue.toUInt(&bValid);
        if (!bValid || (nCodePage == 0)) {
            printf("Error: --codepage requires a non-zero numeric Windows code page\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_CODEPAGE, nCodePage);
    } else if (sName == QLatin1String("password-hex")) {
        const QByteArray baHex = sValue.toLatin1();
        bool bHexValid = !baHex.isEmpty() && ((baHex.size() & 1) == 0) && (baHex.size() <= 2 * 1024 * 1024);
        for (char ch : baHex) {
            if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
                bHexValid = false;
                break;
            }
        }
        if (!bHexValid) {
            printf("Error: --password-hex requires an even, non-empty hexadecimal byte string\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD_BYTES, QByteArray::fromHex(baHex));
    } else if (sName == QLatin1String("probe-timeout")) {
        bool bValid = false;
        const qint64 nTimeout = sValue.toLongLong(&bValid);
        if (!bValid || (nTimeout < 0)) {
            printf("Error: --probe-timeout requires a non-negative number of milliseconds\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        m_nProbeTimeout = nTimeout;
    } else if (sName == QLatin1String("max-output-size") ||
               sName == QLatin1String("max-total-output-size") ||
               sName == QLatin1String("max-entry-count") ||
               sName == QLatin1String("max-memory-output-size")) {
        return applyOutputLimit(sName, sValue, pcrResult);
    } else if (sName == QLatin1String("filetype")) {
        pCommand->fileType = XBinary::ftStringToFileTypeId(sValue);
    } else if (sName == QLatin1String("overwrite")) {
        if (sValue == QLatin1String("always")) pCommand->overwrite = OVERWRITE_ALWAYS;
        else if (sValue == QLatin1String("skip")) pCommand->overwrite = OVERWRITE_SKIP;
        else if (sValue == QLatin1String("rename")) pCommand->overwrite = OVERWRITE_RENAME;
        else {
            printf("Error: --overwrite requires always, skip, or rename\n");
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
    } else if (sName == QLatin1String("format")) {
        if (!setFormat(sValue)) {
            printf("Error: --format requires %s\n", getFormatValues().toUtf8().data());
            if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
            return false;
        }
        pCommand->listFormat = m_listFormat;
        pCommand->resultFormat = m_resultFormat;
    } else if (sName == QLatin1String("help")) {
        pCommand->verb = VERB_NONE;
    } else {
        printf("Error: unknown option --%s\n", sName.toUtf8().data());
        if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
        return false;
    }

    return true;
}

bool XArchiveConsole::parseSevenZip(const QStringList &listArguments, COMMAND *pCommand, XOptions::CR *pcrResult)
{
    bool bStopSwitches = false;
    bool bVerbSeen = false;
    bool bStdoutRequested = false;

    for (qint32 i = 0; i < listArguments.count(); i++) {
        const QString sToken = listArguments.at(i);

        if (!bStopSwitches && (sToken == QLatin1String("--"))) {
            bStopSwitches = true;
            continue;
        }

        if (!bStopSwitches && sToken.startsWith(QLatin1String("--"))) {
            if (!applyLongOption(sToken, pCommand, pcrResult)) {
                return false;
            }
            continue;
        }

        if (!bStopSwitches && sToken.startsWith(QChar('-')) && (sToken.length() > 1)) {
            const QString sSwitch = sToken.mid(1);

            if (sSwitch.startsWith(QLatin1String("o"))) {
                pCommand->sOutputDirectory = sSwitch.mid(1);
            } else if (sSwitch.startsWith(QLatin1String("p"))) {
                m_mapUnpackProperties.insert(XBinary::UNPACK_PROP_PASSWORD, sSwitch.mid(1));
            } else if (sSwitch == QLatin1String("y")) {
                pCommand->overwrite = OVERWRITE_ALWAYS;
            } else if (sSwitch.startsWith(QLatin1String("ao"))) {
                const QString sMode = sSwitch.mid(2);
                if (sMode == QLatin1String("a")) pCommand->overwrite = OVERWRITE_ALWAYS;
                else if (sMode == QLatin1String("s")) pCommand->overwrite = OVERWRITE_SKIP;
                else if ((sMode == QLatin1String("u")) || (sMode == QLatin1String("t"))) pCommand->overwrite = OVERWRITE_RENAME;
                else {
                    printf("Error: -ao requires a, s, t, or u\n");
                    if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                    return false;
                }
            } else if (sSwitch == QLatin1String("so")) {
                bStdoutRequested = true;
            } else if (sSwitch == QLatin1String("slt")) {
                pCommand->listFormat = LISTFORMAT_TECHNICAL;
            } else if (sSwitch.startsWith(QLatin1String("i!")) || sSwitch.startsWith(QLatin1String("ir!"))) {
                pCommand->listIncludes.append(sSwitch.mid(sSwitch.indexOf(QChar('!')) + 1));
            } else if (sSwitch.startsWith(QLatin1String("x!")) || sSwitch.startsWith(QLatin1String("xr!"))) {
                pCommand->listExcludes.append(sSwitch.mid(sSwitch.indexOf(QChar('!')) + 1));
            } else if (sSwitch == QLatin1String("ssc-")) {
                pCommand->bIgnoreCase = true;
            } else if (sSwitch == QLatin1String("ssc")) {
                pCommand->bIgnoreCase = false;
            } else if (sSwitch.startsWith(QLatin1String("t"))) {
                pCommand->fileType = XBinary::ftStringToFileTypeId(sSwitch.mid(1));
            } else if (sSwitch.startsWith(QLatin1String("bso0")) || sSwitch.startsWith(QLatin1String("bsp0")) || (sSwitch == QLatin1String("bd"))) {
                pCommand->bQuiet = true;
            } else if (sSwitch.startsWith(QLatin1String("b")) || sSwitch.startsWith(QLatin1String("r")) || sSwitch.startsWith(QLatin1String("m")) ||
                       sSwitch.startsWith(QLatin1String("sn")) || sSwitch.startsWith(QLatin1String("sp")) || sSwitch.startsWith(QLatin1String("stl")) ||
                       sSwitch.startsWith(QLatin1String("w")) || sSwitch.startsWith(QLatin1String("sa"))) {
                // Accepted and ignored: compression tuning, recursion into
                // directories, NTFS metadata and pager/work-dir switches have no
                // effect on a read-only unpacker.  Rejecting them would break
                // otherwise valid 7-Zip command lines for no benefit.
            } else {
                printf("Error: unsupported 7-Zip switch -%s\n", sSwitch.toUtf8().data());
                if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                return false;
            }

            continue;
        }

        if (!bVerbSeen) {
            bVerbSeen = true;

            if (sToken == QLatin1String("l")) pCommand->verb = VERB_LIST;
            else if (sToken == QLatin1String("x")) pCommand->verb = VERB_EXTRACT;
            else if (sToken == QLatin1String("e")) {
                pCommand->verb = VERB_EXTRACT;
                pCommand->bFlatten = true;
            } else if (sToken == QLatin1String("t")) pCommand->verb = VERB_TEST;
            else if (sToken == QLatin1String("i")) pCommand->verb = VERB_FORMATS;
            else {
                printf("Error: unsupported 7-Zip command '%s'\n", sToken.toUtf8().data());
                if (pcrResult) *pcrResult = XOptions::CR_INVALIDPARAMETER;
                return false;
            }

            continue;
        }

        if (pCommand->listTargets.isEmpty()) {
            pCommand->listTargets.append(sToken);
        } else {
            pCommand->listIncludes.append(sToken);
        }
    }

    if (bStdoutRequested && (pCommand->verb == VERB_EXTRACT)) {
        pCommand->verb = VERB_STDOUT;
    }

    return true;
}

QString XArchiveConsole::getDialectHelp(DIALECT dialect, const QString &sProgramName)
{
    const QString sName = QFileInfo(sProgramName).completeBaseName();
    QString sResult;

    if (dialect == DIALECT_SEVENZIP) {
        sResult += QString("Usage: %1 <command> [<switches>...] <archive> [<members>...]\n\n").arg(sName);
        sResult += "<Commands>\n";
        sResult += "  l : List contents of archive\n";
        sResult += "  x : eXtract files with full paths\n";
        sResult += "  e : Extract files without using directory names\n";
        sResult += "  t : Test integrity of archive\n";
        sResult += "  i : Show information about supported formats\n\n";
        sResult += "<Switches>\n";
        sResult += "  -o{Directory}  : set Output directory\n";
        sResult += "  -p{Password}   : set Password\n";
        sResult += "  -y             : assume Yes on all queries\n";
        sResult += "  -ao{a|s|u}     : set Overwrite mode (always|skip|rename)\n";
        sResult += "  -i!{wildcard}  : Include member names\n";
        sResult += "  -x!{wildcard}  : eXclude member names\n";
        sResult += "  -so            : write data to stdout\n";
        sResult += "  -slt           : show technical information for l (List)\n";
        sResult += "  -t{Type}       : set type of archive\n";
        sResult += "  --             : stop switches parsing\n";
    }

    sResult += "\nThis build also accepts its own long options in this dialect, for example\n";
    sResult += "--format=json, --password-hex, --codepage, --probe-timeout and --filetype.\n";

    return sResult;
}

// ---------------------------------------------------------------------------
// Native command line
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void XArchiveConsole::setUnpackProperties(const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    m_mapUnpackProperties = mapProperties;
}

QMap<XBinary::UNPACK_PROP, QVariant> XArchiveConsole::getUnpackProperties() const
{
    return m_mapUnpackProperties;
}

void XArchiveConsole::setProbeTimeout(qint64 nMilliseconds)
{
    m_nProbeTimeout = nMilliseconds;
}

qint64 XArchiveConsole::getProbeTimeout() const
{
    return m_nProbeTimeout;
}

bool XArchiveConsole::isProbeTimeoutOccurred() const
{
    return m_bProbeTimeoutOccurred;
}

void XArchiveConsole::clearProbeTimeoutOccurred()
{
    m_bProbeTimeoutOccurred = false;
}

QMap<XBinary::UNPACK_PROP, QVariant> XArchiveConsole::buildUnpackProperties(const COMMAND &command) const
{
    QMap<XBinary::UNPACK_PROP, QVariant> mapResult = m_mapUnpackProperties;

    // OVERWRITE_ALWAYS is the historical default and is what decompressToFolder
    // assumes when the property is absent; leave it unset so the extraction
    // core keeps its own defaulting.  OVERWRITE_RENAME is what that core
    // already does when overwriting is off and filename fixing is on, which is
    // 7-Zip's -aou / "extract next to it" behaviour.
    if (command.overwrite == OVERWRITE_SKIP) {
        // Only the dedicated skip flag: clearing UNPACK_PROP_OVERWRITEFILES as
        // well is neither needed nor harmless -- the skip decision happens
        // before the overwrite decision, and the cleared flag drives a separate
        // replace path in the publish step.
        mapResult.insert(XBinary::UNPACK_PROP_SKIPEXISTINGFILES, true);
    } else if (command.overwrite == OVERWRITE_ALWAYS) {
        mapResult.insert(XBinary::UNPACK_PROP_OVERWRITEFILES, true);
    } else if (command.overwrite == OVERWRITE_RENAME) {
        mapResult.insert(XBinary::UNPACK_PROP_OVERWRITEFILES, false);
        mapResult.insert(XBinary::UNPACK_PROP_FIXFILENAMES, true);
        mapResult.insert(XBinary::UNPACK_PROP_SKIPEXISTINGFILES, false);
    }

    return mapResult;
}

// ---------------------------------------------------------------------------
// Member selection
// ---------------------------------------------------------------------------

// Wildcard matching is written out rather than delegated to
// QRegularExpression::wildcardToRegularExpression(): Qt 5 and Qt 6 disagree on
// whether '*' may cross a '/' separator, and this project builds against both.
// Archive member selection follows the unzip / 7-Zip convention, where it does.
static bool _xacWildcardCharEquals(QChar cPattern, QChar cText, Qt::CaseSensitivity caseSensitivity)
{
    if (caseSensitivity == Qt::CaseSensitive) {
        return cPattern == cText;
    }

    return cPattern.toLower() == cText.toLower();
}

static bool _xacMatchWildcard(const QString &sPattern, const QString &sText, Qt::CaseSensitivity caseSensitivity)
{
    qint32 nPattern = 0;
    qint32 nText = 0;
    qint32 nStarPattern = -1;
    qint32 nStarText = 0;

    while (nText < sText.length()) {
        if ((nPattern < sPattern.length()) &&
            ((sPattern.at(nPattern) == QChar('?')) || _xacWildcardCharEquals(sPattern.at(nPattern), sText.at(nText), caseSensitivity))) {
            nPattern++;
            nText++;
        } else if ((nPattern < sPattern.length()) && (sPattern.at(nPattern) == QChar('*'))) {
            nStarPattern = nPattern;
            nStarText = nText;
            nPattern++;
        } else if (nStarPattern >= 0) {
            nPattern = nStarPattern + 1;
            nStarText++;
            nText = nStarText;
        } else {
            return false;
        }
    }

    while ((nPattern < sPattern.length()) && (sPattern.at(nPattern) == QChar('*'))) {
        nPattern++;
    }

    return nPattern == sPattern.length();
}

bool XArchiveConsole::isMemberSelected(const QString &sMemberName, const QStringList &listIncludes, const QStringList &listExcludes, bool bIgnoreCase)
{
    const Qt::CaseSensitivity caseSensitivity = bIgnoreCase ? Qt::CaseInsensitive : Qt::CaseSensitive;
    QString sNormalized = sMemberName;
    sNormalized.replace(QChar('\\'), QChar('/'));

    for (const QString &sPattern : listExcludes) {
        QString sNormalizedPattern = sPattern;
        sNormalizedPattern.replace(QChar('\\'), QChar('/'));

        if (_xacMatchWildcard(sNormalizedPattern, sNormalized, caseSensitivity)) {
            return false;
        }

        // A bare directory name covers everything under it, the way both
        // 7-Zip and unzip treat "sub" as "sub/*".
        if (sNormalized.startsWith(sNormalizedPattern + QChar('/'), caseSensitivity)) {
            return false;
        }
    }

    if (listIncludes.isEmpty()) {
        return true;
    }

    for (const QString &sPattern : listIncludes) {
        QString sNormalizedPattern = sPattern;
        sNormalizedPattern.replace(QChar('\\'), QChar('/'));

        if (_xacMatchWildcard(sNormalizedPattern, sNormalized, caseSensitivity)) {
            return true;
        }

        if (sNormalized.startsWith(sNormalizedPattern + QChar('/'), caseSensitivity)) {
            return true;
        }
    }

    return false;
}

QList<XBinary::ARCHIVERECORD> XArchiveConsole::filterRecords(const QList<XBinary::ARCHIVERECORD> &listRecords, const COMMAND &command)
{
    if (command.listIncludes.isEmpty() && command.listExcludes.isEmpty()) {
        return listRecords;
    }

    QList<XBinary::ARCHIVERECORD> listResult;

    for (qint32 i = 0; i < listRecords.count(); i++) {
        if (isMemberSelected(getRecordName(listRecords.at(i)), command.listIncludes, command.listExcludes, command.bIgnoreCase)) {
            listResult.append(listRecords.at(i));
        }
    }

    return listResult;
}

// ---------------------------------------------------------------------------
// Executors
// ---------------------------------------------------------------------------

XOptions::CR XArchiveConsole::execute(const COMMAND &command)
{
    if (command.verb == VERB_LIST) return listArchives(command);
    if (command.verb == VERB_EXTRACT) return extractArchives(command);
    if (command.verb == VERB_TEST) return testArchives(command);
    if (command.verb == VERB_STDOUT) return writeMembersToStdout(command);
    if (command.verb == VERB_FORMATS) return listSupportedFormats(command);
    if (command.verb == VERB_CREATE) return createArchive(command);
    if (command.verb == VERB_SHOWSTRUCTS) return showStructsOverview(command);

    // `-i` with no target is a question about the tool, not about a file, so
    // answer it instead of rejecting it: show every format this build can open.
    // The other two viewers below have no such tool-wide answer and still
    // require an operand.
    if ((command.verb == VERB_INFO) && command.listTargets.isEmpty()) {
        return showFormatTables(command);
    }

    // The remaining verbs are per-file viewers.
    if ((command.verb == VERB_INFO) || (command.verb == VERB_ENTROPY) || (command.verb == VERB_STRUCT)) {
        if (command.listTargets.isEmpty()) {
            printf("Error: this operation requires a target\n");
            return XOptions::CR_INVALIDPARAMETER;
        }

        XOptions::CR result = XOptions::CR_SUCCESS;
        const bool bShowFileName = (command.listTargets.count() > 1);

        for (const QString &sFileName : command.listTargets) {
            if (bShowFileName) {
                printf("%s:\n", QDir().toNativeSeparators(sFileName).toUtf8().data());
            }

            if (!QFileInfo::exists(sFileName)) {
                printf("Cannot find: %s\n", sFileName.toUtf8().data());
                result = XOptions::CR_CANNOTFINDFILE;
                continue;
            }

            XOptions::CR crViewer = XOptions::CR_SUCCESS;

            if (command.verb == VERB_ENTROPY) crViewer = showFileEntropy(sFileName, command);
            else if (command.verb == VERB_STRUCT) crViewer = showFileStruct(sFileName, command);
            else crViewer = showFileInfo(sFileName, command);

            if (crViewer != XOptions::CR_SUCCESS) result = crViewer;
        }

        return result;
    }

    return XOptions::CR_SUCCESS;
}

XBinary::FT XArchiveConsole::detectFileType(QIODevice *pDevice, XBinary::FT fileType, bool bValidateArchiveType, XBinary::PDSTRUCT *pPdStruct)
{
    // An explicit backend must be used for both enumeration and extraction.
    if (fileType != XBinary::FT_UNKNOWN) return fileType;
    if (m_nProbeTimeout == 0) {
        XBinary::disablePdStructDeadline(pPdStruct);
    } else {
        XBinary::setPdStructDeadline(pPdStruct, m_nProbeTimeout);
    }
    const XBinary::FT ftStatic = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_EXECUTABLES | XBinary::FT_FLAG_STATICUNPACKERS, pPdStruct);
    if (XFormats::isStaticUnpacker(ftStatic)) return ftStatic;

    // Compound-file installers such as MSI become available in this pass.
    const XBinary::FT ftArchive = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES | XBinary::FT_FLAG_STATICUNPACKERS, pPdStruct);
    if (!bValidateArchiveType || XFormats::isStaticUnpacker(ftArchive) || XArchives::getArchiveOpenValidFileTypes().contains(ftArchive)) return ftArchive;
    return XBinary::FT_UNKNOWN;
}

XOptions::CR XArchiveConsole::listArchives(const QStringList &listFileNames, XBinary::FT fileType, bool bVerbose)
{
    COMMAND command;
    command.verb = VERB_LIST;
    command.listTargets = listFileNames;
    command.fileType = fileType;
    command.bVerbose = bVerbose;
    // The host owns --format; setFormat() has already recorded whatever it saw.
    command.listFormat = m_bListFormatSet ? m_listFormat : (bVerbose ? LISTFORMAT_TECHNICAL : LISTFORMAT_NATIVE);
    if (m_bResultFormatSet) command.resultFormat = m_resultFormat;

    return listArchives(command);
}

XOptions::CR XArchiveConsole::listArchives(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    if (command.listTargets.isEmpty()) {
        printf("Error: --listarchive requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    const bool bShowFileName = (command.listTargets.count() > 1);

    for (const QString &sFileName : command.listTargets) {
        if (!QFileInfo::exists(sFileName)) {
            printf("Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        if (bShowFileName) {
            printf("%s:\n", QDir().toNativeSeparators(sFileName).toUtf8().data());
        }

        QFile file;
        file.setFileName(sFileName);

        if (!file.open(QIODevice::ReadOnly)) {
            printf("Cannot open: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        const XBinary::FT currentFileType = detectFileType(&file, command.fileType, true, &archivePdStruct);

        if (XBinary::isPdStructDeadlineExpired(&archivePdStruct)) {
            printf("Detection budget exceeded: %s\n", sFileName.toUtf8().data());
            m_bProbeTimeoutOccurred = true;
            result = XOptions::CR_PROBETIMEOUT;
            file.close();
            continue;
        }

        // The budget covers automatic format probing only. Enumeration
        // and extraction retain their existing cancellation semantics.
        XBinary::disablePdStructDeadline(&archivePdStruct);

        QList<XBinary::ARCHIVERECORD> listRecords;
        QMap<XBinary::FPART_PROP, QVariant> mapArchiveProperties;
        bool bListed = false;
        XBinary *pArchive = nullptr;

        if (XFormats::isStaticUnpacker(currentFileType) || XFormats::isArchive(currentFileType)) {
            pArchive = XFormats::createClass(currentFileType, &file);

            if (pArchive) {
                bool bComplete = false;
                listRecords = getRecords(pArchive, m_mapUnpackProperties, &archivePdStruct, &bComplete, &mapArchiveProperties);
                bListed = bComplete;
            }
        }

        const qint64 nPhysicalSize = file.size();

        if (bListed) {
            const QList<XBinary::ARCHIVERECORD> listSelected = filterRecords(listRecords, command);
            QString sListing;

            if (command.listFormat == LISTFORMAT_JSON) {
                sListing = formatListJson(sFileName, currentFileType, listSelected, nPhysicalSize, mapArchiveProperties);
            } else if (command.listFormat == LISTFORMAT_XML) {
                sListing = formatListXml(sFileName, currentFileType, listSelected, nPhysicalSize, mapArchiveProperties);
            } else if (command.listFormat == LISTFORMAT_CSV) {
                sListing = formatListDelimited(listSelected, false);
            } else if (command.listFormat == LISTFORMAT_TSV) {
                sListing = formatListDelimited(listSelected, true);
            } else {
                sListing = formatList(currentFileType, listSelected, nPhysicalSize, command.listFormat == LISTFORMAT_TECHNICAL, mapArchiveProperties);
            }

            printf("%s", sListing.toUtf8().data());

            // A recognised container that enumerates no members at all is not a
            // complete listing, it is a shortfall: the whole input was accounted
            // as overhead and nothing was described.  Left at CR_SUCCESS this is
            // the one outcome where the exit code claims more than the tool did,
            // so it joins the shortfall codes and says why in plain words.
            // Note this asks the ARCHIVE, not the filtered view - an --include
            // that matches nothing is the user's own narrowing, not a shortfall.
            if (listRecords.isEmpty()) {
                printf("Archive contains no members: %s\n", QDir().toNativeSeparators(sFileName).toUtf8().data());
                result = XOptions::CR_PARTIALRESULT;
            } else {
                // ISSUE-33: the same shortfall one step further in.  A record
                // set can be present and still describe nothing, and a listing
                // is what a sweep reads, so name it here instead of letting the
                // member count stand as evidence the container was understood.
                const QString sRefutation = describeSelfRefutingListing(listRecords, nPhysicalSize);

                if (!sRefutation.isEmpty()) {
                    printf("Listing does not describe the file's contents (%s): %s\n", sRefutation.toUtf8().data(),
                           QDir().toNativeSeparators(sFileName).toUtf8().data());
                    result = XOptions::CR_PARTIALRESULT;
                }
            }
        } else {
            printf("Cannot open archive: %s\n", sFileName.toUtf8().data());

            if (command.bVerbose && (currentFileType != XBinary::FT_UNKNOWN)) {
                printf("  Detected: %s\n", XBinary::fileTypeIdToString(currentFileType).toUtf8().data());
            }

            const QString sArchiveError = XBinary::getPdStructErrorString(&archivePdStruct);

            if (command.bVerbose && !sArchiveError.isEmpty()) {
                printf("  %s\n", sArchiveError.toUtf8().data());
            }

            result = XOptions::CR_CANNOTOPENFILE;
        }

        delete pArchive;
        file.close();
    }

    return result;
}

XOptions::CR XArchiveConsole::extractArchives(const QString &sResultDirectory, const QStringList &listFileNames, XBinary::FT fileType, bool bVerbose)
{
    COMMAND command;
    command.verb = VERB_EXTRACT;
    command.sOutputDirectory = sResultDirectory;
    command.listTargets = listFileNames;
    command.fileType = fileType;
    command.bVerbose = bVerbose;

    return extractArchives(command);
}

XOptions::CR XArchiveConsole::extractArchives(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;
    const QString sResultDirectory = command.sOutputDirectory.isEmpty() ? QDir::currentPath() : command.sOutputDirectory;

    if (command.listTargets.isEmpty()) {
        printf("Error: --extractarchive requires <directory> <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    // The native --extractarchive spelling always names a directory; the
    // foreign dialects may omit it and mean "here".
    if ((command.dialect == DIALECT_NATIVE) && command.sOutputDirectory.isEmpty()) {
        printf("Error: --extractarchive requires <directory> <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    if (!QDir().mkpath(sResultDirectory)) {
        printf("Cannot create directory: %s\n", sResultDirectory.toUtf8().data());
        return XOptions::CR_INVALIDPARAMETER;
    }

    const bool bMemberSelection = !command.listIncludes.isEmpty() || !command.listExcludes.isEmpty() || command.bFlatten;

    if (bMemberSelection) {
        // Member filtering and path flattening have to be applied inside
        // XBinary::decompressToFolder, which owns the folder transaction,
        // canonical-root confinement, reparse-point rejection and duplicate
        // name resolution.  Re-implementing a per-member loop out here would
        // silently drop every one of those guarantees, so refuse instead of
        // pretending the option worked.
        printf("Error: member selection and path flattening are not implemented for extraction yet\n");
        printf("       (they are honoured by --listarchive and --stdout)\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    const QMap<XBinary::UNPACK_PROP, QVariant> mapProperties = buildUnpackProperties(command);

    // One manifest describes the whole run, so several archives extracted in
    // one command produce one file with an entry each.
    QJsonArray jsonManifest;

    for (const QString &sFileName : command.listTargets) {
        if (!QFileInfo::exists(sFileName)) {
            printf("Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        QFile file;
        file.setFileName(sFileName);

        if (!file.open(QIODevice::ReadOnly)) {
            printf("Cannot open: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        const XBinary::FT currentFileType = detectFileType(&file, command.fileType, false, &archivePdStruct);

        if (XBinary::isPdStructDeadlineExpired(&archivePdStruct)) {
            printf("Detection budget exceeded: %s\n", sFileName.toUtf8().data());
            m_bProbeTimeoutOccurred = true;
            result = XOptions::CR_PROBETIMEOUT;
            file.close();
            continue;
        }

        XBinary::disablePdStructDeadline(&archivePdStruct);

        qint32 nNumberOfFiles = 0;
        qint64 nTotalSize = 0;
        bool bTotalSizeComplete = true;
        QList<XBinary::ARCHIVERECORD> listRecords;
        XBinary *pArchive = nullptr;
        bool bArchiveRecognized = false;

        if (XFormats::isStaticUnpacker(currentFileType) || XFormats::isArchive(currentFileType)) {
            pArchive = XFormats::createClass(currentFileType, &file);

            if (pArchive) {
                bArchiveRecognized = true;
                listRecords = getRecords(pArchive, mapProperties, &archivePdStruct);
            }
        }

        // Read the end-of-central-directory comment now: the archive object is
        // deleted below, and the manifest is written after that.
        QString sArchiveComment;
        {
            XArchive *pArchiveForComment = qobject_cast<XArchive *>(pArchive);

            if (pArchiveForComment && pArchiveForComment->isCommentPresent()) {
                sArchiveComment = pArchiveForComment->getComment();
            }
        }

        for (qint32 i = 0; i < listRecords.count(); i++) {
            if (!isRecordFolder(listRecords.at(i))) {
                nNumberOfFiles++;

                if (isRecordSizePresent(listRecords.at(i))) {
                    nTotalSize += getRecordSize(listRecords.at(i));
                } else {
                    bTotalSizeComplete = false;
                }

                if (command.bVerbose) {
                    printf("  %s\n", getRecordName(listRecords.at(i)).toUtf8().data());
                }
            }
        }

        delete pArchive;

        XBinary::setPdStructErrorString(&archivePdStruct, QString());
        const qint64 nPhysicalSize = file.size();
        qint32 nSkippedEntries = 0;
        const bool bExtracted = file.seek(0) && XArchives::decompressToFolder(&file, sResultDirectory, mapProperties, &archivePdStruct, &nSkippedEntries, currentFileType);
        file.close();

        if (!command.sManifest.isEmpty()) {
            // Same record shape as a "--format json" listing, plus what this
            // particular extraction did with them.
            QJsonObject jsonArchive = buildListJson(sFileName, currentFileType, listRecords, nPhysicalSize);

            // Point every member at the file it was written to, so the manifest
            // alone is enough to pack the archive again.
            QJsonArray jsonRecords = jsonArchive.value("records").toArray();

            for (qint32 r = 0; r < jsonRecords.count(); r++) {
                QJsonObject jsonRecord = jsonRecords.at(r).toObject();
                const QString sMemberName = jsonRecord.value("Name").toString();

                if (!sMemberName.isEmpty() && !jsonRecord.value("Folder").toBool()) {
                    const QString sPath = QDir(sResultDirectory).filePath(sMemberName);

                    if (QFileInfo::exists(sPath)) {
                        jsonRecord.insert("sourcePath", QDir().toNativeSeparators(sPath));
                    }
                }

                jsonRecords.replace(r, jsonRecord);
            }

            jsonArchive.insert("records", jsonRecords);
            jsonArchive.insert("outputDirectory", QDir().toNativeSeparators(sResultDirectory));

            // The end-of-central-directory comment belongs to the archive, not
            // to any member, so it is carried at the top level.
            if (!sArchiveComment.isEmpty()) {
                jsonArchive.insert("comment", sArchiveComment);
            }
            jsonArchive.insert("extracted", bExtracted);
            jsonArchive.insert("skipped", nSkippedEntries);
            jsonManifest.append(jsonArchive);
        }

        if (bExtracted) {
            const QString sTotalSize = bTotalSizeComplete ? XBinary::bytesCountToString(nTotalSize, 1024) : QString("unknown");
            // ISSUE-33: the listing contradiction, evaluated on the verb that
            // actually writes files.  The folder transaction commits happily
            // after writing N empty placeholders, so without this an extraction
            // reports success having produced no bytes from a non-empty file -
            // the same silent-success shape as the guard below, one member in.
            const QString sRefutation = describeSelfRefutingListing(listRecords, nPhysicalSize);

            if (nSkippedEntries > 0) {
                const qint32 nExtractedFiles = qMax(0, nNumberOfFiles - nSkippedEntries);
                if (!command.bQuiet) {
                    printf("Extracted %d of %d file(s) -> %s\n", nExtractedFiles, nNumberOfFiles, QDir().toNativeSeparators(sResultDirectory).toUtf8().data());
                }
                result = XOptions::CR_PARTIALRESULT;
            } else if (bArchiveRecognized && listRecords.isEmpty()) {
                // The folder transaction commits trivially when there is nothing
                // to write, so a container that enumerates no members reaches
                // here as a clean success having produced not one byte.  That is
                // the silent-success shape this tool has been bitten by before:
                // name it and carry a shortfall code so a sweep cannot bank it.
                // Guarded on listRecords rather than nNumberOfFiles so that a
                // folder-only archive - which really did create its directories
                // - keeps reporting success.
                if (!command.bQuiet) {
                    printf("Extracted 0 file(s): archive contains no members -> %s\n", QDir().toNativeSeparators(sResultDirectory).toUtf8().data());
                }
                result = XOptions::CR_PARTIALRESULT;
            } else if (!sRefutation.isEmpty()) {
                if (!command.bQuiet) {
                    printf("Extracted %d file(s), %s -> %s\n", nNumberOfFiles, sTotalSize.toUtf8().data(),
                           QDir().toNativeSeparators(sResultDirectory).toUtf8().data());
                    printf("Listing does not describe the file's contents (%s): %s\n", sRefutation.toUtf8().data(),
                           QDir().toNativeSeparators(sFileName).toUtf8().data());
                }

                result = XOptions::CR_PARTIALRESULT;
            } else if (!command.bQuiet) {
                printf("Extracted %d file(s), %s -> %s\n", nNumberOfFiles, sTotalSize.toUtf8().data(), QDir().toNativeSeparators(sResultDirectory).toUtf8().data());
            }

            // Best-effort extraction commits with a skip tally in the
            // progress error string; surface it so a partial result is
            // never mistaken for a complete one.
            const QString sSkippedWarning = XBinary::getPdStructErrorString(&archivePdStruct);

            if (!sSkippedWarning.isEmpty()) {
                printf("  Warning: %s\n", sSkippedWarning.toUtf8().data());
            }
        } else {
            printf("Cannot extract: %s\n", sFileName.toUtf8().data());
            const QString sExtractionError = XBinary::getPdStructErrorString(&archivePdStruct);

            // ISSUE-33's hardest shape is a refusal that never says why.  The
            // reader has already produced a precise cause - an absent
            // companion volume, an unsupported codec - and gating it on
            // --verbose left the default run printing only "Cannot extract",
            // which reads like a parser defect whatever the real reason was.
            // The partial-extraction branch above prints its reason
            // unconditionally; a total failure needs one more, not less.
            if (!sExtractionError.isEmpty()) {
                printf("  %s\n", sExtractionError.toUtf8().data());
            }

            result = XOptions::CR_CANNOTOPENFILE;
        }
    }

    if (!command.sManifest.isEmpty()) {
        // One archive -- the documented case -- is written as the archive
        // object itself, with no wrapper. Several archives in one command
        // become an array of exactly those objects, since a JSON file has room
        // for only one root.
        const QJsonDocument jsonDocument = (jsonManifest.count() == 1) ? QJsonDocument(jsonManifest.at(0).toObject()) : QJsonDocument(jsonManifest);

        QFile manifestFile;
        manifestFile.setFileName(command.sManifest);

        if (manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            manifestFile.write(jsonDocument.toJson(QJsonDocument::Indented));
            manifestFile.close();

            if (!command.bQuiet) {
                printf("Manifest -> %s\n", QDir().toNativeSeparators(command.sManifest).toUtf8().data());
            }
        } else {
            // The archive is already on disk; say the manifest failed rather
            // than reporting the whole extraction as a failure.
            printf("Cannot write manifest: %s\n", command.sManifest.toUtf8().data());
            result = XOptions::CR_PARTIALRESULT;
        }
    }

    return result;
}

XOptions::CR XArchiveConsole::testArchives(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    if (command.listTargets.isEmpty()) {
        printf("Error: --testarchive requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    for (const QString &sFileName : command.listTargets) {
        if (!QFileInfo::exists(sFileName)) {
            printf("Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        XBinary::disablePdStructDeadline(&archivePdStruct);

        const bool bTested = XArchives::testArchive(sFileName, m_mapUnpackProperties, &archivePdStruct, command.fileType);
        const QString sError = XBinary::getPdStructErrorString(&archivePdStruct);

        if (bTested) {
            if (!command.bQuiet) {
                printf("No errors detected in compressed data of %s\n", QDir().toNativeSeparators(sFileName).toUtf8().data());
            }

            if (!sError.isEmpty()) {
                printf("  Warning: %s\n", sError.toUtf8().data());
            }
        } else {
            printf("Test failed: %s\n", QDir().toNativeSeparators(sFileName).toUtf8().data());

            if (!sError.isEmpty()) {
                printf("  %s\n", sError.toUtf8().data());
            }

            result = XOptions::CR_CANNOTOPENFILE;
        }
    }

    return result;
}

XOptions::CR XArchiveConsole::writeMembersToStdout(const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    if (command.listTargets.isEmpty()) {
        printf("Error: --stdout requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    XBinary::OUTPUT_POLICY policy = {};
    if (!XBinary::resolveUnpackOutputPolicy(m_mapUnpackProperties, &policy)) {
        fprintf(stderr, "Invalid unpacked-output limit\n");
        return XOptions::CR_INVALIDPARAMETER;
    }
    QSharedPointer<XBinary::OUTPUT_BUDGET> operationBudget = QSharedPointer<XBinary::OUTPUT_BUDGET>::create();
    operationBudget->configureForProperties(policy, m_mapUnpackProperties);

    // The checked single-record API publishes into a seekable device after
    // verification. Bound this temporary byte array before it reaches stdout.
    class BoundedBuffer final : public QBuffer {
    public:
        BoundedBuffer(QByteArray *pData, qint64 nLimit) : QBuffer(pData), m_nLimit(nLimit) {}
        bool seek(qint64 nPosition) override
        {
            return (nPosition >= 0) && ((m_nLimit < 0) || (nPosition <= m_nLimit)) && QBuffer::seek(nPosition);
        }
    protected:
        qint64 writeData(const char *pData, qint64 nSize) override
        {
            if ((nSize < 0) || !XBinary::OUTPUT_BUDGET::withinLimit(pos(), m_nLimit, nSize)) return -1;
            return QBuffer::writeData(pData, nSize);
        }
    private:
        qint64 m_nLimit;
    };

#ifdef Q_OS_WIN
    // QFile wraps the CRT stdout handle. Its underlying descriptor must be
    // binary too, otherwise Windows expands LF and changes both bytes and size.
    class StdoutModeGuard {
    public:
        StdoutModeGuard() : m_nPreviousMode(-1) {}
        bool setBinary()
        {
            if (std::fflush(stdout) != 0) return false;
            m_nPreviousMode = _setmode(_fileno(stdout), _O_BINARY);
            return m_nPreviousMode != -1;
        }
        ~StdoutModeGuard()
        {
            if (m_nPreviousMode != -1) {
                std::fflush(stdout);
                _setmode(_fileno(stdout), m_nPreviousMode);
            }
        }
    private:
        int m_nPreviousMode;
    } stdoutModeGuard;
    if (!stdoutModeGuard.setBinary()) {
        fprintf(stderr, "Error: cannot set standard output to binary mode\n");
        return XOptions::CR_CANNOTOPENFILE;
    }
#endif

    QFile output;
    if (!output.open(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered, QFileDevice::DontCloseHandle)) {
        fprintf(stderr, "Error: cannot write to standard output\n");
        return XOptions::CR_CANNOTOPENFILE;
    }

    bool bBudgetExhausted = false;
    for (const QString &sFileName : command.listTargets) {
        if (bBudgetExhausted) break;
        if (!QFileInfo::exists(sFileName)) {
            fprintf(stderr, "Cannot find: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
            continue;
        }

        QFile file(sFileName);
        if (!file.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Cannot open: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            continue;
        }

        XBinary::PDSTRUCT archivePdStruct = XBinary::createPdStruct();
        const XBinary::FT currentFileType = detectFileType(&file, command.fileType, true, &archivePdStruct);
        if (XBinary::isPdStructDeadlineExpired(&archivePdStruct)) {
            fprintf(stderr, "Detection budget exceeded: %s\n", sFileName.toUtf8().data());
            m_bProbeTimeoutOccurred = true;
            result = XOptions::CR_PROBETIMEOUT;
            continue;
        }
        XBinary::disablePdStructDeadline(&archivePdStruct);

        // Keep the chosen handler and original record indices. Re-detecting
        // through the legacy name API loses forced types and archive streams.
        QScopedPointer<XBinary> archive;
        if (XFormats::isStaticUnpacker(currentFileType) || XFormats::isArchive(currentFileType)) {
            archive.reset(XFormats::createClass(currentFileType, &file));
        }
        const QList<XBinary::ARCHIVERECORD> listRecords = getRecords(archive.data(), m_mapUnpackProperties, &archivePdStruct);
        if (listRecords.isEmpty()) {
            fprintf(stderr, "Cannot open archive: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
            continue;
        }

        qint32 nMatched = 0;
        for (qint32 i = 0; i < listRecords.count(); ++i) {
            const XBinary::ARCHIVERECORD &record = listRecords.at(i);
            const QString sMemberName = getRecordName(record);
            if (isRecordFolder(record) || !isMemberSelected(sMemberName, command.listIncludes, command.listExcludes, command.bIgnoreCase)) continue;
            ++nMatched;

            if (!operationBudget->beginEntry(i, sMemberName) && operationBudget->isEnforcing()) {
                fprintf(stderr, "Archive member count exceeds the configured limit\n");
                result = XOptions::CR_PARTIALRESULT;
                bBudgetExhausted = true;
                break;
            }

            QMap<XBinary::UNPACK_PROP, QVariant> memberProperties = m_mapUnpackProperties;
            qint64 nBufferLimit = policy.nMaxMemoryOutputSize;
            if (policy.nMaxEntryOutputSize >= 0) nBufferLimit = (nBufferLimit < 0) ? policy.nMaxEntryOutputSize : qMin(nBufferLimit, policy.nMaxEntryOutputSize);
            if (m_mapUnpackProperties.contains(XBinary::UNPACK_PROP_MAX_TOTAL_OUTPUT_SIZE) && (policy.nMaxTotalOutputSize >= 0)) {
                const qint64 nRemaining = qMax(qint64(0), policy.nMaxTotalOutputSize - operationBudget->totalWritten());
                memberProperties.insert(XBinary::UNPACK_PROP_MAX_TOTAL_OUTPUT_SIZE, nRemaining);
                nBufferLimit = (nBufferLimit < 0) ? nRemaining : qMin(nBufferLimit, nRemaining);
            }
            // This call processes one selected member. Its local count must not
            // replace the operation-wide count charged above.
            if (memberProperties.contains(XBinary::UNPACK_PROP_MAX_ENTRY_COUNT)) memberProperties.insert(XBinary::UNPACK_PROP_MAX_ENTRY_COUNT, qint64(1));

            XBinary::PDSTRUCT memberPdStruct = XBinary::createPdStruct();
            XBinary::disablePdStructDeadline(&memberPdStruct);
            QByteArray baData;
            BoundedBuffer buffer(&baData, nBufferLimit);
            const bool bWithinKnownSize = !isRecordSizePresent(record) || (nBufferLimit < 0) || (getRecordSize(record) <= nBufferLimit);
            const bool bUnpacked = bWithinKnownSize && file.seek(0) && buffer.open(QIODevice::ReadWrite) &&
                                   archive->unpackRecordByIndex(i, &record, &buffer, memberProperties, &memberPdStruct);
            buffer.close();
            if (!bUnpacked || !XBinary::isPdStructNotCanceled(&memberPdStruct)) {
                fprintf(stderr, "Cannot extract member: %s\n", sMemberName.toUtf8().data());
                const QString sError = XBinary::getPdStructErrorString(&memberPdStruct);
                if (!bWithinKnownSize) fprintf(stderr, "  Unpacked output exceeds the configured limit\n");
                else if (!sError.isEmpty()) fprintf(stderr, "  %s\n", sError.toUtf8().data());
                result = XOptions::CR_PARTIALRESULT;
                if (!m_mapUnpackProperties.value(XBinary::UNPACK_PROP_CONTINUEONERROR, false).toBool()) break;
                continue;
            }

            if (!operationBudget->debit(baData.size()) && operationBudget->isEnforcing()) {
                fprintf(stderr, "Unpacked output exceeds the configured operation limit\n");
                result = XOptions::CR_PARTIALRESULT;
                bBudgetExhausted = true;
                break;
            }
            qint64 nWritten = 0;
            while (nWritten < baData.size()) {
                const qint64 nChunk = output.write(baData.constData() + nWritten, baData.size() - nWritten);
                if (nChunk <= 0) break;
                nWritten += nChunk;
            }
            if (nWritten != baData.size()) {
                fprintf(stderr, "Cannot write member to standard output: %s\n", sMemberName.toUtf8().data());
                result = XOptions::CR_CANNOTOPENFILE;
                bBudgetExhausted = true;
                break;
            }
        }
        if (nMatched == 0) {
            fprintf(stderr, "No matching member in: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTFINDFILE;
        }
    }
    if (!output.flush()) result = XOptions::CR_CANNOTOPENFILE;
    return result;
}

XOptions::CR XArchiveConsole::showFormatTables(const COMMAND &command)
{
    const QSet<XBinary::FT> stFileTypes = XArchives::getArchiveOpenValidFileTypes();

    // The machine-readable shapes already answer this question with the same
    // set of names, so delegate rather than invent a second grouped schema
    // nothing asked for. Only the two human-readable shapes get tables.
    if ((command.listFormat != LISTFORMAT_NATIVE) && (command.listFormat != LISTFORMAT_TECHNICAL)) {
        return listSupportedFormats(command);
    }

    struct CATEGORY {
        quint32 nFlag;
        const char *pszTitle;
    };

    // Widest interest first. Empty categories are skipped rather than printed
    // as empty tables.
    const CATEGORY categories[] = {
        {XBinary::FT_FLAG_ARCHIVES, "Archives and containers"},
        {XBinary::FT_FLAG_STATICUNPACKERS, "Packed executables"},
        {XBinary::FT_FLAG_EXECUTABLES, "Executables"},
        {XBinary::FT_FLAG_DOCUMENTS, "Documents"},
        {XBinary::FT_FLAG_IMAGES, "Images"},
        {XBinary::FT_FLAG_AUDIO, "Audio"},
        {XBinary::FT_FLAG_VIDEO, "Video"},
        {XBinary::FT_FLAG_TEXT, "Text"},
    };

    const qint32 nNumberOfCategories = qint32(sizeof(categories) / sizeof(categories[0]));
    QSet<XBinary::FT> stShown;
    qint32 nTotal = 0;
    qint32 nTables = 0;

    // Say what the ID column is for. It is the token -F/--filetype accepts, so
    // the table is something to act on rather than just read; without this the
    // two columns look like the same thing printed twice (they coincide for
    // ZIP, and differ for ZLIB/"zlib" or 7ZIP/"7-Zip").
    printf("Container formats this build can open.\n");
    printf("The ID is what -F/--filetype accepts.\n");

    for (qint32 nCategory = 0; nCategory < nNumberOfCategories; nCategory++) {
        const QList<XBinary::FT> listAll = XBinary::_getFileTypeListFromSet(stFileTypes, categories[nCategory].nFlag);
        QList<XBinary::FT> listTypes;

        for (qint32 i = 0; i < listAll.count(); i++) {
            const XBinary::FT fileType = listAll.at(i);

            // _getFileTypeListFromSet prepends the generic pseudo-types to any
            // FT_FLAG_FORMATS request, and every category above is part of that
            // mask - so without this they would head all eight tables.
            if ((fileType == XBinary::FT_REGION) || (fileType == XBinary::FT_DATA) || (fileType == XBinary::FT_BINARY)) continue;
            if (stShown.contains(fileType)) continue;

            listTypes.append(fileType);
            stShown.insert(fileType);
        }

        if (listTypes.isEmpty()) continue;

        printFormatTable(QString(categories[nCategory].pszTitle), listTypes);
        nTotal += listTypes.count();
        nTables++;
    }

    // Anything no category claimed. Printed rather than dropped: this is a
    // "what can it open" answer, so a silently omitted format would be a wrong
    // answer, and an entry appearing here is the signal that a new type was
    // added without a category.
    QList<XBinary::FT> listRest;

    for (QSet<XBinary::FT>::const_iterator it = stFileTypes.constBegin(); it != stFileTypes.constEnd(); ++it) {
        if (stShown.contains(*it)) continue;
        if ((*it == XBinary::FT_REGION) || (*it == XBinary::FT_DATA) || (*it == XBinary::FT_BINARY)) continue;
        if (XBinary::fileTypeIdToString(*it).isEmpty()) continue;

        listRest.append(*it);
    }

    if (!listRest.isEmpty()) {
        printFormatTable(QStringLiteral("Other"), listRest);
        nTotal += listRest.count();
        nTables++;
    }

    printf("\n%lld format(s) in %lld table(s). Name a file to describe that file instead.\n", static_cast<long long>(nTotal),
           static_cast<long long>(nTables));

    return XOptions::CR_SUCCESS;
}

void XArchiveConsole::printFormatTable(const QString &sTitle, const QList<XBinary::FT> &listTypes)
{
    QStringList listIds;
    QStringList listNames;
    qint32 nIdWidth = 2;  // the "ID" header itself

    for (qint32 i = 0; i < listTypes.count(); i++) {
        QString sId = XBinary::fileTypeIdToFtString(listTypes.at(i));
        QString sName = XBinary::fileTypeIdToString(listTypes.at(i));

        if (sName.isEmpty()) sName = sId;

        listIds.append(sId);
        listNames.append(sName);

        if (sId.length() > nIdWidth) nIdWidth = sId.length();
    }

    printf("\n%s (%lld)\n", sTitle.toUtf8().data(), static_cast<long long>(listTypes.count()));
    printf("%s  %s\n", QString("ID").leftJustified(nIdWidth, QChar(' ')).toUtf8().data(), "Format");
    printf("%s  %s\n", QString(nIdWidth, QChar('-')).toUtf8().data(), QString(6, QChar('-')).toUtf8().data());

    for (qint32 i = 0; i < listIds.count(); i++) {
        printf("%s  %s\n", listIds.at(i).leftJustified(nIdWidth, QChar(' ')).toUtf8().data(), listNames.at(i).toUtf8().data());
    }
}

XOptions::CR XArchiveConsole::listSupportedFormats(const COMMAND &command)
{
    QStringList listNames;
    const QSet<XBinary::FT> stFileTypes = XArchives::getArchiveOpenValidFileTypes();

    for (QSet<XBinary::FT>::const_iterator it = stFileTypes.constBegin(); it != stFileTypes.constEnd(); ++it) {
        const QString sName = XBinary::fileTypeIdToString(*it);

        if (!sName.isEmpty()) {
            listNames.append(sName);
        }
    }

    listNames.sort(Qt::CaseInsensitive);

    if (command.listFormat == LISTFORMAT_JSON) {
        QJsonObject jsonRoot;
        QJsonArray jsonFormats;

        for (const QString &sName : listNames) {
            jsonFormats.append(sName);
        }

        jsonRoot.insert("formats", jsonFormats);
        jsonRoot.insert("count", listNames.count());

        printf("%s\n", QJsonDocument(jsonRoot).toJson(QJsonDocument::Indented).constData());
    } else {
        printf("Supported archive formats (%lld):\n", static_cast<long long>(listNames.count()));

        for (const QString &sName : listNames) {
            printf("  %s\n", sName.toUtf8().data());
        }
    }

    return XOptions::CR_SUCCESS;
}

QJsonObject XArchiveConsole::buildListJson(const QString &sArchiveName, XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords,
                                           qint64 nPhysicalSize, const QMap<XBinary::FPART_PROP, QVariant> &mapArchiveProperties)
{
    QJsonObject jsonRoot;

    jsonRoot.insert("archive", QDir().toNativeSeparators(sArchiveName));
    jsonRoot.insert("fileType", XBinary::fileTypeIdToString(fileType));
    jsonRoot.insert("physicalSize", nPhysicalSize);
    // ISSUE-25: see formatList(). Unconditional here - a machine-readable
    // listing has no screen to crowd, and a consumer can ignore a field.
    const QString sArchiveInfoJson = mapArchiveProperties.value(XBinary::FPART_PROP_INFO).toString().trimmed();
    if (!sArchiveInfoJson.isEmpty()) {
        jsonRoot.insert("info", sArchiveInfoJson);
    }

    QJsonArray jsonRecords;
    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    qint32 nNumberOfFiles = 0;
    qint32 nNumberOfFolders = 0;

    for (qint32 i = 0; i < listRecords.count(); i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);
        QJsonObject jsonRecord;

        if (isRecordFolder(record)) {
            nNumberOfFolders++;
        } else {
            nNumberOfFiles++;
        }

        if (isRecordSizePresent(record)) {
            nTotalSize += getRecordSize(record);
        }

        qint64 nPacked = 0;
        if (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE) {
            nTotalPacked += nPacked;
        }

        // Emit the full property map: this is the whole point of a
        // machine-readable listing, and neither 7-Zip nor Info-ZIP exposes it.
        QList<XBinary::FPART_PROP> listKeys = record.mapProperties.keys();
        std::sort(listKeys.begin(), listKeys.end());

        for (qint32 k = 0; k < listKeys.count(); k++) {
            const XBinary::FPART_PROP prop = listKeys.at(k);

            if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN) {
                continue;  // session bookkeeping, not archive metadata
            }

            const QVariant varValue = record.mapProperties.value(prop);
            const QString sKey = getPropertyName(prop);

            // Numbers stay numbers only where the number IS the value: sizes,
            // offsets and ids.  Everything else -- method, CRC, timestamps,
            // file mode -- is emitted as the rendered text, because the raw
            // enum ordinal ("Method": 3) means nothing outside this build and
            // would silently change when the enum grows.
            const bool bNumericQuantity =
                (prop == XBinary::FPART_PROP_UNCOMPRESSEDSIZE) || (prop == XBinary::FPART_PROP_COMPRESSEDSIZE) || (prop == XBinary::FPART_PROP_STREAMOFFSET) ||
                (prop == XBinary::FPART_PROP_STREAMSIZE) || (prop == XBinary::FPART_PROP_STREAMUNPACKEDSIZE) || (prop == XBinary::FPART_PROP_SUBSTREAMOFFSET) ||
                (prop == XBinary::FPART_PROP_UID) || (prop == XBinary::FPART_PROP_GID) || (prop == XBinary::FPART_PROP_WINDOWSIZE) ||
                (prop == XBinary::FPART_PROP_SOLIDFOLDERINDEX) || (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_INDEX) ||
                (prop == XBinary::FPART_PROP_FLAGS) || (prop == XBinary::FPART_PROP_VERSIONMADEBY) || (prop == XBinary::FPART_PROP_VERSIONNEEDED) ||
                (prop == XBinary::FPART_PROP_INTERNALATTRIBUTES) || (prop == XBinary::FPART_PROP_EXTERNALATTRIBUTES);

            if (varValue.userType() == QMetaType::Bool) {
                jsonRecord.insert(sKey, varValue.toBool());
            } else if (bNumericQuantity) {
                jsonRecord.insert(sKey, varValue.toLongLong());
            } else {
                jsonRecord.insert(sKey, getPropertyValueString(record, prop));
            }
        }

        jsonRecords.append(jsonRecord);
    }

    jsonRoot.insert("records", jsonRecords);
    jsonRoot.insert("numberOfFiles", nNumberOfFiles);
    jsonRoot.insert("numberOfFolders", nNumberOfFolders);
    jsonRoot.insert("totalSize", nTotalSize);
    jsonRoot.insert("totalPacked", nTotalPacked);

    return jsonRoot;
}

QString XArchiveConsole::formatListJson(const QString &sArchiveName, XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize,
                                        const QMap<XBinary::FPART_PROP, QVariant> &mapArchiveProperties)
{
    return QString::fromUtf8(QJsonDocument(buildListJson(sArchiveName, fileType, listRecords, nPhysicalSize, mapArchiveProperties)).toJson(QJsonDocument::Indented));
}

QString XArchiveConsole::formatListXml(const QString &sArchiveName, XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize,
                                       const QMap<XBinary::FPART_PROP, QVariant> &mapArchiveProperties)
{
    QString sResult;
    QXmlStreamWriter writer(&sResult);

    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("archive");
    writer.writeAttribute("name", QDir().toNativeSeparators(sArchiveName));
    writer.writeAttribute("fileType", XBinary::fileTypeIdToString(fileType));
    writer.writeAttribute("physicalSize", QString::number(nPhysicalSize));
    // ISSUE-25: see formatList().
    const QString sArchiveInfoXml = mapArchiveProperties.value(XBinary::FPART_PROP_INFO).toString().trimmed();
    if (!sArchiveInfoXml.isEmpty()) {
        writer.writeAttribute("info", sArchiveInfoXml);
    }

    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    qint32 nNumberOfFiles = 0;
    qint32 nNumberOfFolders = 0;

    writer.writeStartElement("records");

    for (qint32 i = 0; i < listRecords.count(); i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);

        if (isRecordFolder(record)) nNumberOfFolders++;
        else nNumberOfFiles++;

        if (isRecordSizePresent(record)) nTotalSize += getRecordSize(record);

        qint64 nPacked = 0;
        if (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE) nTotalPacked += nPacked;

        writer.writeStartElement("record");

        // The full property map, like the JSON encoder: XML has no trouble with
        // a schema that varies by container.
        QList<XBinary::FPART_PROP> listKeys = record.mapProperties.keys();
        std::sort(listKeys.begin(), listKeys.end());

        for (qint32 k = 0; k < listKeys.count(); k++) {
            const XBinary::FPART_PROP prop = listKeys.at(k);

            if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN) {
                continue;  // session bookkeeping, not archive metadata
            }

            // Property names are display text ("Compressed size", "#57"), which
            // is not a legal XML element name; carry it as an attribute and keep
            // the element generic.
            writer.writeStartElement("property");
            writer.writeAttribute("name", getPropertyName(prop));
            writer.writeCharacters(getPropertyValueString(record, prop));
            writer.writeEndElement();
        }

        writer.writeEndElement();
    }

    writer.writeEndElement();

    writer.writeStartElement("summary");
    writer.writeAttribute("numberOfFiles", QString::number(nNumberOfFiles));
    writer.writeAttribute("numberOfFolders", QString::number(nNumberOfFolders));
    writer.writeAttribute("totalSize", QString::number(nTotalSize));
    writer.writeAttribute("totalPacked", QString::number(nTotalPacked));
    writer.writeEndElement();

    writer.writeEndElement();
    writer.writeEndDocument();

    return sResult;
}

// One field, escaped for the delimited formats. CSV follows RFC 4180: a field
// containing the delimiter, a quote or a newline is quoted and its quotes
// doubled. TSV has no such standard, so the characters that would break a row
// are replaced rather than quoted.
static QString _xacDelimitedField(const QString &sValue, bool bTabSeparated)
{
    if (bTabSeparated) {
        QString sResult = sValue;
        sResult.replace(QChar('\t'), QChar(' '));
        sResult.replace(QChar('\r'), QChar(' '));
        sResult.replace(QChar('\n'), QChar(' '));
        return sResult;
    }

    if (sValue.contains(QChar(',')) || sValue.contains(QChar('"')) || sValue.contains(QChar('\n')) || sValue.contains(QChar('\r'))) {
        QString sQuoted = sValue;
        sQuoted.replace(QChar('"'), QStringLiteral("\"\""));
        return QChar('"') + sQuoted + QChar('"');
    }

    return sValue;
}

QString XArchiveConsole::formatListDelimited(const QList<XBinary::ARCHIVERECORD> &listRecords, bool bTabSeparated)
{
    // A fixed column set on purpose. The native table hides columns a container
    // does not populate, which is right for a human but would make every
    // archive produce a different CSV schema.
    const QChar cDelimiter = bTabSeparated ? QChar('\t') : QChar(',');
    QStringList listHeaders;
    listHeaders << "Name"
                << "Size"
                << "Packed"
                << "Ratio"
                << "Method"
                << "Checksum"
                << "Modified"
                << "Attributes"
                << "IsFolder"
                << "IsEncrypted";

    QString sResult = listHeaders.join(cDelimiter) + QLatin1String("\n");

    for (qint32 i = 0; i < listRecords.count(); i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);
        QStringList listFields;

        listFields << getRecordName(record);
        listFields << (isRecordSizePresent(record) ? QString::number(getRecordSize(record)) : QString());
        listFields << getRecordPackedString(record);
        listFields << getRecordRatio(record);
        listFields << XBinary::getHandleMethods(record.mapProperties);
        listFields << getRecordCRC(record);
        listFields << getRecordModified(record);
        listFields << getRecordAttr(record);
        listFields << (isRecordFolder(record) ? QStringLiteral("1") : QStringLiteral("0"));
        listFields << (record.mapProperties.value(XBinary::FPART_PROP_ENCRYPTED).toBool() ? QStringLiteral("1") : QStringLiteral("0"));

        QStringList listEscaped;

        for (const QString &sField : listFields) {
            listEscaped << _xacDelimitedField(sField, bTabSeparated);
        }

        sResult += listEscaped.join(cDelimiter) + QLatin1String("\n");
    }

    return sResult;
}
bool XArchiveConsole::hasAuthoritativeExternalStreamingReader(XBinary::FT fileType)
{
    switch (fileType) {
        case XBinary::FT_ZPAQ:
        case XBinary::FT_BCM:
        case XBinary::FT_LPAQ8:
        case XBinary::FT_PEA:
        case XBinary::FT_FREEARC: return true;
        default: return false;
    }
}

// Collect full archive records through the streaming unpack API so every
// property the format parser filled (method, timestamps, CRC, ownership, ...)
// is available, not just the handful the legacy flat RECORD struct carries.
QList<XBinary::ARCHIVERECORD> XArchiveConsole::getRecords(XBinary *pArchive, const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct,
                                                    bool *pbComplete, QMap<XBinary::FPART_PROP, QVariant> *pMapArchiveProperties)
{
    QList<XBinary::ARCHIVERECORD> listResult;

    if (pbComplete) {
        *pbComplete = false;
    }
    if (pMapArchiveProperties) {
        pMapArchiveProperties->clear();
    }

    if (!pArchive) {
        return listResult;
    }

    XBinary::UNPACK_STATE state = {};
    QMap<XBinary::UNPACK_PROP, QVariant> mapProperties = mapUnpackProperties;
    bool bInitialized = pArchive->initUnpack(&state, mapProperties, pPdStruct);

    const bool bPasswordSupplied = !mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString().isEmpty() ||
                                   !mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD_BYTES).toByteArray().isEmpty();
    if (!bInitialized && XBinary::isPdStructNotCanceled(pPdStruct) && !bPasswordSupplied && !hasAuthoritativeExternalStreamingReader(pArchive->getFileType())) {
        state = XBinary::UNPACK_STATE();
        mapProperties.insert(XBinary::UNPACK_PROP_METADATAONLY, true);
        bInitialized = pArchive->initUnpack(&state, mapProperties, pPdStruct);
    }

    if (bInitialized) {
        const qint32 nNumberOfRecords = state.nNumberOfRecords;
        bool bEnumerationComplete = (state.nCurrentIndex == 0) && (nNumberOfRecords >= 0) && (state.nCurrentIndex <= nNumberOfRecords);

        // The streaming contract advances only *between* declared records.
        // A single-record archive therefore finishes with currentIndex == 0;
        // moveToNext() returning false at that point is not an enumeration
        // failure.  Keep an independent bounded loop and validate that the
        // archive cannot silently change its declared count or cursor.
        for (qint32 i = 0; bEnumerationComplete && (i < nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct); ++i) {
            const qint32 nExpectedIndex = state.nCurrentIndex;
            const XBinary::ARCHIVERECORD record = pArchive->infoCurrent(&state, pPdStruct);
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || record.mapProperties.isEmpty() || !XBinary::isArchiveRecordExtentValid(record) ||
                (state.nCurrentIndex < 0) || (state.nCurrentIndex >= nNumberOfRecords) || (state.nCurrentIndex != nExpectedIndex) ||
                (state.nNumberOfRecords != nNumberOfRecords)) {
                bEnumerationComplete = false;
                break;
            }
            listResult.append(record);

            if (i + 1 < nNumberOfRecords) {
                const qint32 nPreviousIndex = state.nCurrentIndex;
                const bool bMoved = pArchive->moveToNext(&state, pPdStruct);
                if (!bMoved || !XBinary::isPdStructNotCanceled(pPdStruct) || (state.nCurrentIndex != (nPreviousIndex + 1)) || (state.nCurrentIndex >= nNumberOfRecords) ||
                    (state.nNumberOfRecords != nNumberOfRecords)) {
                    bEnumerationComplete = false;
                    break;
                }
            }
        }

        // finishUnpack() resets the state, so the archive-level properties
        // have to be taken before it runs.  Captured whether or not the
        // enumeration completed: a partial listing is exactly the case where
        // the archive's own note about its condition matters most.
        if (pMapArchiveProperties) {
            *pMapArchiveProperties = state.mapArchiveProperties;
        }

        const bool bFinished = pArchive->finishUnpack(&state, nullptr);
        bEnumerationComplete = bEnumerationComplete && bFinished && (listResult.size() == nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct);
        if (pbComplete) {
            *pbComplete = bEnumerationComplete;
        }
        if (!bEnumerationComplete) {
            listResult.clear();
        }
    }

    return listResult;
}

QString XArchiveConsole::getRecordName(const XBinary::ARCHIVERECORD &record)
{
    return record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
}

bool XArchiveConsole::isRecordFolder(const XBinary::ARCHIVERECORD &record)
{
    QString sName = getRecordName(record);

    return record.mapProperties.value(XBinary::FPART_PROP_ISFOLDER).toBool() || sName.endsWith(QChar('/')) || sName.endsWith(QChar('\\'));
}

bool XArchiveConsole::isRecordSizePresent(const XBinary::ARCHIVERECORD &record)
{
    if (!record.mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        return false;
    }

    bool bOk = false;
    const qint64 nSize = record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bOk);
    return bOk && (nSize >= 0);
}

qint64 XArchiveConsole::getRecordSize(const XBinary::ARCHIVERECORD &record)
{
    return isRecordSizePresent(record) ? record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() : 0;
}

// A record's stream coordinates are not always its own.  Two cases:
//   * an archive-stream record (HANDLE_METHOD_ARCHIVE_STREAM) publishes no
//     extent at all, because the member has no addressable extent on the
//     compressed device - the container's single stream holds every member;
//   * a solid record whose stream decodes to more than the member itself
//     (FPART_PROP_STREAMUNPACKEDSIZE > its uncompressed size) shares one
//     compressed stream with every other member of the block.
// In both cases nStreamSize is the container's number, not the member's, and
// printing it per row multiplies the archive size by the record count.
bool XArchiveConsole::isRecordSharingContainerStream(const XBinary::ARCHIVERECORD &record)
{
    qint32 nArchiveStreamIndex = -1;

    if (XBinary::getArchiveStreamRecordIndex(record, &nArchiveStreamIndex)) {
        return true;
    }

    if (record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() && record.mapProperties.contains(XBinary::FPART_PROP_STREAMUNPACKEDSIZE)) {
        return record.mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE).toLongLong() >
               record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    }

    return false;
}

// Report the member's own packed size, never a number that belongs to the
// container.  An honest "unknown" is preferable to a confident wrong total.
XArchiveConsole::PACKEDSTATE XArchiveConsole::getRecordPacked(const XBinary::ARCHIVERECORD &record, qint64 *pnPacked)
{
    if (pnPacked) *pnPacked = 0;

    // Directories have no payload, so they have no packed size - not zero, not
    // the container's size.
    if (isRecordFolder(record)) {
        return PACKEDSTATE_NONE;
    }

    if (record.mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE)) {
        if (pnPacked) *pnPacked = record.mapProperties.value(XBinary::FPART_PROP_COMPRESSEDSIZE).toLongLong();
        return PACKEDSTATE_VALUE;
    }

    if (isRecordSharingContainerStream(record)) {
        return PACKEDSTATE_UNKNOWN;
    }

    // Some static unpackers (including NSIS) publish one packed stream extent
    // on every solid member without a separate block id. Count that extent
    // before synthesizing zero for an empty member, otherwise a leading empty
    // member can claim the stream's deduplication key with a false zero.
    if (record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() && (record.nStreamSize > 0)) {
        if (pnPacked) *pnPacked = record.nStreamSize;
        return PACKEDSTATE_VALUE;
    }

    // Only an explicitly reported zero denotes an empty regular file. A
    // missing size is unknown, not an implicit zero. Prefer an explicit packed
    // size above because an empty payload can still have nonzero framing.
    if (isRecordSizePresent(record) && (getRecordSize(record) == 0)) {
        return PACKEDSTATE_VALUE;
    }

    if (record.nStreamSize > 0) {
        if (pnPacked) *pnPacked = record.nStreamSize;
        return PACKEDSTATE_VALUE;
    }

    return PACKEDSTATE_UNKNOWN;
}

QString XArchiveConsole::getRecordPackedString(const XBinary::ARCHIVERECORD &record)
{
    qint64 nPacked = 0;

    if (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE) {
        return QString::number(nPacked);
    }

    return QString();
}

// ISSUE-33.  A listing asserts that the container was understood, which is why
// a sweep that lists rather than extracts scores a misdetection as a success:
// the member count is there, the sizes are there, and nothing on screen says
// the reader guessed.  This names the one shape that refutes itself with no
// knowledge of the format - a record set that accounts for no bytes at all:
//
//   * every member declares a KNOWN uncompressed size, every one of them is
//     zero, and their packed sizes sum to zero as well, on a file that is not
//     empty.  Nothing in the listing accounts for a single byte of the input,
//     so "N file(s)" is a count of placeholders.  (Oracle Squeeze lists
//     "1 file(s), 0 Bytes -> 0 Bytes" on a 7,346-byte input.)
//   * every member declares a KNOWN packed size, every one of them is zero,
//     and the uncompressed total is not.  Zero compressed bytes cannot decode
//     into 7.58 MiB.  (Asymetrix: "59 file(s), 7.58 MiB -> 0 Bytes".)
//
// Every clause is one-sided and earns its place:
//   - "known" is required because a format that cannot supply a size reports it
//     ABSENT, not zero; the summary then prints "unknown" and those listings
//     are honest.  Testing the sum rather than counting zeros keeps a single
//     genuinely empty member among real ones unremarkable.
//   - the packed clause in the first test is what separates a contradiction
//     from a legitimately empty archive: the LZ4 empty-file test vector lists
//     "1 file(s), 0 Bytes -> 15 Bytes" and is correct, because it has real
//     packed bytes to point at.
//   - the packed sum is taken WITHOUT the solid-block de-duplication the
//     summary applies, which can only lower it, so a zero here is a zero there.
//
// The invariant proposed alongside this one - "a ratio far above 100% is a
// misdetection" - is deliberately NOT implemented, because it is false in this
// corpus.  Of the files that extract correctly, 68 list a total ratio at or
// above 200% and 31 at or above 400%: a Petite SFX reports 6626.1%,
// PowerPacker 10938.5%, an SZDD SFX 4646.0%, all of them producing correct
// output, because a static unpacker's packed size is the whole carrier while
// its uncompressed size is only the payload.  The known misdetection (an SFX
// misread as UPX, 3011.9%) sits BETWEEN two of those, so no threshold
// separates them.  Transport encodings expand for the same honest reason -
// BinHex 4.0 is 4/3, hence its 137.5%.
QString XArchiveConsole::describeSelfRefutingListing(const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize)
{
    qint32 nNumberOfFiles = 0;
    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    bool bSizeComplete = true;
    bool bPackedComplete = true;

    for (qint32 i = 0; i < listRecords.count(); i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);

        // A directory entry has no payload, so it owes the accounting nothing.
        if (isRecordFolder(record)) {
            continue;
        }

        nNumberOfFiles++;

        if (isRecordSizePresent(record)) {
            nTotalSize += getRecordSize(record);
        } else {
            bSizeComplete = false;
        }

        qint64 nPacked = 0;

        if (getRecordPacked(record, &nPacked) == PACKEDSTATE_VALUE) {
            nTotalPacked += nPacked;
        } else {
            bPackedComplete = false;
        }
    }

    // No members at all is ISSUE-21's case and is already reported there; an
    // empty file has nothing to contradict.
    if ((nNumberOfFiles <= 0) || (nPhysicalSize <= 0)) {
        return QString();
    }

    // NOT A CHECK: "every member declares zero size and zero packed bytes".
    // That was tried and REFUSED - it is not self-refuting, it is the normal
    // shape of an archive whose members are genuinely empty, and it fires on
    // correct listings AND correct extractions.  Counterexamples, measured:
    // libarchive's own lzop test vector (F:/ARC/ARC/LZOP/66_..._test_read_
    // filter_lzop.tar.lzo, 334 bytes) lists 6 empty members plus a directory
    // and extracts every one of them correctly; a stored ZIP of empty files
    // and a tar of empty marker files do the same.  Requiring bPackedComplete
    // does not rescue it: a stored-empty ZIP publishes an explicit packed 0 on
    // every record.  No listing-arithmetic threshold separates "the reader
    // produced nothing" from "every member is legitimately empty" - that has
    // to be settled in the reader.  See ISSUE-33.

    if (bPackedComplete && (nTotalPacked == 0) && bSizeComplete && (nTotalSize > 0)) {
        return QString("%1 member(s) declare %2 of content and 0 packed bytes to produce it from")
            .arg(nNumberOfFiles)
            .arg(XBinary::bytesCountToString(nTotalSize, 1024));
    }

    return QString();
}

QString XArchiveConsole::getRecordModified(const XBinary::ARCHIVERECORD &record)
{
    QDateTime dateTime;

    if (record.mapProperties.contains(XBinary::FPART_PROP_DATETIME)) {
        dateTime = record.mapProperties.value(XBinary::FPART_PROP_DATETIME).toDateTime();
    } else if (record.mapProperties.contains(XBinary::FPART_PROP_MTIME)) {
        dateTime = record.mapProperties.value(XBinary::FPART_PROP_MTIME).toDateTime();
    }

    if (dateTime.isValid()) {
        // absolute timestamps (7z FILETIME is UTC) are shown in the viewer's local
        // zone; local/wall-clock timestamps (DOS/ZIP) are already local, so
        // toLocalTime() leaves them unchanged
        return dateTime.toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    }

    return QString();
}

// Render a POSIX file mode as the familiar symbolic form plus octal, e.g.
// "-rwxr-xr-x (0755)".  Decimal mode values are unreadable on their own.
QString XArchiveConsole::getModeString(quint32 nMode, bool bIsFolder)
{
    QString sResult;

    quint32 nType = nMode & 0xF000u;  // S_IFMT

    if (nType == 0x4000u) sResult += QChar('d');       // S_IFDIR
    else if (nType == 0xA000u) sResult += QChar('l');  // S_IFLNK
    else if (nType == 0x8000u) sResult += QChar('-');  // S_IFREG
    else sResult += bIsFolder ? QChar('d') : QChar('-');

    const char cPerms[9] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};

    for (qint32 i = 0; i < 9; i++) {
        sResult += (nMode & (1u << (8 - i))) ? QChar(cPerms[i]) : QChar('-');
    }

    return QString("%1 (0%2)").arg(sResult).arg(nMode & 0777u, 0, 8);
}

QString XArchiveConsole::getRecordCRC(const XBinary::ARCHIVERECORD &record)
{
    XBinary::FPART_PROP prop = XBinary::FPART_PROP_UNKNOWN;

    if (record.mapProperties.contains(XBinary::FPART_PROP_RESULTCRC)) {
        prop = XBinary::FPART_PROP_RESULTCRC;
    } else if (record.mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDCRC)) {
        prop = XBinary::FPART_PROP_UNCOMPRESSEDCRC;
    }

    if (prop != XBinary::FPART_PROP_UNKNOWN) {
        return QString("%1").arg(record.mapProperties.value(prop).toULongLong(), 8, 16, QChar('0')).toUpper();
    }

    const QString sChecksum = record.mapProperties.value(XBinary::FPART_PROP_CHECKSUM).toString().trimmed();
    if (!sChecksum.isEmpty()) {
        return sChecksum.toUpper();
    }

    return QString();
}

QString XArchiveConsole::getRecordAttr(const XBinary::ARCHIVERECORD &record)
{
    QString sResult;

    // Match the technical-list convention used by UnRAR: R H A D S C I.
    // Compression/indexing are not currently exposed by the record model, so
    // those two positions remain dots.  Encryption is not a DOS attribute;
    // retain the app's existing '+' marker as an optional suffix.
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISREADONLY).toBool() ? QChar('R') : QChar('.');
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISHIDDEN).toBool() ? QChar('H') : QChar('.');
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISARCHIVE).toBool() ? QChar('A') : QChar('.');
    sResult += isRecordFolder(record) ? QChar('D') : QChar('.');
    sResult += record.mapProperties.value(XBinary::FPART_PROP_ISSYSTEM).toBool() ? QChar('S') : QChar('.');
    sResult += QStringLiteral("..");
    if (record.mapProperties.value(XBinary::FPART_PROP_ENCRYPTED).toBool()) {
        sResult += QChar('+');
    }

    return sResult;
}

QString XArchiveConsole::getRecordRatio(const XBinary::ARCHIVERECORD &record)
{
    if (isRecordFolder(record) || record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() || !isRecordSizePresent(record)) {
        return QString();
    }

    const qint64 nSize = getRecordSize(record);
    qint64 nPacked = 0;
    if ((nSize <= 0) || (getRecordPacked(record, &nPacked) != PACKEDSTATE_VALUE)) {
        return QString();
    }

    return QString("%1%").arg(QString::number((double)nPacked * 100.0 / (double)nSize, 'f', 1));
}

QString XArchiveConsole::getPropertyName(XBinary::FPART_PROP prop)
{
    QString sResult;

    if (prop == XBinary::FPART_PROP_ORIGINALNAME) sResult = "Name";
    else if (prop == XBinary::FPART_PROP_UNCOMPRESSEDSIZE) sResult = "Size";
    else if (prop == XBinary::FPART_PROP_COMPRESSEDSIZE) sResult = "Compressed size";
    else if (prop == XBinary::FPART_PROP_HANDLEMETHOD) sResult = "Method";
    else if (prop == XBinary::FPART_PROP_HANDLEMETHOD2) sResult = "Method 2";
    else if (prop == XBinary::FPART_PROP_DATETIME) sResult = "Modified";
    else if (prop == XBinary::FPART_PROP_MTIME) sResult = "MTime";
    else if (prop == XBinary::FPART_PROP_CTIME) sResult = "Created";
    else if (prop == XBinary::FPART_PROP_ATIME) sResult = "Accessed";
    else if (prop == XBinary::FPART_PROP_RESULTCRC) sResult = "CRC";
    else if (prop == XBinary::FPART_PROP_UNCOMPRESSEDCRC) sResult = "CRC";
    else if (prop == XBinary::FPART_PROP_CRC_TYPE) sResult = "CRC type";
    else if (prop == XBinary::FPART_PROP_ENCRYPTED) sResult = "Encrypted";
    else if (prop == XBinary::FPART_PROP_FILEMODE) sResult = "Mode";
    else if (prop == XBinary::FPART_PROP_USERNAME) sResult = "User";
    else if (prop == XBinary::FPART_PROP_GROUPNAME) sResult = "Group";
    else if (prop == XBinary::FPART_PROP_UID) sResult = "UID";
    else if (prop == XBinary::FPART_PROP_GID) sResult = "GID";
    else if (prop == XBinary::FPART_PROP_LINKNAME) sResult = "Link";
    else if (prop == XBinary::FPART_PROP_INFO) sResult = "Info";
    else if (prop == XBinary::FPART_PROP_ISFOLDER) sResult = "Folder";
    else if (prop == XBinary::FPART_PROP_ISSOLID) sResult = "Solid";
    else if (prop == XBinary::FPART_PROP_SOLIDFOLDERINDEX) sResult = "Solid block";
    else if (prop == XBinary::FPART_PROP_WINDOWSIZE) sResult = "Window size";
    else if (prop == XBinary::FPART_PROP_STREAMOFFSET) sResult = "Stream offset";
    else if (prop == XBinary::FPART_PROP_STREAMSIZE) sResult = "Stream size";
    else if (prop == XBinary::FPART_PROP_STREAMUNPACKEDSIZE) sResult = "Stream unpacked size";
    else if (prop == XBinary::FPART_PROP_SUBSTREAMOFFSET) sResult = "Substream offset";
    else if (prop == XBinary::FPART_PROP_FILEMD5) sResult = "File MD5";
    else if (prop == XBinary::FPART_PROP_FLAGS) sResult = "Flags";
    else if (prop == XBinary::FPART_PROP_TYPE) sResult = "Raw method";
    else if (prop == XBinary::FPART_PROP_COMPRESSPROPERTIES) sResult = "Compress properties";
    else if (prop == XBinary::FPART_PROP_ISREADONLY) sResult = "Read-only";
    else if (prop == XBinary::FPART_PROP_ISHIDDEN) sResult = "Hidden";
    else if (prop == XBinary::FPART_PROP_ISSYSTEM) sResult = "System";
    else if (prop == XBinary::FPART_PROP_ISARCHIVE) sResult = "Archive attribute";
    else if (prop == XBinary::FPART_PROP_ISCOMMENTPRESENT) sResult = "Has comment";
    else if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_INDEX) sResult = "Archive record index";
    else if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN) sResult = "Archive record token";
    else if (prop == XBinary::FPART_PROP_REPORTEDMETHOD) sResult = "Reported method";
    else if (prop == XBinary::FPART_PROP_HOSTOS) sResult = "Host OS";
    else if (prop == XBinary::FPART_PROP_CHECKSUM) sResult = "Checksum";
    else if (prop == XBinary::FPART_PROP_CHECKSUMTYPE) sResult = "Checksum type";
    else if (prop == XBinary::FPART_PROP_EXTRAFIELDOFFSET) sResult = "Extra field offset";
    else if (prop == XBinary::FPART_PROP_EXTRAFIELDLENGTH) sResult = "Extra field length";
    else if (prop == XBinary::FPART_PROP_FILECOMMENTOFFSET) sResult = "File comment offset";
    else if (prop == XBinary::FPART_PROP_FILECOMMENTLENGTH) sResult = "File comment length";
    else if (prop == XBinary::FPART_PROP_VERSIONMADEBY) sResult = "Version made by";
    else if (prop == XBinary::FPART_PROP_VERSIONNEEDED) sResult = "Version needed";
    else if (prop == XBinary::FPART_PROP_INTERNALATTRIBUTES) sResult = "Internal attributes";
    else if (prop == XBinary::FPART_PROP_EXTERNALATTRIBUTES) sResult = "External attributes";
    else if (prop == XBinary::FPART_PROP_ISUTF8NAME) sResult = "UTF-8 name";
    else if (prop == XBinary::FPART_PROP_HASDATADESCRIPTOR) sResult = "Data descriptor";
    else if (prop == XBinary::FPART_PROP_ISSTRONGENCRYPTED) sResult = "Strong encryption";
    else if (prop == XBinary::FPART_PROP_VERSION) sResult = "Version";
    else if (prop == XBinary::FPART_PROP_VERSIONCREATED) sResult = "Version created";
    else if (prop == XBinary::FPART_PROP_HOSTSYSTEM) sResult = "Host system";
    else if (prop == XBinary::FPART_PROP_EXTRAFIELD) sResult = "Extra field";
    else if (prop == XBinary::FPART_PROP_EXTRAFIELDLOCAL) sResult = "Extra field local";
    else if (prop == XBinary::FPART_PROP_FILECOMMENT) sResult = "File comment";
    else if (prop == XBinary::FPART_PROP_ENCODER) sResult = "Encoder";
    else sResult = QString("#%1").arg(static_cast<qint32>(prop));

    return sResult;
}

QString XArchiveConsole::getPropertyValueString(const XBinary::ARCHIVERECORD &record, XBinary::FPART_PROP prop)
{
    QVariant varValue = record.mapProperties.value(prop);

    if (prop == XBinary::FPART_PROP_HANDLEMETHOD) {
        const QString sMethod = XBinary::getHandleMethods(record.mapProperties);

        if (!sMethod.isEmpty()) {
            return sMethod;
        }
    } else if ((prop == XBinary::FPART_PROP_HANDLEMETHOD2) || (prop == XBinary::FPART_PROP_HANDLEMETHOD3)) {
        QMap<XBinary::FPART_PROP, QVariant> mapOne;
        mapOne.insert(XBinary::FPART_PROP_HANDLEMETHOD, varValue);
        QString sMethod = XBinary::getHandleMethods(mapOne);

        if (!sMethod.isEmpty()) {
            return sMethod;
        }
    }

    if ((prop == XBinary::FPART_PROP_RESULTCRC) || (prop == XBinary::FPART_PROP_UNCOMPRESSEDCRC)) {
        return QString("%1").arg(varValue.toULongLong(), 8, 16, QChar('0')).toUpper();
    }

    if ((prop == XBinary::FPART_PROP_DATETIME) || (prop == XBinary::FPART_PROP_MTIME) || (prop == XBinary::FPART_PROP_CTIME) || (prop == XBinary::FPART_PROP_ATIME)) {
        return varValue.toDateTime().toLocalTime().toString("yyyy-MM-dd hh:mm:ss");
    }

    if (prop == XBinary::FPART_PROP_FILEMODE) {
        return getModeString(varValue.toUInt(), isRecordFolder(record));
    }

    if ((prop == XBinary::FPART_PROP_COMPRESSPROPERTIES) || (prop == XBinary::FPART_PROP_COMPRESSPROPERTIES2)) {
        QByteArray baProps = varValue.toByteArray();
        QString sHex = QString(baProps.toHex());
        XBinary::FPART_PROP methodProp = (prop == XBinary::FPART_PROP_COMPRESSPROPERTIES) ? XBinary::FPART_PROP_HANDLEMETHOD : XBinary::FPART_PROP_HANDLEMETHOD2;
        XBinary::HANDLE_METHOD handleMethod = (XBinary::HANDLE_METHOD)record.mapProperties.value(methodProp).toUInt();
        QString sDecoded = XBinary::getCoderParamsString(handleMethod, baProps);

        if (!sDecoded.isEmpty()) {
            return QString("%1 (%2:%3)").arg(sHex, XBinary::handleMethodToString(handleMethod), sDecoded);
        }

        return sHex;
    }

    if (varValue.userType() == QMetaType::Bool) {
        return varValue.toBool() ? QString("Yes") : QString("No");
    }

    if (varValue.userType() == QMetaType::QByteArray) {
        return QString(varValue.toByteArray().toHex());
    }

    return varValue.toString();
}

QString XArchiveConsole::getCellValue(const XBinary::ARCHIVERECORD &record, qint32 nColId)
{
    if (nColId == 0) return getRecordAttr(record);
    if (nColId == 1) return getRecordModified(record);
    if (nColId == 2) return isRecordSizePresent(record) ? QString::number(getRecordSize(record)) : QString();
    if (nColId == 3) return getRecordPackedString(record);
    if (nColId == 4) return XBinary::getHandleMethods(record.mapProperties);
    if (nColId == 5) return getRecordCRC(record);
    if (nColId == 7) return getRecordRatio(record);

    return getRecordName(record);
}

// Human-readable, aligned archive listing with a format/size summary line and
// (when bVerbose) a full per-record property dump.  Only columns the format
// actually populates are shown, so each archive type surfaces its own metadata.
QString XArchiveConsole::formatList(XBinary::FT fileType, const QList<XBinary::ARCHIVERECORD> &listRecords, qint64 nPhysicalSize, bool bVerbose,
                                   const QMap<XBinary::FPART_PROP, QVariant> &mapArchiveProperties)
{
    QString sResult;

    qint32 nNumberOfRecords = listRecords.count();

    qint64 nTotalSize = 0;
    qint64 nTotalPacked = 0;
    qint32 nNumberOfFiles = 0;
    qint32 nNumberOfFolders = 0;
    bool bAnyModified = false;
    bool bAnyMethod = false;
    bool bAnyCRC = false;
    bool bAnyAttr = false;
    bool bAnyRatio = false;
    bool bSolid = false;
    bool bSizeComplete = true;
    bool bPackedComplete = true;
    QSet<qint64> stBlocks;

    // Solid formats report the same shared compressed stream on every member of
    // a block, so count each distinct stream region once for the total and blank
    // the repeated "Packed" cells (the compressed size belongs on the first
    // member of a folder only).  Only records that actually declare solid-block
    // membership are folded this way: readers that report offset 0 for every
    // member would otherwise collapse distinct members
    // of equal size into one and understate the total.
    QStringList listPackedDisplay;
    QSet<QString> stSeenStreams;
    QSet<qint64> stSeenPackedBlocks;
    QSet<qint64> stBlocksWithPackedValue;
    QSet<qint64> stBlocksWithUnknownMembers;
    bool bUnknownPackedWithoutBlock = false;

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        const XBinary::ARCHIVERECORD &record = listRecords.at(i);

        if (isRecordSizePresent(record)) {
            nTotalSize += getRecordSize(record);
        } else if (!isRecordFolder(record)) {
            bSizeComplete = false;
        }

        if (isRecordFolder(record)) {
            nNumberOfFolders++;
        } else {
            nNumberOfFiles++;
        }

        if (record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool()) {
            bSolid = true;
        }

        if (record.mapProperties.contains(XBinary::FPART_PROP_SOLIDFOLDERINDEX)) {
            stBlocks.insert(record.mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX).toLongLong());
        }

        qint64 nPacked = 0;
        PACKEDSTATE packedState = getRecordPacked(record, &nPacked);
        QString sPackedDisplay;
        const bool bHasBlock = record.mapProperties.contains(XBinary::FPART_PROP_SOLIDFOLDERINDEX);
        const qint64 nBlock = bHasBlock ? record.mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX).toLongLong() : -1;

        if (packedState == PACKEDSTATE_UNKNOWN) {
            if (bHasBlock) {
                stBlocksWithUnknownMembers.insert(nBlock);
            } else {
                bUnknownPackedWithoutBlock = true;
            }
        } else if (packedState == PACKEDSTATE_VALUE) {
            sPackedDisplay = QString::number(nPacked);

            const bool bDeclaresSolidBlock = record.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool() || bHasBlock;

            if (bHasBlock) {
                // getRecordPacked() synthesizes zero for an empty regular file.
                // That is a valid row value, but it is not evidence that the
                // provider supplied this solid folder's packed size.  Only an
                // actual packed-size/stream value can make unknown sibling
                // members complete.
                const bool bProviderPackedValue = record.mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE) || (record.nStreamSize > 0);

                if (bProviderPackedValue) {
                    stBlocksWithPackedValue.insert(nBlock);

                    if (stSeenPackedBlocks.contains(nBlock)) {
                        sPackedDisplay.clear();
                    } else {
                        stSeenPackedBlocks.insert(nBlock);
                        nTotalPacked += nPacked;
                    }
                }
            } else if (bDeclaresSolidBlock && (record.nStreamSize > 0)) {
                QString sStreamKey = QString("%1:%2").arg(record.nStreamOffset).arg(record.nStreamSize);

                if (stSeenStreams.contains(sStreamKey)) {
                    sPackedDisplay.clear();  // already counted: this file shares the block
                } else {
                    stSeenStreams.insert(sStreamKey);
                    nTotalPacked += nPacked;
                }
            } else {
                nTotalPacked += nPacked;
            }
        }
        // PACKEDSTATE_NONE (directory): no cell, no contribution, and no reason
        // to call the total incomplete.

        listPackedDisplay.append(sPackedDisplay);

        if (!getRecordModified(record).isEmpty()) bAnyModified = true;
        if (!XBinary::getHandleMethods(record.mapProperties).isEmpty()) bAnyMethod = true;
        if (!getRecordCRC(record).isEmpty()) bAnyCRC = true;
        if (isRecordFolder(record) || record.mapProperties.value(XBinary::FPART_PROP_ISREADONLY).toBool() ||
            record.mapProperties.value(XBinary::FPART_PROP_ISHIDDEN).toBool() || record.mapProperties.value(XBinary::FPART_PROP_ISSYSTEM).toBool() ||
            record.mapProperties.value(XBinary::FPART_PROP_ISARCHIVE).toBool() || record.mapProperties.value(XBinary::FPART_PROP_ENCRYPTED).toBool())
            bAnyAttr = true;
        if (!getRecordRatio(record).isEmpty()) bAnyRatio = true;
    }

    // Missing per-member sizes are complete when every such member belongs to
    // a block for which the provider reported one folder-level packed size.
    // This is the kpidPackSize/kpidBlock contract used by solid 7z and RAR.
    if (bUnknownPackedWithoutBlock) {
        bPackedComplete = false;
    }
    for (qint64 nBlock : stBlocksWithUnknownMembers) {
        if (!stBlocksWithPackedValue.contains(nBlock)) {
            bPackedComplete = false;
            break;
        }
    }

    // A packed total can never exceed the bytes that are actually on disk.  If
    // the per-record numbers add up to more than the file itself, they are not
    // this container's packed sizes; report that they are unknown rather than
    // publishing a sum (and a ratio) that the file cannot support.
    if (bPackedComplete && (nPhysicalSize > 0) && (nTotalPacked > nPhysicalSize)) {
        bPackedComplete = false;
    }

    QString sRatio;

    if (bSizeComplete && bPackedComplete && (nTotalSize > 0)) {
        sRatio = QString(" (%1%)").arg(QString::number(double(nTotalPacked) * 100.0 / double(nTotalSize), 'f', 1));
    }

    const QString sSizeSummary = bSizeComplete ? XBinary::bytesCountToString(nTotalSize, 1024) : QString("unknown");
    const QString sPackedSummary = bPackedComplete ? XBinary::bytesCountToString(nTotalPacked, 1024) : QString("unknown");

    sResult += QString("%1: %2 file(s)%3, %4 -> %5%6\n")
                   .arg(XBinary::fileTypeIdToString(fileType))
                   .arg(nNumberOfFiles)
                   .arg(nNumberOfFolders > 0 ? QString(", %1 folder(s)").arg(nNumberOfFolders) : QString())
                   .arg(sSizeSummary)
                   .arg(sPackedSummary)
                   .arg(sRatio);

    // archive-level info line: on-disk size, metadata overhead, solidity
    QStringList listInfo;

    if (nPhysicalSize > 0) {
        listInfo.append(QString("physical %1").arg(XBinary::bytesCountToString(nPhysicalSize, 1024)));

        qint64 nOverhead = bPackedComplete ? (nPhysicalSize - nTotalPacked) : 0;

        if (nOverhead > 0) {
            listInfo.append(QString("overhead %1").arg(XBinary::bytesCountToString(nOverhead, 1024)));
        }
    }

    if (bSolid) {
        listInfo.append(QString("solid"));

        if (stBlocks.count() > 0) {
            listInfo.append(QString("%1 block(s)").arg(stBlocks.count()));
        }
    }

    if (!listInfo.isEmpty()) {
        sResult += QString("  %1\n").arg(listInfo.join(", "));
    }

    // ISSUE-25: the archive's own note about itself.  Readers put a description
    // - and for a damaged container a warning - in FPART_PROP_INFO, which until
    // now only the GUI model rendered, so a truncated archive listed exactly
    // like a healthy one.  Suppressed when it merely repeats the format name
    // already on the summary line above, which would double the words on screen
    // for the many readers that write just that.
    const QString sArchiveInfo = mapArchiveProperties.value(XBinary::FPART_PROP_INFO).toString().trimmed();
    if (!sArchiveInfo.isEmpty() && (sArchiveInfo != XBinary::fileTypeIdToString(fileType))) {
        sResult += QString("  %1\n").arg(sArchiveInfo);
    }

    if (bVerbose) {
        for (qint32 i = 0; i < nNumberOfRecords; i++) {
            const XBinary::ARCHIVERECORD &record = listRecords.at(i);

            sResult += QString("\n[%1]\n").arg(getRecordName(record));

            QList<XBinary::FPART_PROP> listKeys = record.mapProperties.keys();
            std::sort(listKeys.begin(), listKeys.end());

            for (qint32 k = 0; k < listKeys.count(); k++) {
                XBinary::FPART_PROP prop = listKeys.at(k);

                if (prop == XBinary::FPART_PROP_ORIGINALNAME) {
                    continue;
                }

                // An opaque identity digest is bookkeeping between a record and
                // the session that produced it, not something the archive says
                // about this member.  A listing describes the member.
                if (prop == XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN) {
                    continue;
                }

                // The exact provider text is already rendered as Method via
                // getHandleMethods(); do not print a duplicate verbose field.
                if ((prop == XBinary::FPART_PROP_REPORTEDMETHOD) && record.mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD)) {
                    continue;
                }

                // A stream coordinate describes where a payload lives.  A
                // directory entry has no payload, so no coordinate on any
                // device describes it and none may be shown for it.
                if (isRecordFolder(record) && ((prop == XBinary::FPART_PROP_STREAMOFFSET) || (prop == XBinary::FPART_PROP_STREAMSIZE) ||
                                               (prop == XBinary::FPART_PROP_STREAMUNPACKEDSIZE) || (prop == XBinary::FPART_PROP_SUBSTREAMOFFSET))) {
                    continue;
                }

                // A record that shares the container's single compressed
                // stream has no extent of its own either; printing the shared
                // one per row presents a transport-envelope number as this
                // member's own metadata, which is how "Stream size: <whole
                // container>" ended up on every row of a compressed-tar
                // listing.
                if (((prop == XBinary::FPART_PROP_STREAMOFFSET) || (prop == XBinary::FPART_PROP_STREAMSIZE)) && isRecordSharingContainerStream(record)) {
                    continue;
                }

                sResult += QString("  %1: %2\n").arg(getPropertyName(prop), getPropertyValueString(record, prop));
            }
        }

        return sResult;
    }

    // choose the columns present for this format (Name/Size/Packed always shown)
    QList<qint32> listColIds;
    QStringList listHeaders;
    QList<bool> listRightAlign;

    if (bAnyAttr) {
        listColIds.append(0);
        listHeaders.append("Attr");
        listRightAlign.append(false);
    }

    if (bAnyModified) {
        listColIds.append(1);
        listHeaders.append("Modified");
        listRightAlign.append(false);
    }

    listColIds.append(2);
    listHeaders.append("Size");
    listRightAlign.append(true);

    listColIds.append(3);
    listHeaders.append("Packed");
    listRightAlign.append(true);

    if (bAnyRatio) {
        listColIds.append(7);
        listHeaders.append("Ratio");
        listRightAlign.append(true);
    }

    if (bAnyMethod) {
        listColIds.append(4);
        listHeaders.append("Method");
        listRightAlign.append(false);
    }

    if (bAnyCRC) {
        listColIds.append(5);
        listHeaders.append("Checksum");
        listRightAlign.append(false);
    }

    listColIds.append(6);
    listHeaders.append("Name");
    listRightAlign.append(false);

    qint32 nNumberOfColumns = listColIds.count();

    QVector<qint32> vWidths(nNumberOfColumns);

    for (qint32 c = 0; c < nNumberOfColumns; c++) {
        vWidths[c] = listHeaders.at(c).length();
    }

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        for (qint32 c = 0; c < nNumberOfColumns; c++) {
            qint32 nColId = listColIds.at(c);
            QString sCell = (nColId == 3) ? listPackedDisplay.at(i) : getCellValue(listRecords.at(i), nColId);
            qint32 nLength = sCell.length();

            if (nLength > vWidths[c]) {
                vWidths[c] = nLength;
            }
        }
    }

    QString sHeader;
    QString sSeparator;

    for (qint32 c = 0; c < nNumberOfColumns; c++) {
        if (c > 0) {
            sHeader += "  ";
            sSeparator += "  ";
        }

        bool bLastColumn = (c == (nNumberOfColumns - 1));

        if (listRightAlign.at(c)) {
            sHeader += listHeaders.at(c).rightJustified(vWidths[c], QChar(' '));
        } else if (bLastColumn) {
            sHeader += listHeaders.at(c);
        } else {
            sHeader += listHeaders.at(c).leftJustified(vWidths[c], QChar(' '));
        }

        sSeparator += QString(vWidths[c], QChar('-'));
    }

    sResult += sHeader + "\n";
    sResult += sSeparator + "\n";

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        QString sRow;

        for (qint32 c = 0; c < nNumberOfColumns; c++) {
            if (c > 0) {
                sRow += "  ";
            }

            qint32 nColId = listColIds.at(c);
            QString sCell = (nColId == 3) ? listPackedDisplay.at(i) : getCellValue(listRecords.at(i), nColId);
            bool bLastColumn = (c == (nNumberOfColumns - 1));

            if (listRightAlign.at(c)) {
                sRow += sCell.rightJustified(vWidths[c], QChar(' '));
            } else if (bLastColumn) {
                sRow += sCell;
            } else {
                sRow += sCell.leftJustified(vWidths[c], QChar(' '));
            }
        }

        sResult += sRow + "\n";
    }

    return sResult;
}

// ---------------------------------------------------------------------------
// Packing
//
// ZIP only, stored or deflated. Everything else this console does is read-only,
// so the writer is kept deliberately small and explicit: it emits local file
// headers, the central directory and the end-of-central-directory record, and
// refuses anything it cannot represent rather than producing an archive that
// only half-works.
// ---------------------------------------------------------------------------

// A stored name uses '/' and carries no drive letter or leading separator, so
// it can never escape the destination when it is extracted again.
static QString _xacArchiveName(const QDir &dirBase, const QString &sFilePath)
{
    QString sName = dirBase.relativeFilePath(sFilePath);
    sName.replace(QChar('\\'), QChar('/'));

    while (sName.startsWith(QLatin1String("./"))) {
        sName = sName.mid(2);
    }

    return sName;
}

// Read the entries back out of a manifest written by an extraction. The
// manifest fixes the member order, the stored name and the method, so packing
// from it reproduces the archive rather than merely re-creating one.
bool XArchiveConsole::collectManifestEntries(const QString &sManifestPath, QList<PACKENTRY> *pListEntries, QString *psComment, QString *psError)
{
    QFile manifestFile;
    manifestFile.setFileName(sManifestPath);

    if (!manifestFile.open(QIODevice::ReadOnly)) {
        if (psError) *psError = QString("Cannot open manifest: %1").arg(sManifestPath);
        return false;
    }

    QJsonParseError parseError = {};
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
    manifestFile.close();

    if (parseError.error != QJsonParseError::NoError) {
        if (psError) *psError = QString("Invalid manifest: %1").arg(parseError.errorString());
        return false;
    }

    // An extraction of several archives writes an array; one archive writes the
    // object itself.
    const QJsonObject jsonArchive = jsonDocument.isArray() ? jsonDocument.array().first().toObject() : jsonDocument.object();
    if (psComment) *psComment = jsonArchive.value("comment").toString();
    const QJsonArray jsonRecords = jsonArchive.value("records").toArray();

    if (jsonRecords.isEmpty()) {
        if (psError) *psError = QString("Manifest has no records: %1").arg(sManifestPath);
        return false;
    }

    for (qint32 i = 0; i < jsonRecords.count(); i++) {
        const QJsonObject jsonRecord = jsonRecords.at(i).toObject();

        const bool bIsFolder = jsonRecord.value("Folder").toBool();
        const QString sName = jsonRecord.value("Name").toString();
        const QString sSourcePath = jsonRecord.value("sourcePath").toString();

        if (sName.isEmpty()) {
            if (psError) *psError = QString("Manifest record %1 has no name").arg(i);
            return false;
        }

        // A directory entry has no file behind it, so it needs no sourcePath.
        if (!bIsFolder) {
            if (sSourcePath.isEmpty()) {
                if (psError) *psError = QString("Manifest record '%1' has no sourcePath").arg(sName);
                return false;
            }

            if (!QFileInfo::exists(sSourcePath)) {
                if (psError) *psError = QString("Cannot find: %1").arg(sSourcePath);
                return false;
            }
        }

        PACKENTRY entry;
        entry.bIsFolder = bIsFolder;
        entry.sSourcePath = sSourcePath;
        entry.sArchiveName = sName;
        entry.bMethodFromManifest = true;
        entry.bStore = (jsonRecord.value("Method").toString() == QLatin1String("Store"));
        entry.dtModified = QDateTime::fromString(jsonRecord.value("Modified").toString(), "yyyy-MM-dd hh:mm:ss");
        // Container header fields, so the repack reproduces the original rather
        // than substituting this build's defaults.
        entry.nFlags = (quint16)jsonRecord.value("Flags").toInt();
        entry.nVersionMadeBy = (quint16)jsonRecord.value("Version made by").toInt();
        entry.nVersionNeeded = (quint16)jsonRecord.value("Version needed").toInt();
        entry.nExternalAttributes = (quint32)jsonRecord.value("External attributes").toDouble();
        entry.nInternalAttributes = (quint16)jsonRecord.value("Internal attributes").toInt();
        // The byte-valued fields come back as hex, the way the listing renders
        // a QByteArray.
        entry.baExtraFieldCentral = QByteArray::fromHex(jsonRecord.value("Extra field").toString().toLatin1());
        entry.baExtraFieldLocal = QByteArray::fromHex(jsonRecord.value("Extra field local").toString().toLatin1());
        entry.baFileComment = QByteArray::fromHex(jsonRecord.value("File comment").toString().toLatin1());
        pListEntries->append(entry);
    }

    return true;
}

bool XArchiveConsole::collectPackEntries(const QStringList &listInputs, QList<PACKENTRY> *pListEntries, QString *psError)
{
    for (const QString &sInput : listInputs) {
        const QFileInfo fileInfo(sInput);

        if (!fileInfo.exists()) {
            if (psError) *psError = QString("Cannot find: %1").arg(sInput);
            return false;
        }

        if (fileInfo.isDir()) {
            // A directory contributes its files under its own name, so
            // "pack dir" produces "dir/..." rather than a flattened list.
            const QDir dirBase(fileInfo.absoluteFilePath() + QLatin1String("/.."));

            // An empty directory carries no file, so without an explicit entry
            // it would simply vanish from the archive.
            QDirIterator itDirs(fileInfo.absoluteFilePath(), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

            while (itDirs.hasNext()) {
                const QString sDirPath = itDirs.next();

                if (!QDir(sDirPath).isEmpty()) {
                    continue;  // its files already imply the directory
                }

                PACKENTRY entry;
                entry.bIsFolder = true;
                entry.sArchiveName = _xacArchiveName(dirBase, sDirPath) + QLatin1String("/");
                pListEntries->append(entry);
            }

            QDirIterator it(fileInfo.absoluteFilePath(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

            while (it.hasNext()) {
                const QString sFilePath = it.next();
                PACKENTRY entry;
                entry.sSourcePath = sFilePath;
                entry.sArchiveName = _xacArchiveName(dirBase, sFilePath);
                pListEntries->append(entry);
            }
        } else {
            PACKENTRY entry;
            entry.sSourcePath = fileInfo.absoluteFilePath();
            entry.sArchiveName = fileInfo.fileName();
            pListEntries->append(entry);
        }
    }

    return true;
}

XOptions::CR XArchiveConsole::createArchive(const COMMAND &command)
{
    if (command.listTargets.isEmpty()) {
        printf("Error: -c/--create requires the archive to create\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    if (command.listTargets.count() > 1) {
        printf("Error: -c/--create writes one archive; name it once\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    // The files come either from the operands or from a manifest, never both.
    if (command.listIncludes.isEmpty() && command.sManifest.isEmpty()) {
        printf("Error: -c/--create requires the files to add, or --manifest\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    if (!command.listIncludes.isEmpty() && !command.sManifest.isEmpty()) {
        printf("Error: -c/--create takes files or --manifest, not both\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    const QString sArchivePath = command.listTargets.at(0);

    QList<PACKENTRY> listEntries;
    QString sError;
    QString sArchiveComment;

    const bool bCollected = command.sManifest.isEmpty() ? collectPackEntries(command.listIncludes, &listEntries, &sError)
                                                        : collectManifestEntries(command.sManifest, &listEntries, &sArchiveComment, &sError);

    if (!bCollected) {
        printf("%s\n", sError.toUtf8().data());
        return XOptions::CR_CANNOTFINDFILE;
    }

    if (listEntries.isEmpty()) {
        printf("Error: nothing to add\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
    XBinary::disablePdStructDeadline(&pdStruct);

    QFile archiveFile;
    archiveFile.setFileName(sArchivePath);

    if (!archiveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        printf("Cannot create: %s\n", sArchivePath.toUtf8().data());
        return XOptions::CR_CANNOTOPENFILE;
    }

    // The container is written by XZip, which owns the ZIP layout: it stages
    // the source, computes the CRC, deflates through XArchive::_compress and
    // range-checks every offset against the 4 GB the format can express.
    QList<XZip::ZIPFILE_RECORD> listZipRecords;
    bool bFailed = false;

    for (qint32 i = 0; (i < listEntries.count()) && !bFailed; i++) {
        const PACKENTRY &entry = listEntries.at(i);

        QFile sourceFile;
        QBuffer emptyPayload;
        QIODevice *pSource = nullptr;

        if (entry.bIsFolder) {
            // A stored directory is a real member with a zero-length payload,
            // not a missing one.
            emptyPayload.open(QIODevice::ReadOnly);
            pSource = &emptyPayload;
        } else {
            sourceFile.setFileName(entry.sSourcePath);

            if (!sourceFile.open(QIODevice::ReadOnly)) {
                printf("Cannot open: %s\n", entry.sSourcePath.toUtf8().data());
                bFailed = true;
                break;
            }

            pSource = &sourceFile;
        }

        XZip::ZIPFILE_RECORD zipRecord = {};
        zipRecord.sFileName = entry.sArchiveName;
        const bool bStore = entry.bMethodFromManifest ? entry.bStore : (command.packMethod == PACKMETHOD_STORE);
        zipRecord.method = bStore ? XZip::CMETHOD_STORE : XZip::CMETHOD_DEFLATE;
        // A manifest pins the stored timestamp; without one it comes from the
        // file, which is what a fresh archive should record.
        zipRecord.dtTime = (entry.bMethodFromManifest && entry.dtModified.isValid()) ? entry.dtModified : QFileInfo(entry.sSourcePath).lastModified();

        if (entry.bMethodFromManifest) {
            zipRecord.nFlags = entry.nFlags;
            zipRecord.nVersion = (quint8)(entry.nVersionMadeBy & 0xFF);
            zipRecord.nOS = (quint8)((entry.nVersionMadeBy >> 8) & 0xFF);
            zipRecord.nMinVersion = (quint8)(entry.nVersionNeeded & 0xFF);
            zipRecord.nMinOS = (quint8)((entry.nVersionNeeded >> 8) & 0xFF);
            zipRecord.nExternalFileAttributes = entry.nExternalAttributes;
            zipRecord.nInternalFileAttributes = entry.nInternalAttributes;
            zipRecord.baExtraFieldLocal = entry.baExtraFieldLocal;
            zipRecord.baExtraFieldCentral = entry.baExtraFieldCentral;
            zipRecord.baFileComment = entry.baFileComment;
        }

        const bool bAdded = XZip::addLocalFileRecord(pSource, &archiveFile, &zipRecord, &pdStruct);
        sourceFile.close();
        emptyPayload.close();

        if (!bAdded) {
            printf("Cannot add: %s\n", entry.sArchiveName.toUtf8().data());
            bFailed = true;
            break;
        }

        listZipRecords.append(zipRecord);

        if (!command.bQuiet) {
            printf("  %s (%s)\n", entry.sArchiveName.toUtf8().data(), (zipRecord.method == XZip::CMETHOD_DEFLATE) ? "Deflate" : "Store");
        }
    }

    if (!bFailed && !XZip::addCentralDirectory(&archiveFile, &listZipRecords, sArchiveComment, &pdStruct)) {
        printf("Cannot write the central directory: %s\n", sArchivePath.toUtf8().data());
        bFailed = true;
    }

    archiveFile.close();

    if (bFailed) {
        QFile::remove(sArchivePath);  // never leave a half-written archive behind
        return XOptions::CR_CANNOTOPENFILE;
    }

    if (!command.bQuiet) {
        printf("Created %s with %lld file(s)\n", QDir().toNativeSeparators(sArchivePath).toUtf8().data(), static_cast<long long>(listZipRecords.count()));
    }

    return XOptions::CR_SUCCESS;
}

// ---------------------------------------------------------------------------
// XFormats-backed viewers
//
// None of these needs a signature database or a scan engine, so an archive
// front end can carry them itself instead of borrowing a scan-engine console.
// ---------------------------------------------------------------------------

static QString _xacRenderItems(const QVector<XBinary::KeyValueItem> &listItems, XArchiveConsole::RESULTFORMAT resultFormat)
{
    if (resultFormat == XArchiveConsole::RESULTFORMAT_JSON) return XFormats::toJSON(listItems);
    if (resultFormat == XArchiveConsole::RESULTFORMAT_XML) return XFormats::toXML(listItems);
    if (resultFormat == XArchiveConsole::RESULTFORMAT_CSV) return XFormats::toCSV(listItems);
    if (resultFormat == XArchiveConsole::RESULTFORMAT_TSV) return XFormats::toTSV(listItems);

    return XFormats::toFormattedString(listItems);
}

XOptions::CR XArchiveConsole::showFileInfo(const QString &sFileName, const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        QVector<XBinary::KeyValueItem> listItems = XFormats::getFileInfo(&file, false, -1, &pdStruct, command.fileType);

        // ISSUE-32: getFileInfo() probes with FT_FLAG_FORMATS, whose preference
        // order answers with the CARRIER - every SFX and installer reports
        // PE32/NE/MSDOS whether or not a reader recognises the container inside
        // it, so this line alone cannot be read as "what XFU recognised".  Ask
        // the same two-pass detector the listing path uses and publish its
        // answer; when it finds nothing, say so, because a MISSING line is what
        // let the carrier be taken for the verdict.  This reports DETECTION
        // only: a named container can still fail to open, and the exit code is
        // deliberately unchanged because -i is a viewer.
        if ((!listItems.isEmpty()) && file.seek(0)) {
            XBinary::PDSTRUCT containerPdStruct = XBinary::createPdStruct();
            XBinary::FT containerFileType = detectFileType(&file, command.fileType, true, &containerPdStruct);
            QString sContainer;

            if (XBinary::isPdStructDeadlineExpired(&containerPdStruct)) {
                // The budget ran out, so nothing was established.  Report that
                // rather than "none", which would be a verdict we did not reach.
                sContainer = QString("unknown (detection budget exceeded)");
            } else if (containerFileType != XBinary::FT_UNKNOWN) {
                sContainer = XBinary::fileTypeIdToString(containerFileType);
            } else {
                sContainer = QString("none recognised");
            }

            // A plain archive detects as itself and one line is enough there;
            // the Container line exists for the files where the two differ.
            qint32 nInsertIndex = listItems.count();

            for (qint32 i = 0; i < listItems.count(); i++) {
                if (listItems.at(i).key == QString("FileType")) {
                    nInsertIndex = (listItems.at(i).value.toString() == sContainer) ? -1 : (i + 1);
                    break;
                }
            }

            if (nInsertIndex >= 0) {
                XBinary::KeyValueItem itemContainer;
                itemContainer.key = QString("Container");
                itemContainer.value = sContainer;
                listItems.insert(nInsertIndex, itemContainer);
            }
        }

        printf("%s", _xacRenderItems(listItems, command.resultFormat).toUtf8().data());
        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

XOptions::CR XArchiveConsole::showFileEntropy(const QString &sFileName, const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        const QVector<XBinary::KeyValueItem> listItems = XFormats::getEntropy(&file, false, -1, &pdStruct);

        printf("%s", _xacRenderItems(listItems, command.resultFormat).toUtf8().data());
        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

XOptions::CR XArchiveConsole::showFileStruct(const QString &sFileName, const COMMAND &command)
{
    XOptions::CR result = XOptions::CR_SUCCESS;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        const XBinary::XFHEADER xFHeader = XFormats::getXFHeaderFromStructName(&file, command.sStruct, false, -1, &pdStruct);

        if (xFHeader.xfType != XBinary::XFTYPE_UNKNOWN) {
            XBinary *pBinary = XFormats::createClass(xFHeader.fileType, &file);

            if (pBinary) {
                QString sStructInfo;
                XBinary::INDATA inData = XFormats::createINDATA(xFHeader.fileType, &file);

                if (xFHeader.xfType == XBinary::XFTYPE_HEADER) {
                    XFModel_header modelHeader(nullptr);
                    modelHeader.setData(inData, xFHeader);

                    if (command.resultFormat == RESULTFORMAT_JSON) sStructInfo = modelHeader.toJSON();
                    else if (command.resultFormat == RESULTFORMAT_XML) sStructInfo = modelHeader.toXML();
                    else if (command.resultFormat == RESULTFORMAT_CSV) sStructInfo = XFModel::exportToString(&modelHeader, XFModel::EXPORT_CSV);
                    else if (command.resultFormat == RESULTFORMAT_TSV) sStructInfo = XFModel::exportToString(&modelHeader, XFModel::EXPORT_TSV);
                    else XOptions::printModel(&modelHeader);
                } else if (xFHeader.xfType == XBinary::XFTYPE_TABLE) {
                    XFModel_table modelTable;
                    modelTable.setData(inData, xFHeader);
                    modelTable.setShowPresentation(true);

                    if (command.resultFormat == RESULTFORMAT_JSON) sStructInfo = modelTable.toJSON();
                    else if (command.resultFormat == RESULTFORMAT_XML) sStructInfo = modelTable.toXML();
                    else if (command.resultFormat == RESULTFORMAT_CSV) sStructInfo = XFModel::exportToString(&modelTable, XFModel::EXPORT_CSV);
                    else if (command.resultFormat == RESULTFORMAT_TSV) sStructInfo = XFModel::exportToString(&modelTable, XFModel::EXPORT_TSV);
                    else XOptions::printModel(&modelTable);
                }

                if (!sStructInfo.isEmpty()) {
                    printf("%s", sStructInfo.toUtf8().data());
                }

                delete pBinary;
            } else {
                printf("Cannot read structure: %s\n", sFileName.toUtf8().data());
                result = XOptions::CR_CANNOTOPENFILE;
            }
        } else {
            printf("Cannot find struct '%s': %s\n", command.sStruct.toUtf8().data(), sFileName.toUtf8().data());
            result = XOptions::CR_INVALIDPARAMETER;
        }

        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

XOptions::CR XArchiveConsole::showStructsOverview(const COMMAND &command)
{
    if (command.listTargets.isEmpty()) {
        printf("Error: --showstructs requires <target>\n");
        return XOptions::CR_INVALIDPARAMETER;
    }

    XOptions::CR result = XOptions::CR_SUCCESS;
    const QString sFileName = command.listTargets.at(0);

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
        XBinary::FT fileType = command.fileType;

        if (fileType == XBinary::FT_UNKNOWN) {
            fileType = XFormats::getPrefFileType(&file, XBinary::FT_FLAG_FORMATS, &pdStruct);
        }

        XBinary *pBinary = XFormats::createClass(fileType, &file);

        if (pBinary) {
            const QList<XBinary::XFHEADER> listHeaders = pBinary->_getXFHeaders(&pdStruct);
            XBinary::INDATA inData = XFormats::createINDATA(fileType, &file);

            XFTreeModel treeModel(nullptr);
            treeModel.setData(inData, listHeaders);

            QString sStructs;

            if (command.resultFormat == RESULTFORMAT_JSON) sStructs = treeModel.toJSON();
            else if (command.resultFormat == RESULTFORMAT_XML) sStructs = treeModel.toXML();
            else if (command.resultFormat == RESULTFORMAT_CSV) sStructs = treeModel.toCSV();
            else if (command.resultFormat == RESULTFORMAT_TSV) sStructs = treeModel.toTSV();
            else sStructs = treeModel.toFormattedString();

            printf("%s", sStructs.toUtf8().data());

            delete pBinary;
        } else {
            printf("Cannot read structures: %s\n", sFileName.toUtf8().data());
            result = XOptions::CR_CANNOTOPENFILE;
        }

        file.close();
    } else {
        printf("Cannot open: %s\n", sFileName.toUtf8().data());
        result = XOptions::CR_CANNOTOPENFILE;
    }

    return result;
}

// ---------------------------------------------------------------------------
// Standalone console entry point
// ---------------------------------------------------------------------------

int XArchiveConsole::process(QCoreApplication &app, const QString &sDescription)
{
    // Text codecs (cp437 for DOS-era archive names, the legacy ANSI code pages
    // an installer may use) are needed by the format parsers.
    XOptions::registerCodecs();

    // 7-Zip and Info-ZIP command lines are answered first: their clustered and
    // attached-value switch forms are not expressible as Qt options, and
    // QCommandLineParser::process() would reject them outright.
    qint32 nDialectResult = XOptions::CR_SUCCESS;

    if (XLegacyConsole::process(*this, app.arguments(), &nDialectResult)) return nDialectResult;

    if (processForeignDialect(app.arguments(), &nDialectResult)) {
        return nDialectResult;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(sDescription);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("target", "The file or directory to open.");

    // addVersionOption() owns -v, so verbosity uses -b; addOptionChecked()
    // guarantees a letter that is already taken can never be dropped silently.
    if (!addOptions(&parser)) {
        return XOptions::CR_INVALIDPARAMETER;
    }

    parser.process(app);

    XOptions::CR crResult = XOptions::CR_SUCCESS;

    if (!applyOptions(&parser, &crResult)) {
        return crResult;
    }

    if (parser.isSet(m_clNoColor)) {
        XOptions::setNoColor(true);
    }

    COMMAND command;

    if (!buildCommand(&parser, &command, &crResult)) {
        return crResult;
    }

    if (command.verb == VERB_NONE) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    qint32 nResult = execute(command);

    if (m_bProbeTimeoutOccurred) {
        nResult = XOptions::CR_PROBETIMEOUT;
    }

    return nResult;
}
