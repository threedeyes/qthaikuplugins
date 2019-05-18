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

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>
#include <QtWidgets/qmessagebox.h>
#include <QTextDocument>
#include <QHash>

#include <qdebug.h>

#include <Alert.h>
#include <TextView.h>

#define DISABLE_MSG_NATIVE 1

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

bool QHaikuPlatformMessageDialogHelper::show(Qt::WindowFlags, Qt::WindowModality, QWindow *)
{
    QSharedPointer<QMessageDialogOptions> options = this->options();
    if (!options.data() || DISABLE_MSG_NATIVE)
        return false;

    const QString informativeText = options->informativeText();
    const QString title = options->windowTitle();
    const QString text = informativeText.isEmpty() ? options->text() : (options->text() + QLatin1Char('\n') + informativeText);
    
    alert_type type = B_EMPTY_ALERT;
    switch (options->icon()) {
    	case QMessageBox::NoIcon:
    	case QMessageBox::Information:
		case QMessageBox::Question:
    		type = B_INFO_ALERT;
    		break;
		case QMessageBox::Warning:
			type = B_WARNING_ALERT;
			break;
		case QMessageBox::Critical:
			type = B_STOP_ALERT;
			break;
    }

	QTextDocument doc;
	doc.setHtml(text);

	BAlert* alert = new BAlert();
	alert->SetText(doc.toPlainText().toUtf8().constData());
	alert->SetType(type);
	alert->SetButtonSpacing(B_EVEN_SPACING);
	alert->SetButtonWidth(B_WIDTH_AS_USUAL);

	int32 buttonsCount = 0;

	QHash<int32, uint32> buttonsHash;

	uint32 defButtonsPrio[]={
			QPlatformDialogHelper::Ok,
			QPlatformDialogHelper::Apply,
			QPlatformDialogHelper::Save,
			QPlatformDialogHelper::Open,
			QPlatformDialogHelper::Cancel,
			QPlatformDialogHelper::NoButton
		};
	uint32 escapeButtonsPrio[]={
			QPlatformDialogHelper::Cancel,
			QPlatformDialogHelper::Abort,
			QPlatformDialogHelper::Ignore,
			QPlatformDialogHelper::NoButton
		};

	int buttons = options->standardButtons() == QPlatformDialogHelper::NoButton ?
		QPlatformDialogHelper::Ok : options->standardButtons();

	uint32 defButtonId = QPlatformDialogHelper::NoButton;
	for (int i = 0; defButtonsPrio[i] != QPlatformDialogHelper::NoButton; i++) {
		if (buttons & defButtonsPrio[i] && defButtonId == QPlatformDialogHelper::NoButton)
			defButtonId = defButtonsPrio[i];
	}

    for (uint32 i = QPlatformDialogHelper::FirstButton; i < QPlatformDialogHelper::LastButton; i<<=1) {
        if ((buttons & i) && i != defButtonId) {
            QString label = QGuiApplicationPrivate::platformTheme()->standardButtonText(i);
            label = QPlatformTheme::removeMnemonics(label).trimmed();
            alert->AddButton(label.toUtf8().constData(), buttonsCount);
            buttonsHash[buttonsCount] = i;
            buttonsCount++;
        }
    }

	if (defButtonId != QPlatformDialogHelper::NoButton) {
		QString label = QGuiApplicationPrivate::platformTheme()->standardButtonText(defButtonId);
		label = QPlatformTheme::removeMnemonics(label).trimmed();
		alert->AddButton(label.toUtf8().constData(), buttonsCount);
		buttonsHash[buttonsCount] = defButtonId;
		buttonsCount++;
	}

	uint32 escapeButtonId = QPlatformDialogHelper::NoButton;
	for (int i = 0; escapeButtonsPrio[i] != QPlatformDialogHelper::NoButton; i++) {
		if (buttons & escapeButtonsPrio[i] && escapeButtonId == QPlatformDialogHelper::NoButton)
			escapeButtonId = escapeButtonsPrio[i];
	}
	
	alert->SetShortcut(buttonsHash.key(escapeButtonId, 0), B_ESCAPE);

    int32 alertButton = alert->Go();
	uint32 buttonId = buttonsHash[alertButton];

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
