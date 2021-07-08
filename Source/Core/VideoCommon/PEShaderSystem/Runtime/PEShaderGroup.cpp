// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/PEShaderSystem/Runtime/PEShaderGroup.h"

#include <algorithm>

#include "VideoCommon/AbstractFramebuffer.h"
#include "VideoCommon/AbstractPipeline.h"
#include "VideoCommon/AbstractShader.h"
#include "VideoCommon/AbstractTexture.h"

namespace VideoCommon::PE
{
void ShaderGroup::UpdateConfig(const ShaderConfigGroup& group, bool force_recompile)
{
  const bool needs_compile =  group.m_changes != m_last_change_count || force_recompile;
  m_last_change_count = group.m_changes;
  if (needs_compile)
  {
    if (!CreateShaders(group))
    {
      m_shaders.clear();
    }
  }
  else
  {
    bool failure = false;
    for (std::size_t i = 0; i < m_shaders.size(); i++)
    {
      const u32& shader_index = group.m_shader_order[i];
      const auto& config_shader = group.m_shaders[shader_index];
      if (!config_shader.m_enabled)
        continue;

      auto& shader = m_shaders[i];
      if (!shader.UpdateConfig(config_shader))
      {
        failure = true;
        break;
      }
    }

    if (failure)
    {
      m_shaders.clear();
    }
  }
}

void ShaderGroup::Apply(const ShaderApplyOptions& options)
{
  if (m_shaders.empty())
    return;

  const u32 dest_rect_width = static_cast<u32>(options.m_dest_rect.GetWidth());
  const u32 dest_rect_height = static_cast<u32>(options.m_dest_rect.GetHeight());
  if (m_target_width != dest_rect_width || m_target_height != dest_rect_height ||
      m_target_layers != options.m_dest_fb->GetLayers() ||
      m_target_format != options.m_dest_fb->GetColorFormat())
  {
    const bool rebuild_pipelines = m_target_format != options.m_dest_fb->GetColorFormat() ||
                                   m_target_layers != options.m_dest_fb->GetLayers();
    m_target_width = dest_rect_width;
    m_target_height = dest_rect_height;
    m_target_layers = options.m_dest_fb->GetLayers();
    m_target_format = options.m_dest_fb->GetColorFormat();
    bool failure = false;
    const AbstractTexture* last_texture = options.m_source_color_tex;
    for (auto& shader : m_shaders)
    {
      if (!shader.CreateAllPassOutputTextures(dest_rect_width, dest_rect_height,
                                              options.m_dest_fb->GetLayers(),
                                              options.m_dest_fb->GetColorFormat()))
      {
        failure = true;
        break;
      }

      shader.UpdatePassInputs(last_texture);

      if (rebuild_pipelines)
      {
        if (!shader.RebuildPipeline(m_target_format, m_target_layers))
        {
          failure = true;
          break;
        }
      }
      last_texture = shader.GetPasses().back().m_output_texture.get();
    }

    if (failure)
    {
      // Clear shaders so we won't try again until rebuilt
      m_shaders.clear();
      return;
    }
  }

  const AbstractTexture* last_texture = options.m_source_color_tex;
  float output_scale = 1.0f;
  for (std::size_t i = 0; i < m_shaders.size() - 1; i++)
  {
    auto& shader = m_shaders[i];
    const bool skip_final_copy = false;
    shader.Apply(skip_final_copy, options, last_texture, output_scale);

    const auto& last_pass = shader.GetPasses().back();
    last_texture = last_pass.m_output_texture.get();
    output_scale = last_pass.m_output_scale;
  }

  const auto& last_pass = m_shaders.back().GetPasses().back();
  const bool last_pass_scaled = last_pass.m_output_scale != 1.0;
  const bool last_pass_uses_color_buffer = std::any_of(
      last_pass.m_inputs.begin(), last_pass.m_inputs.end(),
      [](auto&& input) { return input->GetType() == InputType::ColorBuffer; });

  // Determine whether we can skip the final copy by writing directly to the output texture, if the
  // last pass is not scaled, and the target isn't multisampled.
  const bool skip_final_copy =
      !last_pass_scaled && !last_pass_uses_color_buffer &&
      options.m_dest_fb->GetColorAttachment() != options.m_source_color_tex &&
      options.m_dest_fb->GetSamples() == 1;

  const auto last_index = m_shaders.size() - 1;
  m_shaders[last_index].Apply(skip_final_copy, options, last_texture, output_scale);
}

bool ShaderGroup::CreateShaders(const ShaderConfigGroup& group)
{
  m_target_width = 0;
  m_target_height = 0;
  m_target_format = AbstractTextureFormat::Undefined;
  m_target_layers = 0;

  m_shaders.clear();
  for (const u32& shader_index : group.m_shader_order)
  {
    const auto& config_shader = group.m_shaders[shader_index];
    if (!config_shader.m_enabled)
      continue;
    Shader shader;
    shader.CreateOptions(config_shader);
    if (!shader.CreatePasses(config_shader))
    {
      return false;
    }
    m_shaders.push_back(std::move(shader));
  }

  return true;
}
}  // namespace VideoCommon::PE
