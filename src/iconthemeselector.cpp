/*-
 * Copyright (c) 2021 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QIcon>
#include <QTranslator>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDirIterator>

#include "iconthemeselector.h"

IconThemeSelector::IconThemeSelector(QWidget *parent) :
    QDialog(parent)
{
	QStringList names;
	QStringList paths	  = QIcon::themeSearchPaths();
	themeList		  = new QListWidget(this);
	QVBoxLayout *vbox	  = new QVBoxLayout;
	QHBoxLayout *hbox	  = new QHBoxLayout;
	QPushButton *okButton	  = new QPushButton(tr("&Ok"));
	QPushButton *cancelButton = new QPushButton(tr("&Cancel"));

	setWindowTitle(tr("Select Icon Theme"));
	for (int i = 0; i < paths.size(); i++) {
		QDirIterator it(paths.at(i));
		while (it.hasNext()) {
			QString indexPath = QString("%1/index.theme").arg(it.next());
			if (!it.fileInfo().isDir())
				continue;
			QString name = it.fileName();
			if (name == "." || name == "..")
				continue;
			QFile indexFile(indexPath);
			if (!indexFile.exists())
				continue;
			indexFile.close();
			names.append(name);
		}
	}
	names.sort(Qt::CaseInsensitive);
	names.removeDuplicates();
	foreach (const QString &name, names) {
		QListWidgetItem *item = new QListWidgetItem(name);
		item->setData(Qt::UserRole, QVariant(name));
		themeList->addItem(item);
	}
	vbox->setContentsMargins(15, 15, 15, 15);
	hbox->addWidget(okButton, 1, Qt::AlignRight);
	hbox->addWidget(cancelButton, 0, Qt::AlignLeft);
	vbox->addWidget(themeList);
	vbox->addLayout(hbox);
	setLayout(vbox);
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

QString
IconThemeSelector::getTheme()
{
	QString		theme("");
	QListWidgetItem *item;
	
	if (this->exec() == QDialog::Accepted) {
		if ((item = themeList->currentItem()) != 0)
			theme = item->data(Qt::UserRole).toString();
	}
	return (theme);
}
