// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QPushButton>

class GraphicsImage : public QPushButton
{
    Q_OBJECT
public:
    GraphicsImage( QWidget* parent );

    const std::string& GetPath() const { return m_path; }

signals:
    void PathIsUpdated();
public slots:
    void UpdateImage();
    void SelectImage();

private:
    std::string m_path;
};
