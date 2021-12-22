/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Copyright (C) 2015-2021 Gerasim Troeglazov,
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
#include <QFileDialog>
#include <QHash>
#include <QList>
#include <QUrl>

#include <qdebug.h>

#include <Alert.h>
#include <TextView.h>
#include <MenuBar.h>
#include <MenuItem.h>

#define DISABLE_MSG_NATIVE 1

Q_DECLARE_METATYPE(QList<QUrl>)

namespace QtHaikuDialogHelpers {

enum {
	kMsgFilter = 'filt',
	kMsgFilterDisable = 'foff'
};

QHaikuPlatformMessageDialogHelper::QHaikuPlatformMessageDialogHelper()
    :m_buttonId(-1)
    ,m_shown(false)
{
	qRegisterMetaType<QList<QUrl> >();
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


bool FileRefFilter::Filter(const entry_ref* ref, BNode* node,
		struct stat_beos* stat, const char* filetype)
{
	Q_UNUSED(stat)
	Q_UNUSED(filetype)

	if (node->IsDirectory() || m_showAll)
		return true;

	BPath path(ref);
	QFileInfo info(path.Path());
	return m_exts.contains("." + info.completeSuffix());
}

void FileRefFilter::setFilters(const QString &filters)
{
	m_exts.clear();
	m_showAll = false;

	for (const QString &filter : QPlatformFileDialogHelper::cleanFilterList(filters)) {
		const int offset = (filter.length() > 1 && filter.startsWith(QLatin1Char('*'))) ? 1 : 0;
		QString suffix = QLatin1String(filter.toUtf8().data() + offset);
		m_exts << suffix;
		if (suffix.contains("*"))
			m_showAll = true;
	}
}


FilePanelLooper::FilePanelLooper(QHaikuFileDialogHelper *helper)
	:QObject()
	,BLooper("PanelLooper")
	,m_helper(helper)
	,m_state(B_CANCEL)
{
	qRegisterMetaType<QList<QUrl> >();
}

void FilePanelLooper::MessageReceived(BMessage* message)
{
	message->PrintToStream();

	switch (message->what) {
		case kMsgFilter:
			{
				const char *filter = message->FindString("filter");
				emit selectNameFilter(QString::fromUtf8(filter));
				break;
			}
		case kMsgFilterDisable:
			{
				emit selectNameFilter(QString());
				break;
			}
		case B_SAVE_REQUESTED:
			{
				entry_ref ref;
				const char *name;
				message->FindRef("directory", &ref);
				BDirectory  dir(&ref);
				BPath path(&dir, NULL, false);
				message->FindString("name", &name);
				path.Append(name);

				m_filename.clear();
				m_filenames.clear();

				m_filename = QUrl::fromLocalFile(QString::fromUtf8(path.Path()));
				m_filenames.append(m_filename);

				m_helper->setSelectedFiles(m_filenames);

				emit fileSelected(m_filename);
				emit accept();

				m_state = B_OK;
			}
			break;
		case B_REFS_RECEIVED:
			{
				uint32 type;
				int32 count;

				m_filename.clear();
				m_filenames.clear();

				message->GetInfo("refs", &type, &count);
				if ( type != B_REF_TYPE || count <= 0)
					return;

				for ( long i = --count; i >= 0; i-- ) {
					if ( message->FindRef("refs", i, &m_ref) == B_OK ) {
						BPath path(&m_ref);
						m_filename = QUrl::fromLocalFile(QString::fromUtf8(path.Path()));
						m_filenames.append(m_filename);
					}
				}
				m_helper->setSelectedFiles(m_filenames);

				emit filesSelected(m_filenames);
				emit accept();

				m_state = B_OK;
			}
			break;
		case B_CANCEL:
			{
				if ( m_state != B_OK ) {
					m_state = B_CANCEL;
					m_filename.clear();
					m_filenames.clear();

					m_helper->setSelectedFiles(m_filenames);

					emit reject();
				}
			}
			break;
		default:
			break;
	}
}

QHaikuFileDialogHelper::QHaikuFileDialogHelper()
    : QPlatformFileDialogHelper(), d_ptr(new QHaikuFileDialogHelperPrivate)
{
    Q_D(QHaikuFileDialogHelper);

    d->shown = false;
    d->m_panelLooper = NULL;
    d->m_fileRefFilter = NULL;
    d->m_typeMenu = NULL;
}

void QHaikuFileDialogHelper::createFilePanel(file_panel_mode mode, uint32 flavors)
{
	Q_D(QHaikuFileDialogHelper);

	const QSharedPointer<QFileDialogOptions> dialogOptions = options();

	bool directoryMode = flavors == B_DIRECTORY_NODE;

	d->m_panelLooper = new FilePanelLooper(this);

    connect(d->m_panelLooper, &FilePanelLooper::accept, this, &QPlatformDialogHelper::accept);
	connect(d->m_panelLooper, &FilePanelLooper::reject, this, &QPlatformDialogHelper::reject);
	connect(d->m_panelLooper, &FilePanelLooper::fileSelected, this, &QPlatformFileDialogHelper::fileSelected);
	connect(d->m_panelLooper, &FilePanelLooper::filesSelected, this, &QPlatformFileDialogHelper::filesSelected);
	connect(d->m_panelLooper, &FilePanelLooper::selectNameFilter, this, &QPlatformFileDialogHelper::selectNameFilter);

	if (!directoryMode)
		d->m_fileRefFilter = new FileRefFilter();

	d->m_filePanel = new BFilePanel(mode, NULL, NULL, flavors,
						dialogOptions->fileMode() == QFileDialogOptions::ExistingFiles, NULL,
						directoryMode ? new DirectoryRefFilter() : NULL, true, false);
	d->m_filePanel->SetTarget(BMessenger(d->m_panelLooper));
	d->m_filePanel->SetPanelDirectory(options()->initialDirectory().path().toUtf8().data());

	if (!options()->windowTitle().isEmpty())
		d->m_filePanel->Window()->SetTitle(options()->windowTitle().toUtf8().data());

	bool allFilesOnly = false;
	if (options()->nameFilters().count() == 1) {
		QStringList filters = QPlatformFileDialogHelper::cleanFilterList(options()->nameFilters().at(0));
		if (filters.contains("*.*") || filters.contains("*"))
			allFilesOnly = true;
	}

	if (!directoryMode && options()->nameFilters().count() > 0 && !allFilesOnly) {
		BMenuBar *menuBar = dynamic_cast<BMenuBar*>(d->m_filePanel->Window()->ChildAt(0)->FindView("MenuBar"));
		d->m_typeMenu = new BMenu(QFileDialog::tr("Files of type:").remove(':').toUtf8().data());
		d->m_typeMenu->SetRadioMode(true);
		menuBar->AddItem(d->m_typeMenu);

		BMenuItem* item = new BMenuItem(QFileDialog::tr("All files (*)").remove(" (*)").toUtf8().data(),
							new BMessage(kMsgFilterDisable));
		item->SetTarget(BMessenger(d->m_panelLooper));
		d->m_typeMenu->AddItem(item);
		d->m_typeMenu->AddSeparatorItem();

		const QStringList nameFilters = options()->nameFilters();
		for (const QString &namedFilter : nameFilters) {
			const int offset = namedFilter.indexOf(QLatin1String("("));
			const QString filterTitle = namedFilter.mid(0, offset);
			BMessage* mess = new BMessage(kMsgFilter);
			mess->AddString("filter", namedFilter.toUtf8().data());
			BMenuItem* item = new BMenuItem(filterTitle.toUtf8().data(), mess);
			item->SetTarget(BMessenger(d->m_panelLooper));
			d->m_typeMenu->AddItem(item);
		}

		selectNameFilter(options()->initiallySelectedNameFilter());
	}

	d->m_panelLooper->Run();
}

void QHaikuFileDialogHelper::exec()
{
    Q_D(QHaikuFileDialogHelper);

    if (!d->shown)
        show(Qt::Dialog, Qt::ApplicationModal, 0);

    d->m_eventloop.exec(QEventLoop::DialogExec);
}

bool QHaikuFileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags)
    Q_UNUSED(windowModality)
    Q_UNUSED(parent)
    Q_D(QHaikuFileDialogHelper);

	const QSharedPointer<QFileDialogOptions> dialogOptions = options();
    switch (dialogOptions->acceptMode()) {
	    default:
	    case QFileDialogOptions::AcceptOpen:
	    {
	        switch (dialogOptions->fileMode())
	        {
		        case QFileDialogOptions::Directory:
		        case QFileDialogOptions::DirectoryOnly:
		        {
					createFilePanel(B_OPEN_PANEL, B_DIRECTORY_NODE);
					d->m_filePanel->Show();
					d->shown = true;
					break;
		        }
		        case QFileDialogOptions::AnyFile:
		        case QFileDialogOptions::ExistingFile:
		        case QFileDialogOptions::ExistingFiles:
		        {
					createFilePanel(B_OPEN_PANEL);
					d->m_filePanel->Show();
					d->shown = true;
					break;
		        }
	        }
	        break;
	    }
	    case QFileDialogOptions::AcceptSave:
	    {
			createFilePanel(B_SAVE_PANEL);
			d->m_filePanel->SetSaveText(d->m_saveFileName.toUtf8().data());

			BView* textControl = dynamic_cast<BView*>(d->m_filePanel->Window()->ChildAt(0)->FindView("text view"));
			BView* buttonControl = dynamic_cast<BView*>(d->m_filePanel->Window()->ChildAt(0)->FindView("cancel button"));

			if (textControl != NULL && buttonControl != NULL) {
				textControl->Window()->LockLooper();
				float shift = (buttonControl->Frame().left - textControl->Frame().right) - textControl->Frame().left;
				textControl->ResizeBy(shift, 0);
				textControl->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
				textControl->Window()->UnlockLooper();
			}

			d->m_filePanel->Show();
			d->m_filePanel->Refresh();

			d->shown = true;
			break;
		}
    }

    if (d->shown)
		selectNameFilter(d->m_filter);

    return d->shown;
}

void QHaikuFileDialogHelper::hide()
{
    Q_D(QHaikuFileDialogHelper);

	if (!d->shown)
		return;

	if (d->m_eventloop.isRunning())
		d->m_eventloop.exit();

	delete d->m_filePanel;

	if (d->m_fileRefFilter != NULL)
		delete d->m_fileRefFilter;

	d->m_panelLooper->Lock();
	d->m_panelLooper->Quit();

    d->shown = false;

	d->m_fileRefFilter = NULL;
	d->m_filePanel = NULL;
	d->m_typeMenu = NULL;
}

QUrl QHaikuFileDialogHelper::directory() const
{
    Q_D(const QHaikuFileDialogHelper);

	if (!d->shown)
		return QUrl();

	entry_ref panelDirRef;
	d->m_filePanel->GetPanelDirectory(&panelDirRef);
	BPath path(&panelDirRef);
	return QUrl::fromLocalFile(QString::fromUtf8(path.Path()));
}

void QHaikuFileDialogHelper::setDirectory(const QUrl &directory)
{
	Q_D(QHaikuFileDialogHelper);

	if (!d->shown || !directory.isLocalFile())
		return;

	d->m_filePanel->SetPanelDirectory(directory.toLocalFile().toUtf8().data());
}

QString QHaikuFileDialogHelper::selectedNameFilter() const
{
	Q_D(const QHaikuFileDialogHelper);
	return d->m_filter;
}

void QHaikuFileDialogHelper::selectNameFilter(const QString &filter)
{
	Q_D(QHaikuFileDialogHelper);
	d->m_filter = filter;

	if (!d->shown)
		return;

	if (d->m_fileRefFilter != NULL) {
		if (filter.isEmpty()) {
			d->m_fileRefFilter->setFilters("All files (*.*)");
		} else {
			d->m_fileRefFilter->setFilters(d->m_filter);
		}
		d->m_filePanel->SetRefFilter(d->m_fileRefFilter);
	}

	if (d->m_typeMenu != NULL) {
		if (filter.isEmpty()) {
			d->m_typeMenu->ItemAt(0)->SetMarked(true);
		} else {
			for (int32 index = 0; index < d->m_typeMenu->CountItems(); index++) {
				BMessage *mess = d->m_typeMenu->ItemAt(index)->Message();
				if (mess != NULL && mess->what == kMsgFilter) {
					QString itemFilter = QString::fromUtf8(mess->FindString("filter"));
					if (itemFilter == filter)
						d->m_typeMenu->ItemAt(index)->SetMarked(true);
				}
			}
		}
	}

	d->m_filePanel->Refresh();
}

void QHaikuFileDialogHelper::selectFile(const QUrl &saveFileName)
{
    Q_D(QHaikuFileDialogHelper);

 	if (saveFileName.isLocalFile())
		d->m_saveFileName = QFileInfo(saveFileName.toLocalFile()).fileName();
	else
		d->m_saveFileName = QString();

	if (d->shown)
		d->m_filePanel->SetSaveText(d->m_saveFileName.toUtf8().data());
}

}
