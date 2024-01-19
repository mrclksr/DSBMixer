/*-
 * Copyright (c) 2024 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "aboutwin.h"
#include "defs.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include "contributors.h"
#include "qt-helper/qt-helper.h"

AboutWin::AboutWin(IconLoader &iconLoader, QWidget *parent)
    : QDialog(parent), iconLoader{&iconLoader} {
  QVBoxLayout *layout{new QVBoxLayout};
  QTabWidget *tabs{new QTabWidget{this}};
  tabs->addTab(createAboutTab(), QString(tr("About")));
  tabs->addTab(createContribTab(), tr("Contributors"));
  QIcon icon{qh::loadStockIcon(QStyle::SP_DialogCloseButton)};
  QPushButton *close{new QPushButton{icon, tr("&Close")}};
  connect(close, SIGNAL(clicked()), this, SLOT(reject()));
  layout->addWidget(tabs);
  layout->addStretch(1);
  layout->addWidget(close, 0, Qt::AlignRight);
  setLayout(layout);
  setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
  setWindowTitle(tr("About DSBMixer"));
  setWindowIcon(iconLoader.aboutIcon);
}

QWidget *AboutWin::createAboutTab() {
  QWidget *container{new QWidget{this}};
  QHBoxLayout *layout{new QHBoxLayout};
  QVBoxLayout *vbox{new QVBoxLayout};
  QLabel *logo{new QLabel};
  QLabel *title{new QLabel{"<b>DSBMixer</b>"}};
  QLabel *version{new QLabel{QString("<b>%1</b>").arg(PROGRAM_VERSION)}};
  QLabel *text{new QLabel{tr("DSBMixer is a tabbed audio mixer for FreeBSD.")}};
  QLabel *author{new QLabel{"© 2017-2024 Marcel Kaiser <mk@freeshell.de>"}};
  QLabel *website{new QLabel{QString("<a href=\"%1\">Website").arg(WEBSITE)}};
  website->setOpenExternalLinks(true);
  addPointSize(author, -2);
  logo->setPixmap(iconLoader->mixerIcon.pixmap(64));
  layout->addWidget(logo, 0, Qt::AlignLeft | Qt::AlignTop);
  vbox->addWidget(title);
  vbox->addWidget(version);
  vbox->addWidget(text);
  vbox->addWidget(website);
  vbox->addWidget(author);
  layout->addLayout(vbox);
  layout->addStretch(1);
  container->setLayout(layout);
  return (container);
}

QWidget *AboutWin::createContribTab() {
  QWidget *container{new QWidget{this}};
  QVBoxLayout *layout{new QVBoxLayout};
  QString listText;
  for (auto c : contributors)
    listText.append(QString("%1<br/>").arg(QString::fromStdString(c)));
  QLabel *list{new QLabel{listText}};
  list->setOpenExternalLinks(true);
  layout->addWidget(list);
  container->setLayout(layout);
  return (container);
}

void AboutWin::addPointSize(QLabel *text, int increase) {
  QFont font{text->font()};
  font.setPointSize(font.pointSize() - 2);
  text->setFont(font);
}
