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

#ifndef QHAIKUPLATFORMDIALOGHELPERS_H
#define QHAIKUPLATFORMDIALOGHELPERS_H

#include <qpa/qplatformdialoghelper.h>
#include <QEventLoop>

#include <FilePanel.h>
#include <Directory.h>
#include <Entry.h>
#include <Node.h>
#include <Path.h>
#include <TextControl.h>
#include <Looper.h>
#include <Menu.h>

namespace QtHaikuDialogHelpers {

class QHaikuFileDialogHelper;
class QHaikuFileDialogHelperPrivate;

class QHaikuPlatformMessageDialogHelper: public QPlatformMessageDialogHelper
{
    Q_OBJECT
public:
    QHaikuPlatformMessageDialogHelper();
    void exec();
    bool show(Qt::WindowFlags windowFlags,
                          Qt::WindowModality windowModality,
                          QWindow *parent);
    void hide();
    
private:
    int m_buttonId;
    bool m_shown;
};


class DirectoryRefFilter : public BRefFilter {
public:
	virtual ~DirectoryRefFilter() {	};
	virtual bool Filter(const entry_ref* ref, BNode* node,
		struct stat_beos* stat, const char* filetype)
	{
		Q_UNUSED(ref)
		Q_UNUSED(stat)
		Q_UNUSED(filetype)
		return node->IsDirectory();
	}
};


class FileRefFilter : public BRefFilter {
public:
	virtual ~FileRefFilter() { };
	virtual bool Filter(const entry_ref* ref, BNode* node,
		struct stat_beos* stat, const char* filetype);
	void setFilters(const QString &filters);
private:
	QStringList m_exts;
	bool m_showAll;
};


class FilePanelLooper : public QObject, public BLooper {
	Q_OBJECT
public:
	FilePanelLooper(QHaikuFileDialogHelper *helper);
	virtual	~FilePanelLooper() { };
	virtual	void MessageReceived(BMessage* message);

Q_SIGNALS:
    void accept();
    void reject();
    void fileSelected(const QUrl &file);
    void filesSelected(const QList<QUrl> &files);
    void selectNameFilter(const QString &filter);

private:
	QHaikuFileDialogHelper *m_helper;

	uint32 m_state;
	QUrl m_filename;
	QList<QUrl> m_filenames;
	entry_ref m_ref;
};


class QHaikuFileDialogHelperPrivate
{
public:
    bool shown;
    QEventLoop m_eventloop;
    FilePanelLooper *m_panelLooper;
    BFilePanel *m_filePanel;
    BMenu *m_typeMenu;
    FileRefFilter *m_fileRefFilter;
    QString m_filter;
    QString m_saveFileName;
};


class QHaikuFileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT
public:
    explicit QHaikuFileDialogHelper();
    ~QHaikuFileDialogHelper() { };

    void exec() override;
    bool show(Qt::WindowFlags, Qt::WindowModality, QWindow *) override;
    void hide() override;

    QString selectedNameFilter() const override;
    void selectNameFilter(const QString &filter) override;
    void setFilter() override {};
    QList<QUrl> selectedFiles() const override { return m_selectedFile; };
    void selectFile(const QUrl &saveFileName) override;
    void setDirectory(const QUrl &directory) override;
    QUrl directory() const override;
    bool defaultNameFilterDisables() const override { return false; };

    void setSelectedFiles(QList<QUrl> &list) { m_selectedFile = list; };

private:
	void createFilePanel(file_panel_mode mode = B_OPEN_PANEL,
		uint32 flavors = B_FILE_NODE | B_DIRECTORY_NODE | B_SYMLINK_NODE);

    QScopedPointer<QHaikuFileDialogHelperPrivate> d_ptr;

    QList<QUrl> m_selectedFile;

    Q_DECLARE_PRIVATE(QHaikuFileDialogHelper)
};

}

#endif // QHAIKUPLATFORMDIALOGHELPERS_H
