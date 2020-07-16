// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QWidget>

#include "VideoCommon/PostProcessing.h"

class QGroupBox;
class QHBoxLayout;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QVBoxLayout;

class PostProcessingShaderListWidget final : public QWidget
{
  Q_OBJECT
public:
  enum class ShaderType
  {
    Textures,
    EFB,
    XFB,
    Post
  };
  PostProcessingShaderListWidget(QWidget* parent, ShaderType type);

private:
  void CreateWidgets();
  void ConnectWidgets();

  void RefreshShaderList();
  void ShaderSelectionChanged();
  void ShaderItemChanged(QListWidgetItem* item);

  void OnShaderChanged(VideoCommon::PostProcessing::Shader* shader);
  QLayout* BuildPassesLayout(VideoCommon::PostProcessing::Shader* shader);
  QLayout* BuildSnapshotsLayout(VideoCommon::PostProcessing::Shader* shader);
  QLayout* BuildOptionsLayout(VideoCommon::PostProcessing::Shader* shader);

  void SaveShaderList();

  void OnShaderAdded();
  void OnShaderRemoved();

  void ClearLayoutRecursively(QLayout* layout);

  VideoCommon::PostProcessing::Config* GetConfig() const;
  
  ShaderType m_type;

  QListWidget* m_shader_list;

  QGroupBox* m_passes_box;
  QGroupBox* m_options_snapshots_box;
  QGroupBox* m_options_box;
  QLabel* m_selected_shader_name;
  QVBoxLayout* m_shader_meta_layout;
  QHBoxLayout* m_shader_buttons_layout;
  
  QPushButton* m_add_shader;
  QPushButton* m_remove_shader;
};
