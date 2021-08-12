// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <vector>

#include <QDialog>
#include <QString>

class PostProcessingAddShaderListWidget;
class QDialogButtonBox;
class QTabWidget;
class QVBoxLayout;

class PostProcessingAddShaderDialog final : public QDialog
{
  Q_OBJECT
public:
  explicit PostProcessingAddShaderDialog(QWidget* parent);

  std::vector<QString> ChosenShaderPathes() const;

private:
  void CreateWidgets();

  QDialogButtonBox* m_buttonbox;
  QTabWidget* m_shader_tabs;
  QVBoxLayout* m_main_layout;
  PostProcessingAddShaderListWidget* m_system_shader_list;
  PostProcessingAddShaderListWidget* m_user_shader_list;
};
