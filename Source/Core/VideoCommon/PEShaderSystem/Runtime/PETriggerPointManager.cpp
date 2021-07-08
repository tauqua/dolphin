#include "VideoCommon/PEShaderSystem/Runtime/PETriggerPointManager.h"

#include "Common/VariantUtil.h"
#include "VideoCommon/GraphicsTriggerManager.h"

namespace VideoCommon::PE
{
TriggerPointManager::TriggerPointManager()
    : m_default_shader_group_config(ShaderConfigGroup::CreateDefaultGroup())
{
}

void TriggerPointManager::UpdateConfig(const TriggerConfig& config,
                                       const GraphicsTriggerManager& manager)
{
  const bool reset = config.m_changes != m_trigger_changes;
  if (reset || m_post_group == nullptr)
  {
    m_trigger_name_to_group.clear();
    m_efb_shadergroups.clear();
    m_texture_shadergroups.clear();
    m_draw_call_2d_shadergroups.clear();
    m_draw_call_3d_shadergroups.clear();

    const bool first_run = m_post_group == nullptr;
    m_post_group = &m_trigger_name_to_group[Constants::default_trigger];
    if (first_run)
    {
      m_default_shader_group_config.m_changes = 0;
      m_post_group->UpdateConfig(m_default_shader_group_config, true);
    }
    m_trigger_changes = config.m_changes;
  }

  for (const auto& [trigger_name, shader_group_config] : config.m_trigger_name_to_shader_groups)
  {
    auto& shader_group = m_trigger_name_to_group[trigger_name];
    const bool all_disabled =
        std::none_of(shader_group_config.m_shaders.begin(), shader_group_config.m_shaders.end(),
                     [](const ShaderConfig& config) { return config.m_enabled; });
    if (shader_group_config.m_shaders.empty() || all_disabled)
    {
      m_default_shader_group_config.m_changes = shader_group_config.m_changes;
      shader_group.UpdateConfig(m_default_shader_group_config, false);
    }
    else
    {
      shader_group.UpdateConfig(shader_group_config, false);
    }
    if (reset)
    {
      const GraphicsTrigger* trigger = manager.GetTrigger(trigger_name);
      if (!trigger)
        continue;

      std::visit(overloaded{[&](const EFBGraphicsTrigger& t) {
                              m_efb_shadergroups.push_back({t, &shader_group});
                            },
                            [&](const TextureLoadGraphicsTrigger& t) {
                              m_texture_shadergroups.push_back({t, &shader_group});
                            },
                            [&](const DrawCall2DGraphicsTrigger& t) {
                              m_draw_call_2d_shadergroups.push_back({t, &shader_group});
                            },
                            [&](const DrawCall3DGraphicsTrigger& t) {
                              m_draw_call_3d_shadergroups.push_back({t, &shader_group});
                            },
                            [&](const PostGraphicsTrigger& t) { m_post_group = &shader_group; }},
                 *trigger);
    }
  }
}

void TriggerPointManager::OnEFB(const TriggerParameters&)
{
}

void TriggerPointManager::OnTextureLoad(const TriggerParameters&)
{
}

void TriggerPointManager::OnDraw(const TriggerParameters&)
{
}

void TriggerPointManager::OnPost(const TriggerParameters& trigger_parameters)
{
  const auto options = OptionsFromParameters(trigger_parameters);
  m_post_group->Apply(options);
}

void TriggerPointManager::SetDepthNearFar(float depth_near, float depth_far)
{
  m_depth_near = depth_near;
  m_depth_far = depth_far;
}

void TriggerPointManager::Start()
{
  m_timer.Start();
}

void TriggerPointManager::Stop()
{
  m_timer.Stop();
}

ShaderApplyOptions
TriggerPointManager::OptionsFromParameters(const TriggerParameters& trigger_parameters)
{
  ShaderApplyOptions options;
  options.m_dest_fb = trigger_parameters.m_dest_fb;
  options.m_dest_rect = trigger_parameters.m_dest_rect;
  options.m_source_color_tex = trigger_parameters.m_source_color_tex;
  options.m_source_depth_tex = trigger_parameters.m_source_depth_tex;

  options.m_source_rect = trigger_parameters.m_source_rect;
  options.m_source_layer = trigger_parameters.m_source_layer;
  options.m_time_elapsed = m_timer.GetTimeElapsed();
  options.m_depth_near = m_depth_near;
  options.m_depth_far = m_depth_far;
  return options;
}
}  // namespace VideoCommon::PE
