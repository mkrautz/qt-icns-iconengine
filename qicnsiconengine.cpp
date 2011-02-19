/*
 * Qt icon engine for Mac OS X 'icns' files.
 * Copyright (C) 2010 Mikkel Krautz <mikkel@krautz.dk>
 *
 * Icon engine structure based on Qt's SVG icon engine
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * All rights reserved.
 * Contact: Nokia Corporation (qt-info@nokia.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "qicnsiconengine.h"

#include "qimagereader.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qpixmapcache.h"
#include "qstyle.h"
#include "qapplication.h"
#include "qstyleoption.h"
#include "qfileinfo.h"
#include <QAtomicInt>
#include "qdebug.h"
#include "qbuffer.h"
#include <QtEndian>

QT_BEGIN_NAMESPACE

static bool QSizeGreaterThan(const QSize &s1, const QSize &s2)
{
	return s1.width() > s2.width();
}

static bool QSizeLessThan(const QSize &s1, const QSize &s2)
{
	return s1.width() < s2.width();
}

class QIcnsIconEnginePrivate : public QSharedData
{
public:
    QIcnsIconEnginePrivate()
        { stepSerialNum(); }

    ~QIcnsIconEnginePrivate()
        {}

    static int hashKey(QIcon::Mode mode, QIcon::State state,
	                   QSize size = QSize())
		{ return ((size.width() & 0xff) << 24) |
		         ((size.height() & 0xff) << 16) |
			     ((mode & 0xff) << 8) | (state & 0xff); }

    QString pmcKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
        { return QLatin1String("$qt_icnsicon_")
                 + QString::number(serialNum, 16).append(QLatin1Char('_'))
                 + QString::number(hashKey(mode, state, size), 16); }

    void stepSerialNum()
        { serialNum = lastSerialNum.fetchAndAddRelaxed(1); }

	bool loadIcnsData(QIODevice *dev, QIcon::Mode mode, QIcon::State state);

	QByteArray decompressIconData(const QByteArray &rawIconData,
	                              int npixels) const;

	QPixmap bestPixmap(const QSize &size, QIcon::Mode state,
	                   QIcon::State state);

	QHash<int, QString> iconFiles;
    QHash<int, QByteArray> imageBuffers;
	QMultiHash<int, QSize> iconSizes;

    int serialNum;
    static QAtomicInt lastSerialNum;
};

QAtomicInt QIcnsIconEnginePrivate::lastSerialNum;

QIcnsIconEngine::QIcnsIconEngine()
    : d(new QIcnsIconEnginePrivate)
{
}

QIcnsIconEngine::QIcnsIconEngine(const QIcnsIconEngine &other)
    : QIconEngineV2(other), d(new QIcnsIconEnginePrivate)
{
	d->iconFiles = other.d->iconFiles;
	d->imageBuffers = other.d->imageBuffers;
}

QIcnsIconEngine::~QIcnsIconEngine()
{
}

QSize QIcnsIconEngine::actualSize(const QSize &size, QIcon::Mode mode,
                                  QIcon::State state)
{
    QPixmap pm = pixmap(size, mode, state);
    if (pm.isNull())
        return QSize();
    return pm.size();
}

QByteArray QIcnsIconEnginePrivate::decompressIconData(const QByteArray &in,
                                                      int npixels) const
{
	QByteArray out;

	/* Is this chunk even compressed? */
	if (in.length() == npixels*npixels*4)
		return in;

	const char *inbuf = in.data();
	out.resize(npixels * npixels * 4);

	for (int i = 2; i >= 0; i--) {
		char *outbuf = out.data() + i;
		int remain = npixels * npixels;
		while (remain > 0) {
			int count = 0;
			if (inbuf[0] & 0x80) {
				count = static_cast<unsigned char>(inbuf[0]) - 125;
				for (int j = 0; j < count; j++) {
					*outbuf = inbuf[1];
					outbuf += 4;
				}
				inbuf += 2;
			} else {
				count = static_cast<unsigned char>(inbuf[0]) + 1;
				for (int j = 0; j < count; j++) {
					*outbuf = inbuf[j + 1];
					outbuf += 4;
				}
				inbuf += count + 1;
			}
			remain -= count;
		}
	}

	return out;
}

bool QIcnsIconEnginePrivate::loadIcnsData(QIODevice *dev, QIcon::Mode mode,
                                          QIcon::State state)
{
	QByteArray id, data;
	qint32 length;

	QDataStream ds(dev);
	ds.setByteOrder(QDataStream::BigEndian);

	id = dev->read(4);
	if (!id.startsWith("icns"))
		return false;

	ds >> length;
	if (dev->isSequential() && length != dev->size())
		return false;

	while (! dev->atEnd()) {
		int npixels = 0;

		id = dev->read(4);
		ds >> length;
		data = dev->read(length - 8);

		if (id == "ic09")        /* 512x512 JPEG2000 */
			npixels = 512;
		else if (id == "ic08")   /* 256x256 JPEG2000 */
			npixels = 256;
		else if (id == "it32")   /* 128x128 24-bit RGB */
			npixels = 128;
		else if (id == "ih32")   /* 48x48 24-bit RGB */
			npixels = 48;
		else if (id == "il32")   /* 32x32 24-bit RGB */
			npixels = 32;
		else if (id == "is32")   /* 16x16 24-bit RGB */
			npixels = 16;
		else
			continue;

		/*
		 * JPEG2000-based icons.
		 */
		if (npixels > 128) {
			QList<QByteArray> formats = QImageReader::supportedImageFormats();
			if (!formats.contains(QByteArray("jp2")))
				continue;
		} else {
			/*
			 * Apparently, the 128x128 image can have 4 extra bytes prepended
			 * to them. GDK-pixbuf does this, too.
			 */
			if (npixels == 128) {
				if (data.startsWith("\0\0\0\0")) {
					data.remove(0, 4);
				}
			}

			QByteArray rgb = decompressIconData(data, npixels);

			/*
			 * Determine if there is an alpha channel for this image in
			 * the stream.
			 */
			id = dev->peek(4);
			if (id == "t8mk" || id == "h8mk" || id == "l8mk" || id == "s8mk") {
				id = dev->read(4);
				ds >> length;
				data = dev->read(length - 8);

				int length = data.length();

				if (length != npixels*npixels) {
					qWarning("QIcnsIconEnginePrivate::loadIcnsData: mask"
					         " size mismatch");
					continue;
				}

				for (int i = 0; i < length; i++)
					rgb[i*4 + 3] = data[i];
			}

			imageBuffers.insert(hashKey(mode, state, QSize(npixels, npixels)), rgb);
			iconSizes.insert(hashKey(mode, state), QSize(npixels, npixels));
		}
	}

	return dev->atEnd();
}

QPixmap QIcnsIconEnginePrivate::bestPixmap(const QSize &requestedSize,
                                           QIcon::Mode mode,
                                           QIcon::State state)
{
	QSize iconSize;
	int key;

	/*
	 * First, check if there are any "unprocessed" files for this
	 * mode and state combination.  If there are any, load the icons.
	 */
	key = hashKey(mode, state);
	if (iconFiles.contains(key)) {
		QString abs = iconFiles.value(key);
		QFile f(abs);
		if (f.open(QIODevice::ReadOnly)) {
			loadIcnsData(&f, mode, state);
		}
		iconFiles.remove(key);
	}

	/*
	 * Check if we have any matching values for the current
	 * mode and key combination.
	 *
	 * If we don't find any matches, it means that we weren't fed
	 * any icons specifically to use for this mode, and we're free
	 * to use QStyle's generatedIconPixmap() to generate an icon
	 * for the requested mode that matches the application's current
	 * style.
	 */
	if (! iconSizes.contains(key)) {
		mode = QIcon::Normal;
		state = QIcon::Off;
		key = hashKey(mode, state);
	}

	QList<QSize> sizes = iconSizes.values(key);
	if (!sizes.contains(requestedSize)) {
		qSort(sizes.begin(), sizes.end(), QSizeLessThan);
		foreach (const QSize &size, sizes) {
			if (QSizeGreaterThan(size, requestedSize)) {
				iconSize = size;
			}
		}
	} else {
		iconSize = requestedSize;
	}

	if (iconSize.isNull())
		return QPixmap();	

	QByteArray rgb = imageBuffers.value(hashKey(mode, state, iconSize));
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
	if (rgb.length() % 4)
		return QPixmap();
	quint32 *rgbbuf = reinterpret_cast<quint32 *>(rgb.data());
	for (int i = 0; i < rgb.length()/4; i++) {
		rgbbuf[i] = qFromLittleEndian(rgbbuf[i]);
	}
#endif

	if (!rgb.isNull()) {
		QImage img = QImage(reinterpret_cast<const uchar *>(rgb.constData()),
		                    iconSize.width(), iconSize.height(),
		                    QImage::Format_ARGB32);
		QImage scaled = img;
		if (iconSize != requestedSize)
			scaled = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

		QPixmap pm = QPixmap::fromImage(scaled);
		QStyleOption opt(0);
		opt.palette = QApplication::palette();

		QPixmap generated = QApplication::style()->generatedIconPixmap(mode, pm,
		                                                               &opt);
		if (!generated.isNull())
			pm = generated;

		if (!pm.isNull())
			QPixmapCache::insert(pmcKey(iconSize, mode, state), pm);

		return pm;
	}

	return QPixmap();
}

QPixmap QIcnsIconEngine::pixmap(const QSize &requestedSize, QIcon::Mode mode,
                                QIcon::State state)
{
	return d->bestPixmap(requestedSize, mode, state);
}

void QIcnsIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                                QIcon::State state)
{
	/* No-op. */
}

void QIcnsIconEngine::addFile(const QString &fileName, const QSize &size,
                              QIcon::Mode mode, QIcon::State state)
{
    if (!fileName.isEmpty()) {
        QString abs = fileName;
        if (fileName.at(0) != QLatin1Char(':'))
            abs = QFileInfo(fileName).absoluteFilePath();
        if (abs.endsWith(QLatin1String(".icns"), Qt::CaseInsensitive)) {
			d->stepSerialNum();
			d->iconFiles.insert(d->hashKey(mode, state), abs);
		}
    }
}

void QIcnsIconEngine::paint(QPainter *painter, const QRect &rect,
                            QIcon::Mode mode, QIcon::State state)
{
	painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
}

QString QIcnsIconEngine::key() const
{
	return QLatin1String("icns");
}

QIconEngineV2 *QIcnsIconEngine::clone() const
{
	return new QIcnsIconEngine(*this);
}

bool QIcnsIconEngine::read(QDataStream &in)
{
	return false;
}

bool QIcnsIconEngine::write(QDataStream &out) const
{
	return false;
}

QT_END_NAMESPACE
