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

#include "qhaikuplatformdialoghelpers.h"

#include <QTextDocument>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#include <QtWidgets/qmessagebox.h>
#include <qdebug.h>

#include <Alert.h>
#include <TextView.h>

namespace QtHaikuDialogHelpers {

QHaikuPlatformMessageDialogHelper::QHaikuPlatformMessageDialogHelper()
    :m_buttonId(-1)
    ,m_shown(false)
{
}

void QHaikuPlatformMessageDialogHelper::exec()
{
    if (!m_shown)
        show(Qt::Dialog, Qt::ApplicationModal, 0);
}

static QString htmlText(QString text)
{
    if (Qt::mightBeRichText(text))
        return text;
    text.remove(QLatin1Char('\r'));
    return text.toHtmlEscaped().replace(QLatin1Char('\n'), QLatin1String("<br />"));
}

bool QHaikuPlatformMessageDialogHelper::show(Qt::WindowFlags windowFlags
                                         , Qt::WindowModality windowModality
                                         , QWindow *parent)
{
    QSharedPointer<QMessageDialogOptions> options = this->options();
    if (!options.data())
        return false;

    const QString informativeText = options->informativeText();
    const QString title = options->windowTitle();
    const QString text = informativeText.isEmpty() ? options->text() : (options->text() + QLatin1Char('\n') + informativeText);
    
    alert_type type = B_INFO_ALERT;
    switch (options->icon()) {
    	case QMessageBox::NoIcon:
    		type = B_EMPTY_ALERT;
    		break;
    	case QMessageBox::Information:
    		type = B_INFO_ALERT;
    		break;
		case QMessageBox::Warning:
			type = B_WARNING_ALERT;
			break;
		case QMessageBox::Critical:
			type = B_STOP_ALERT;
			break;			
		case QMessageBox::Question:
			type = B_INFO_ALERT; //?
			break;
		default:
			type = B_INFO_ALERT;
			break;			
    }    
	
	QTextDocument doc;
	doc.setHtml(text);

	int buttons = options->standardButtons();
    if (buttons == 0)
        buttons = QPlatformDialogHelper::Ok;

	const char *nativeButtonText[3] = {NULL, NULL, NULL};
	int	buttonsIds[3] = {0, 0, 0};
	int32 buttonsCount = 0;
	
    for (int i = QPlatformDialogHelper::FirstButton; i < QPlatformDialogHelper::LastButton; i<<=1) {
        if (buttons & i) {
			if (buttonsCount > 2)
				return false;
            QString label = QGuiApplicationPrivate::platformTheme()->standardButtonText(i);
            label.remove(QChar('&'));
            nativeButtonText[buttonsCount] = strdup(label.toUtf8());
            buttonsIds[buttonsCount] = i;
            buttonsCount++;
        }
    }
	
	BAlert* alert = new BAlert(title.toUtf8(), doc.toPlainText().toUtf8(),
		nativeButtonText[0], nativeButtonText[1], nativeButtonText[2], B_WIDTH_AS_USUAL, type);
	
	int buttonId = buttonsIds[alert->Go()];

    QPlatformDialogHelper::StandardButton standardButton = static_cast<QPlatformDialogHelper::StandardButton>(buttonId);
    QPlatformDialogHelper::ButtonRole role = QPlatformDialogHelper::buttonRole(standardButton);
    emit clicked(standardButton, role);

	m_shown = true;
    return true;
}

void QHaikuPlatformMessageDialogHelper::hide()
{
    m_shown = false;
}

}
