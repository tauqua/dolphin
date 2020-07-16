// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QString>
#include <QWidget>

#include <vector>

class QComboBox;
class QDir;
class QLineEdit;
class QListWidget;
class QVBoxLayout;

class PostProcessingAddShaderListWidget final : public QWidget
{
  Q_OBJECT
public:
  PostProcessingAddShaderListWidget(QWidget* parent, const QDir& root_path, bool allows_drag_drop);

  std::vector<QString> ChosenShaderPathes() const { return m_chosen_shader_pathes; }

private:
  void CreateWidgets();
  void ConnectWidgets();

  void OnShadersSelected();

  void OnShaderTypeChanged();
  void OnSearchTextChanged(QString search_text);
  static std::vector<std::string> GetAvailableShaders(const std::string& directory_path);

  QVBoxLayout* m_main_layout;
  QListWidget* m_shader_list;
  QComboBox* m_shader_type;
  QLineEdit* m_search_text;

  const QDir& m_root_path;
  bool m_allows_drag_drop;
  std::vector<QString> m_chosen_shader_pathes;
};
