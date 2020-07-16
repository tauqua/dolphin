// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "DolphinQt/Config/Graphics/GraphicsWidget.h"

class CustomShaderWindow;
class GraphicsBool;
class GraphicsChoice;
class GraphicsSlider;
class GraphicsWindow;
class QCheckBox;
class QComboBox;
class QPushButton;
class QSlider;
class ToolTipComboBox;

class EnhancementsWidget final : public GraphicsWidget
{
  Q_OBJECT
public:
  explicit EnhancementsWidget(GraphicsWindow* parent);

private:
  void LoadSettings() override;
  void SaveSettings() override;

  void CreateWidgets();
  void ConnectWidgets();
  void AddDescriptions();

  // Enhancements
  GraphicsChoice* m_ir_combo;
  ToolTipComboBox* m_aa_combo;
  GraphicsChoice* m_af_combo;
  QPushButton* m_configure_custom_shaders;
  GraphicsBool* m_scaled_efb_copy;
  GraphicsBool* m_per_pixel_lighting;
  GraphicsBool* m_force_texture_filtering;
  GraphicsBool* m_widescreen_hack;
  GraphicsBool* m_disable_fog;
  GraphicsBool* m_force_24bit_color;
  GraphicsBool* m_disable_copy_filter;
  GraphicsBool* m_arbitrary_mipmap_detection;
  CustomShaderWindow* m_custom_shader_window;

  // Stereoscopy
  GraphicsChoice* m_3d_mode;
  GraphicsSlider* m_3d_depth;
  GraphicsSlider* m_3d_convergence;
  GraphicsBool* m_3d_swap_eyes;

  int m_msaa_modes;
  bool m_block_save;
};
