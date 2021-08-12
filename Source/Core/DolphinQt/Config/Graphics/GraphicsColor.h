// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QColor>
#include <QPushButton>

class GraphicsColor : public QPushButton
{
  Q_OBJECT
public:
  GraphicsColor(QWidget* parent, bool supports_alpha);

  void SetColor(const QColor& color);
  const QColor& GetColor();

signals:
  void ColorIsUpdated();
public slots:
  void UpdateColor();
  void ChangeColor();

private:
  QColor m_color;
  bool m_supports_alpha = false;
};
