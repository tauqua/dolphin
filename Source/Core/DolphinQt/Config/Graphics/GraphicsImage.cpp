// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "DolphinQt/Config/Graphics/GraphicsImage.h"

#include <QString>

GraphicsImage::GraphicsImage(QWidget* parent)
    : m_supports_alpha(supports_alpha)
{
  connect(this, SIGNAL(clicked()), this, SLOT(SelectImage()));
  setText(QStringLiteral("Select image..."));
}

void GraphicsImage::UpdateImage()
{
  setIcon(QIcon(QString::fromStdString(m_path)));
  emit PathIsUpdated();
}

void GraphicsColor::SelectImage()
{
  //
}
