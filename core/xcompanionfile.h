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
#ifndef XCOMPANIONFILE_H
#define XCOMPANIONFILE_H

#include <QIODevice>
#include <QString>

// Companion-file resolution for readers whose input is not self-contained: a
// key or catalogue that lives beside the archive (RPG Maker MV System.json,
// Visionaire vis.key, InstallShield DATA1.HDR), or the next volume of a split
// set.  One rule, so no reader grows its own: the companion is looked up in
// the directory of the source FILE only - exact name first, then a
// case-insensitive match - and a source that is not a named file, or a name
// carrying any path component, resolves to nothing.  A reader that gets an
// empty result must fail closed: guessing a companion is how a renamed sample
// fakes a successful extraction.
class XCompanionFile {
public:
    // Absolute path of the regular file behind pDevice, or empty when the
    // device is not backed by a named file.
    static QString sourcePath(QIODevice *pDevice);
    // Directory of the regular file behind pDevice, or empty.
    static QString sourceDirectory(QIODevice *pDevice);
    // Resolve sName beside the file behind pDevice.  Empty when the device has
    // no file, sName is not a bare name, or no readable regular file matches.
    static QString resolve(QIODevice *pDevice, const QString &sName);
    // Resolve sName inside sDirectory: exact, then case-insensitive.  Returns
    // the absolute path of a readable regular file, or empty.
    static QString resolveInDirectory(const QString &sDirectory, const QString &sName);
    // True when sName is a bare file name: no separators, no drive colon, and
    // neither "." nor "..".
    static bool isBareName(const QString &sName);
    // Resolve a forward-slash separated relative path of bare names (for
    // example "data/System.json") in the directory of the file behind pDevice
    // and then in up to nMaxLevels of its parent directories, nearest first;
    // every component is matched exact-then-case-insensitively.  This serves
    // the RPG Maker MV layout, where the key (www/data/System.json) lives in
    // a sibling tree of the resource (www/img/pictures/x.rpgmvp).  Empty when
    // nothing matches: the caller must fail closed, never guess a key.
    static QString resolveInAncestors(QIODevice *pDevice, const QString &sRelativePath, qint32 nMaxLevels = 4);
    // The same walk from an explicit start directory.
    static QString resolveInAncestorsOf(const QString &sDirectory, const QString &sRelativePath, qint32 nMaxLevels = 4);
};

#endif  // XCOMPANIONFILE_H
