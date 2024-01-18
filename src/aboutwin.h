/*-
 * Copyright (c) 2024 Marcel Kaiser. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <QDialog>
#include <QLabel>
#include <QWidget>

#include "iconloader.h"

class AboutWin : public QDialog {
  Q_OBJECT
 public:
  AboutWin(IconLoader &iconLoader, QWidget *parent = nullptr);

 private:
  void addPointSize(QLabel *text, int increase);
  QWidget *createAboutTab();
  QWidget *createContribTab();

  IconLoader *iconLoader = nullptr;
};
