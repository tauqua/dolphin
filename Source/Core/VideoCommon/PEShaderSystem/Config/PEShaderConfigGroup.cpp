// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/PEShaderSystem/Config/PEShaderConfigGroup.h"

namespace VideoCommon::PE
{
bool ShaderConfigGroup::AddShader(const std::string& path)
{
  if (auto shader = ShaderConfig::Load(path))
  {
    m_shaders.push_back(*shader);
    const u32 index = static_cast<u32>(m_shaders.size());
    m_shader_order.push_back(index - 1);
    m_changes += 1;
    return true;
  }

  return false;
}

void ShaderConfigGroup::RemoveShader(u32 index)
{
  auto it = std::find_if(m_shader_order.begin(), m_shader_order.end(),
                         [index](u32 order_index) { return index == order_index; });
  if (it == m_shader_order.end())
    return;
  m_shader_order.erase(it);

  for (std::size_t i = 0; i < m_shader_order.size(); i++)
  {
    if (m_shader_order[i] > index)
    {
      m_shader_order[i]--;
    }
  }

  m_shaders.erase(m_shaders.begin() + index);
  m_changes += 1;
}

void ShaderConfigGroup::SerializeToProfile(picojson::object& obj) const
{
}

void ShaderConfigGroup::DeserializeFromProfile(picojson::object& obj)
{
  m_changes += 1;
}

ShaderConfigGroup ShaderConfigGroup::CreateDefaultGroup()
{
  ShaderConfigGroup group;
  group.m_shaders.push_back(ShaderConfig::CreateDefaultShader());
  group.m_shader_order.push_back(0);
  group.m_changes += 1;
  return group;
}
}  // namespace VideoCommon::PE
