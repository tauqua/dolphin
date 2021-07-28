// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <QWidget>

#include "VideoCommon/PEShaderSystem/Config/PEShaderConfig.h"
#include "VideoCommon/PEShaderSystem/Config/PEShaderConfigGroup.h"

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
  PostProcessingShaderListWidget(QWidget* parent, VideoCommon::PE::ShaderConfigGroup* group);

private:
  void CreateWidgets();
  void ConnectWidgets();

  void RefreshShaderList();
  void ShaderSelectionChanged();
  void ShaderItemChanged(QListWidgetItem* item);

  void OnShaderChanged(VideoCommon::PE::ShaderConfig* shader);
  QLayout* BuildPassesLayout(VideoCommon::PE::ShaderConfig* shader);
  QLayout* BuildSnapshotsLayout(VideoCommon::PE::ShaderConfig* shader);
  QLayout* BuildOptionsLayout(VideoCommon::PE::ShaderConfig* shader);

  void SaveShaderList();

  void OnShaderAdded();
  void OnShaderRemoved();

  void ClearLayoutRecursively(QLayout* layout);
  
  VideoCommon::PE::ShaderConfigGroup* m_group;

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
