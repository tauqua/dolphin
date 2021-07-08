// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "Common/CommonTypes.h"
#include "Common/MathUtil.h"
#include "Common/Timer.h"

#include "VideoCommon/GraphicsTrigger.h"
#include "VideoCommon/PEShaderSystem/Config/PEShaderConfigGroup.h"
#include "VideoCommon/PEShaderSystem/Config/PETriggerConfig.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShaderGroup.h"

class AbstractFramebuffer;
class AbstractTexture;
class GraphicsTriggerManager;

namespace VideoCommon::PE
{
struct TriggerParameters
{
  AbstractFramebuffer* m_dest_fb;
  MathUtil::Rectangle<int> m_dest_rect;
  const AbstractTexture* m_source_color_tex;
  const AbstractTexture* m_source_depth_tex;
  MathUtil::Rectangle<int> m_source_rect;
  int m_source_layer;
};

class TriggerPointManager
{
public:
  TriggerPointManager();
  void UpdateConfig(const TriggerConfig& config, const GraphicsTriggerManager& manager);
  void OnEFB(const TriggerParameters& trigger_parameters);
  void OnTextureLoad(const TriggerParameters& trigger_parameters);
  void OnDraw(const TriggerParameters& trigger_parameters);
  void OnPost(const TriggerParameters& trigger_parameters);
  void SetDepthNearFar(float depth_near, float depth_far);
  void Start();
  void Stop();

private:
  ShaderApplyOptions OptionsFromParameters(const TriggerParameters& trigger_parameters);
  Common::Timer m_timer;
  float m_depth_near = 0.0f;
  float m_depth_far = 0.0f;
  u64 m_trigger_changes = 0;
  std::map<std::string, ShaderGroup> m_trigger_name_to_group;
  std::vector<std::pair<EFBGraphicsTrigger, ShaderGroup*>> m_efb_shadergroups;
  std::vector<std::pair<TextureLoadGraphicsTrigger, ShaderGroup*>> m_texture_shadergroups;
  std::vector<std::pair<DrawCall2DGraphicsTrigger, ShaderGroup*>> m_draw_call_2d_shadergroups;
  std::vector<std::pair<DrawCall3DGraphicsTrigger, ShaderGroup*>> m_draw_call_3d_shadergroups;
  ShaderConfigGroup m_default_shader_group_config;
  ShaderGroup* m_post_group = nullptr;
};
}  // namespace VideoCommon::PE
