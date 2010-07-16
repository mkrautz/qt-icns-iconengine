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

#include <qiconengineplugin.h>
#include <qstringlist.h>

#if !defined(QT_NO_IMAGEFORMATPLUGIN)

#include "qicnsiconengine.h"

#include <qiodevice.h>
#include <qbytearray.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

class QIcnsIconPlugin : public QIconEnginePluginV2
{
public:
    QStringList keys() const;
    QIconEngineV2 *create(const QString &filename = QString());
};

QStringList QIcnsIconPlugin::keys() const
{
    return QStringList(QLatin1String("icns"));
}

QIconEngineV2 *QIcnsIconPlugin::create(const QString &file)
{
    QIcnsIconEngine *engine = new QIcnsIconEngine;
    if (!file.isNull())
        engine->addFile(file, QSize(), QIcon::Normal, QIcon::Off);
    return engine;
}

Q_EXPORT_STATIC_PLUGIN(QIcnsIconPlugin)
Q_EXPORT_PLUGIN2(qicnsicon, QIcnsIconPlugin)

QT_END_NAMESPACE

#endif // !QT_NO_IMAGEFORMATPLUGIN
