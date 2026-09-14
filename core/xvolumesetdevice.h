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
#ifndef XVOLUMESETDEVICE_H
#define XVOLUMESETDEVICE_H

#include <QIODevice>
#include <QList>
#include <QPointer>
#include <QString>

// One read-only, random-access view over an ordered list of byte ranges taken
// from several devices: the segments of a split ZIP set (name.z01, name.z02,
// ..., name.zip), the volumes of a 7-Zip .001/.002 set, or the pieces of an
// ARJ member continued across volumes.  A reader resolves the companions
// beside its source file (XCompanionFile), appends them here in order, and
// binds the set with XBinary::setDevice() so every offset it publishes is a
// plain offset into the joined stream.
//
// The set never materialises the joined bytes.  Segments are read on demand
// and a segment whose device shrank below its declared range fails the read
// rather than returning short data.  Files appended with appendFile() are
// owned by the set; devices appended with appendSegment() are not.
//
// XArchive treats the set as a generic device: retained unpack sessions
// fingerprint its complete logical contents (see captureSourceDeviceSnapshot),
// which is correct and simply proportional to the joined size.
class XVolumeSetDevice : public QIODevice {
    Q_OBJECT

public:
    explicit XVolumeSetDevice(QObject *pParent = nullptr);
    virtual ~XVolumeSetDevice();

    // nLength bytes of pDevice starting at nOffset become the next segment.
    // pDevice must be open, readable and random-access; it is not owned.
    bool appendSegment(QIODevice *pDevice, qint64 nOffset, qint64 nLength);
    // Opens sFileName read-only, owns it, and appends [nOffset, nOffset+nLength)
    // of it (nLength < 0 means "to the end of the file").
    bool appendFile(const QString &sFileName, qint64 nOffset = 0, qint64 nLength = -1);

    qint32 getNumberOfSegments() const;
    // Logical offset at which segment nIndex begins, -1 when out of range.
    qint64 getSegmentStart(qint32 nIndex) const;
    qint64 getSegmentLength(qint32 nIndex) const;
    QIODevice *getSegmentDevice(qint32 nIndex) const;
    // Logical offset of byte nOffsetInSegment of segment nIndex, -1 when the
    // pair does not address a byte inside that segment (the end position of
    // the segment is accepted, so a zero-length range at its end resolves).
    qint64 toLogicalOffset(qint32 nIndex, qint64 nOffsetInSegment) const;
    // True when any segment device aliases pDevice (XBinary::devicesAlias),
    // so an unpack output can never be one of the inputs.
    bool aliases(QIODevice *pDevice) const;

    virtual bool open(OpenMode mode) override;
    virtual qint64 size() const override;
    virtual bool isSequential() const override;
    virtual bool seek(qint64 nPos) override;
    virtual bool atEnd() const override;

protected:
    virtual qint64 readData(char *pData, qint64 nMaxSize) override;
    virtual qint64 writeData(const char *pData, qint64 nMaxSize) override;

private:
    struct SEGMENT {
        QPointer<QIODevice> pDevice;
        qint64 nOffset;
        qint64 nLength;
        qint64 nStart;
    };

    qint32 segmentIndexForPosition(qint64 nPosition) const;

    QList<SEGMENT> m_listSegments;
    qint64 m_nSize;
    qint64 m_nPosition;
};

#endif  // XVOLUMESETDEVICE_H
