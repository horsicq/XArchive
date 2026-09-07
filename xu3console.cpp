/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#include "xu3console.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <cstdio>
#include <limits>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString pathKey(const QString &path)
{
    QString result = QDir::cleanPath(QDir::fromNativeSeparators(path));
#ifdef Q_OS_WIN
    result = result.toCaseFolded();
#endif
    return result;
}
bool inside(const QString &path, const QString &root)
{
    const QString p = pathKey(path), r = pathKey(root);
    return !r.isEmpty() && (p == r || p.startsWith(r.endsWith('/') ? r : r + '/'));
}
bool linked(const QString &path)
{
#ifdef Q_OS_WIN
    const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(path).utf16()));
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    return QFileInfo(path).isSymLink();
#endif
}
QString safeTypeDirectory(QString value)
{
    for (qint32 i = 0; i < value.size(); ++i) {
        if (value.at(i).unicode() < 32 || QStringLiteral("\"*/:<>?\\|").contains(value.at(i))) value[i] = QLatin1Char('_');
    }
    while (value.endsWith(' ') || value.endsWith('.')) value.chop(1);
    return value.isEmpty() ? QStringLiteral("Unknown") : value;
}
class PropertyScope {
public:
    explicit PropertyScope(XArchiveConsole &console) : c(console), old(console.getUnpackProperties()) {}
    ~PropertyScope() { c.setUnpackProperties(old); }
    XArchiveConsole &c;
    QMap<XBinary::UNPACK_PROP, QVariant> old;
};
}

bool XU3Console::commandMatches(const QString &token, const char *alias)
{
    const QString name = QString::fromLatin1(alias);
    return token.compare(name, Qt::CaseInsensitive) == 0 ||
           (token.size() > 1 && (token.startsWith('-') || token.startsWith('/')) && token.mid(1).compare(name, Qt::CaseInsensitive) == 0);
}

XU3Console::OPTIONS XU3Console::parse(const QStringList &arguments)
{
    OPTIONS o;
    if (arguments.isEmpty() || arguments.size() > 3) { o.valid = false; return o; }
    if (arguments.size() > 1) o.source = arguments.at(1);
    if (arguments.size() > 2) o.destination = arguments.at(2);
    const QString first = arguments.first();
    if (commandMatches(first,"extract") || commandMatches(first,"e") || commandMatches(first,"x")) o.command = EXTRACT;
    else if (commandMatches(first,"x2")) o.command = EXTRACT_PATHS;
    else if (commandMatches(first,"x3")) { o.command = EXTRACT_PATHS; o.rename = true; }
    else if (commandMatches(first,"list") || commandMatches(first,"l") || commandMatches(first,"v")) o.command = LIST;
    else if (commandMatches(first,"type") || commandMatches(first,"r") || commandMatches(first,"w")) o.command = TYPE;
    else if (commandMatches(first,"sort") || commandMatches(first,"s")) o.command = SORT;
    else if (commandMatches(first,"test") || commandMatches(first,"t")) o.command = TEST;
    else if (commandMatches(first,"testlist") || commandMatches(first,"tl")) o.command = TESTLIST;
    else if (commandMatches(first,"testsort")) o.command = TESTSORT;
    else if (commandMatches(first,"help") || commandMatches(first,"h") || commandMatches(first,"?")) o.command = HELP;
    else {
        // U3 0079d5e9: unknown first token means an archive pathname. The third
        // argument is ignored in this branch, including a path beginning '-'.
        o.command = EXTRACT; o.source = first; o.destination = arguments.size() > 1 ? arguments.at(1) : QString();
    }
    if (o.command != HELP && o.source.isEmpty()) o.valid = false;
    return o;
}

bool XU3Console::collect(const QString &directory, const QString &relative, const QString &mask, const QString &excluded,
                        QList<TARGET> *targets, QSet<QString> *visited, int depth, QString *error)
{
    if (depth > 256 || targets->size() >= 262144) { *error = QStringLiteral("U3 batch traversal limit exceeded"); return false; }
    const QFileInfo directoryInfo(directory);
    const QString canonical = directoryInfo.canonicalFilePath();
    if (canonical.isEmpty() || linked(directory)) { *error = QStringLiteral("Cannot traverse directory: %1").arg(directory); return false; }
    if (!excluded.isEmpty() && inside(canonical, excluded)) return true;
    const QString key = pathKey(canonical);
    if (visited->contains(key)) return true;
    visited->insert(key);
    struct ENTRY { QString name; bool directory; bool reparse; };
    QList<ENTRY> entries;
#ifdef Q_OS_WIN
    // U3 0079cc70 -> 004158c0 -> 00412570 uses the same FindFirstFileW mask
    // for files AND directories. A '*.zip' mask therefore recurses only into
    // directory names matching '*.zip'; '*' traverses every ordinary directory.
    WIN32_FIND_DATAW data = {};
    const QString search = QDir::toNativeSeparators(QDir(directory).filePath(mask));
    HANDLE handle = FindFirstFileW(reinterpret_cast<LPCWSTR>(search.utf16()), &data);
    if (handle == INVALID_HANDLE_VALUE) {
        const DWORD code = GetLastError();
        if (code == ERROR_FILE_NOT_FOUND || code == ERROR_NO_MORE_FILES) return true;
        *error = QStringLiteral("Cannot enumerate directory: %1 (system error %2)").arg(directory).arg(code); return false;
    }
    do {
        const QString name = QString::fromWCharArray(data.cFileName);
        if (name != "." && name != "..") {
            if (entries.size() >= 262144) { FindClose(handle); *error = QStringLiteral("U3 directory entry limit exceeded"); return false; }
            entries.append({name,(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0,(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0});
        }
    } while (FindNextFileW(handle, &data));
    const DWORD code = GetLastError();
    FindClose(handle);
    if (code != ERROR_NO_MORE_FILES) { *error = QStringLiteral("Directory enumeration failed: %1").arg(directory); return false; }
#else
    const QFileInfoList files = QDir(directory).entryInfoList({mask}, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDir::NoSort);
    for (const QFileInfo &file : files) entries.append({file.fileName(),file.isDir(),file.isSymLink()});
#endif
    for (const ENTRY &entry : entries) {
        if (entry.reparse) continue;
        const QString path = QDir(directory).filePath(entry.name);
        if (entry.directory) {
            const QString childRelative = relative.isEmpty() ? entry.name : relative + '/' + entry.name;
            if (!collect(path,childRelative,mask,excluded,targets,visited,depth+1,error)) return false;
        } else {
            if (targets->size() >= 262144) { *error = QStringLiteral("U3 batch target limit exceeded"); return false; }
            targets->append({path,relative});
        }
    }
    return true;
}

bool XU3Console::prepare(const OPTIONS &options, PATHS *paths, QString *error)
{
    if (!paths || !error || !options.valid || options.source.isEmpty()) return false;
    *paths = PATHS();
    const QString input = QDir::cleanPath(QDir().absoluteFilePath(QDir::fromNativeSeparators(options.source)));
    QFileInfo source(input);
    paths->sourceDirectory = source.isDir() ? input : source.absolutePath();
    paths->mask = source.isDir() ? QStringLiteral("*") : source.fileName();
    if (paths->mask.isEmpty()) paths->mask = QStringLiteral("*");
    paths->pattern = paths->mask.contains('*') || paths->mask.contains('?');
    paths->destination = QDir::cleanPath(QDir().absoluteFilePath(options.destination.isEmpty() ? QStringLiteral("Output") : options.destination));
    if (!QFileInfo(paths->sourceDirectory).isDir()) { *error = QStringLiteral("Cannot find source directory: %1").arg(paths->sourceDirectory); return false; }
    if (!paths->pattern) {
        if (!source.isFile() || linked(input)) { *error = QStringLiteral("Cannot open ordinary file: %1").arg(input); return false; }
        paths->targets.append({input,QString()}); return true;
    }
    const bool mutating = options.command == SORT || options.command == TESTSORT || options.command == EXTRACT || options.command == EXTRACT_PATHS;
    QString excluded;
    if (mutating) {
        excluded = QFileInfo(paths->destination).canonicalFilePath();
        if (excluded.isEmpty()) excluded = paths->destination;
        const QString canonicalSource = QFileInfo(paths->sourceDirectory).canonicalFilePath();
        const bool sorting = options.command == SORT || options.command == TESTSORT;
        // The complete target list is collected before any mutation. An output
        // directory above the source must not suppress that source; an equal
        // extraction destination must reach ordinary file/directory handling.
        // Preserve U3's exact-destination exclusion for sort/testsort (0079d000).
        if (inside(canonicalSource, excluded) && (!sorting || pathKey(canonicalSource) != pathKey(excluded))) excluded.clear();
    }
    QSet<QString> visited;
    // Snapshot the target list before any output or move, so newly produced files
    // cannot enter the same batch and an output subtree is never visited twice.
    return collect(paths->sourceDirectory,QString(),paths->mask,excluded,&paths->targets,&visited,0,error);
}

QString XU3Console::batchDestination(const OPTIONS &o, const PATHS &p, const TARGET &t)
{
    if (!p.pattern) return p.destination;
    QString relative = QFileInfo(t.file).fileName();
    if (o.command == EXTRACT_PATHS && !t.relativeDirectory.isEmpty()) relative = t.relativeDirectory + '/' + relative;
    return QDir(p.destination).filePath(relative);
}

XOptions::CR XU3Console::sortFile(const QString &source, const QString &destination, XBinary::FT type)
{
    const QString typeDirectory = QDir(destination).filePath(safeTypeDirectory(XBinary::fileTypeIdToString(type)));
    if (linked(destination) || linked(typeDirectory) || !QDir().mkpath(typeDirectory)) {
        fprintf(stderr,"Cannot create sort directory: %s\n",qPrintable(typeDirectory)); return XOptions::CR_CANNOTOPENFILE;
    }
    const QString canonicalRoot = QFileInfo(destination).canonicalFilePath();
    const QString canonicalType = QFileInfo(typeDirectory).canonicalFilePath();
    if (!inside(canonicalType,canonicalRoot) || linked(source)) return XOptions::CR_CANNOTOPENFILE;
    const QString base = QDir(canonicalType).filePath(QFileInfo(source).fileName());
    QString candidate = base;
    for (quint64 attempt = 1; QFileInfo::exists(candidate) || linked(candidate); ++attempt) {
        if (attempt > quint64((std::numeric_limits<qint32>::max)())) return XOptions::CR_CANNOTOPENFILE;
        candidate = base + '.' + QString::number(attempt);
    }
    // QFile::rename never replaces an existing destination. Its cross-volume
    // fallback retains the source if copying fails; no separate delete loop.
    QFile input(source);
    if (!input.rename(candidate)) {
        fprintf(stderr,"Cannot move %s -> %s: %s\n",qPrintable(source),qPrintable(candidate),qPrintable(input.errorString()));
        return XOptions::CR_CANNOTOPENFILE;
    }
    printf("%s -> %s\n",qPrintable(source),qPrintable(candidate));
    return XOptions::CR_SUCCESS;
}

XOptions::CR XU3Console::run(XArchiveConsole &console, const OPTIONS &options, const PATHS &paths)
{
    PropertyScope scope(console);
    auto properties = console.getUnpackProperties();
    properties.insert(XBinary::UNPACK_PROP_DISK_FILESYSTEM,true);
    properties.insert(XBinary::UNPACK_PROP_TRANSPORT_ONLY,true);
    properties.insert(XBinary::UNPACK_PROP_U3_RENAME,options.rename);
    console.setUnpackProperties(properties);
    XOptions::CR result = XOptions::CR_SUCCESS;
    for (const TARGET &target : paths.targets) {
        XArchiveConsole::COMMAND command;
        command.listTargets = QStringList(target.file);
        command.sOutputDirectory = batchDestination(options,paths,target);
        command.overwrite = options.rename ? XArchiveConsole::OVERWRITE_RENAME : XArchiveConsole::OVERWRITE_SKIP;
        XOptions::CR current = XOptions::CR_SUCCESS;
        if (options.command == TYPE || options.command == SORT || options.command == TESTSORT) {
            if (options.command == TESTSORT) {
                command.verb = XArchiveConsole::VERB_TEST;
                current = console.execute(command);
                if (current != XOptions::CR_SUCCESS) { result = current; continue; }
            }
            QFile file(target.file);
            if (!file.open(QIODevice::ReadOnly)) { result = XOptions::CR_CANNOTOPENFILE; continue; }
            XBinary::PDSTRUCT pd = XBinary::createPdStruct();
            const XBinary::FT type = console.detectFileType(&file,XBinary::FT_UNKNOWN,true,&pd);
            const bool expired = XBinary::isPdStructDeadlineExpired(&pd);
            file.close();
            if (expired) { console.m_bProbeTimeoutOccurred = true; result = XOptions::CR_PROBETIMEOUT; continue; }
            // U3 type/sort omit unrecognized inputs; they are never moved merely
            // because their filename has a familiar suffix.
            if (type == XBinary::FT_UNKNOWN || (!XFormats::isArchive(type) && !XFormats::isStaticUnpacker(type))) continue;
            if (options.command == TYPE) printf("%s: %s\n",qPrintable(XBinary::fileTypeIdToString(type)),qPrintable(target.file));
            else current = sortFile(target.file,paths.destination,type);
        } else {
            if (options.command == LIST || options.command == TESTLIST) {
                command.verb = XArchiveConsole::VERB_LIST; current = console.execute(command);
            }
            if (options.command == TEST || options.command == TESTLIST) {
                command.verb = XArchiveConsole::VERB_TEST;
                const XOptions::CR tested = console.execute(command);
                if (tested != XOptions::CR_SUCCESS) current = tested;
            }
            if (options.command == EXTRACT || options.command == EXTRACT_PATHS) {
                command.verb = XArchiveConsole::VERB_EXTRACT; current = console.execute(command);
            }
        }
        if (current != XOptions::CR_SUCCESS) result = current;
    }
    return console.isProbeTimeoutOccurred() ? XOptions::CR_PROBETIMEOUT : result;
}

bool XU3Console::process(XArchiveConsole &console, const QStringList &arguments, qint32 *result)
{
    if (arguments.size() < 2 || arguments.at(1) != "--u3") return false;
    if (!result) return false;
    const OPTIONS options = parse(arguments.mid(2));
    if (!options.valid || options.command == HELP) {
        printf("%s",help().toUtf8().constData());
        *result = options.valid ? XOptions::CR_SUCCESS : XOptions::CR_INVALIDPARAMETER; return true;
    }
    PATHS paths; QString error;
    if (!prepare(options,&paths,&error)) {
        fprintf(stderr,"%s\n",qPrintable(error)); *result = XOptions::CR_CANNOTFINDFILE; return true;
    }
    *result = run(console,options,paths); return true;
}

QString XU3Console::help()
{
    return QStringLiteral(
        "XFU U3 command adapter\n"
        "Usage: xfileunpackerc --u3 [command] archive-or-pattern [destination]\n"
        "Commands (case-insensitive; one '-' or '/' prefix allowed):\n"
        "  type/r/w       recognize archive type\n"
        "  list/l/v       list entries\n"
        "  test/t         verify without writing members\n"
        "  testlist/tl    list and verify\n"
        "  sort/s         MOVE recognized originals into destination/type\n"
        "  testsort       verify, then MOVE successfully tested originals\n"
        "  extract/e/x    extract (default command)\n"
        "  x2             preserve source-relative directories in batch mode\n"
        "  x3             x2 plus U3 '(0)name'..'(999)name' collision renaming\n"
        "  help/h/?       this help\n"
        "Default destination: ./Output. Batch archive directory names retain extensions.\n"
        "Use '*' for recursive traversal; U3 applies the mask to directory names too.\n"
        "Interactive overwrite prompts are not implemented: extract/x2 preserve existing files.\n"
        "Formatting/type labels and exit codes use XFU; supported guest filesystem mode is enabled.\n");
}
