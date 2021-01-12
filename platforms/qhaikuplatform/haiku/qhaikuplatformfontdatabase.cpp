/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2017 Gerasim Troeglazov,
** Contact: 3dEyes@gmail.com
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDir>
#include <QDirIterator>

#include "qhaikuintegration.h"
#include "qhaikuplatformfontdatabase.h"

#include <Path.h>
#include <PathFinder.h>
#include <String.h>
#include <StringList.h>

QFontEngineHaikuFT::QFontEngineHaikuFT(const QFontDef &fd)
    : QFontEngineFT(fd)
{
}

QFontEngineHaikuFT *QFontEngineHaikuFT::create(const QFontDef &fontDef, FaceId faceId, const QByteArray &fontData)
{
    QFontEngineHaikuFT::GlyphFormat format = QFontEngineHaikuFT::Format_A32;

    QScopedPointer<QFontEngineHaikuFT> engine(new QFontEngineHaikuFT(fontDef));

	bool subpixel = false;
	get_subpixel_antialiasing(&subpixel);

    if (!subpixel) {
        format = QFontEngineFT::Format_A8;
        engine->subpixelType = QFontEngine::Subpixel_None;
    } else {
        format = QFontEngineFT::Format_A32;
        engine->subpixelType = QFontEngine::Subpixel_RGB;
    }

    if (!engine->init(faceId, true, format, fontData) || engine->invalid()) {
        qWarning("QFontEngineHaikuFT: Failed to create FreeType font engine");
        return nullptr;
    }

	uint8 hinting = 1;
	get_hinting_mode(&hinting);

	if (hinting == 0)
		engine->setDefaultHintStyle(QFontEngineHaikuFT::HintNone);
	else if (hinting == 1)
		engine->setDefaultHintStyle(QFontEngineHaikuFT::HintFull);
	else if (hinting == 2 && fontDef.family.contains("Mono"))
		engine->setDefaultHintStyle(QFontEngineHaikuFT::HintFull);

	engine->default_load_flags = subpixel ? FT_LOAD_TARGET_LCD : FT_LOAD_TARGET_NORMAL;

    return engine.take();
}

QString QHaikuPlatformFontDatabase::fontDir() const
{
    return QLatin1String("/system/data/fonts/ttfonts");
}

void QHaikuPlatformFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();

    if (!QFile::exists(fontpath)) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
			qPrintable(fontpath));
    }

	BStringList fontPaths;
	BPathFinder::FindPaths(NULL, B_FIND_PATH_FONTS_DIRECTORY,
		NULL, B_FIND_PATH_EXISTING_ONLY, fontPaths);
	
	for (int32 i = 0; i < fontPaths.CountStrings(); i++) {
	    QDir dir(QLatin1String(fontPaths.StringAt(i).String()));
		QDirIterator qdi(dir.absolutePath(),
            QStringList() << "*.ttf" << "*.otf",
            QDir::Files, QDirIterator::Subdirectories);
    	while (qdi.hasNext()) {
			const QByteArray file = QFile::encodeName(qdi.next());
        	QStringList families = addTTFile(QByteArray(), file);
    	}
    }
}

QStringList QHaikuPlatformFontDatabase::fallbacksForFamily(const QString &family,
                                                             QFont::Style style,
                                                             QFont::StyleHint styleHint,
                                                             QChar::Script script) const
{
	QStringList result;
	if (styleHint == QFont::Monospace || styleHint == QFont::Courier) {
		result.append(QString::fromLatin1("Noto Sans Mono"));
		result.append(QString::fromLatin1("DejaVu Sans Mono"));
	} else {
		if (styleHint == QFont::Serif) {
			result.append(QString::fromLatin1("Noto Serif"));
			result.append(QString::fromLatin1("DejaVu Serif"));
		} else {
			result.append(QString::fromLatin1("Noto Sans Display"));
			result.append(QString::fromLatin1("DejaVu Sans"));
		}
	}
	result.append(QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script));
	return result;
}

QFont QHaikuPlatformFontDatabase::defaultFont() const
{
	QFont font(QStringLiteral("Noto Sans Display"));
	font.setStretch(QFont::Unstretched);
    return font;
}

QFontEngine *QHaikuPlatformFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
	if (!handle) // Happens if a font family population failed
		return 0;

    QFontEngine::FaceId faceId;
	FontFile *fontfile = static_cast<FontFile *>(handle);
	faceId.filename = fontfile->fileName.toLocal8Bit();
	faceId.index = fontfile->indexValue;

	QFontEngineHaikuFT *engine = QFontEngineHaikuFT::create(fontDef, faceId);

	return engine;
}

void QHaikuPlatformFontDatabase::releaseHandle(void *handle)
{
    if (!handle)
        return;
    QFreeTypeFontDatabase::releaseHandle(handle);
}
