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

#ifndef QICNSICONENGINE_H
#define QICNSICONENGINE_H

#include <QtGui/qiconengine.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QIcnsIconEnginePrivate;

class QIcnsIconEngine : public QIconEngineV2
{
public:
    QIcnsIconEngine();
    QIcnsIconEngine(const QIcnsIconEngine &other);
    ~QIcnsIconEngine();
    void paint(QPainter *painter, const QRect &rect,
               QIcon::Mode mode, QIcon::State state);
    QSize actualSize(const QSize &size, QIcon::Mode mode,
                     QIcon::State state);
    QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                   QIcon::State state);

    void addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                   QIcon::State state);
    void addFile(const QString &fileName, const QSize &size,
                 QIcon::Mode mode, QIcon::State state);

    QString key() const;
    QIconEngineV2 *clone() const;
    bool read(QDataStream &in);
    bool write(QDataStream &out) const;

private:
    QSharedDataPointer<QIcnsIconEnginePrivate> d;
};

QT_END_NAMESPACE

#endif
