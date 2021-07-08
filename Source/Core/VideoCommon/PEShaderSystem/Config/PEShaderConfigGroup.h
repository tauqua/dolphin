// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <vector>

#include <picojson.h>

#include "Common/CommonTypes.h"
#include "VideoCommon/PEShaderSystem/Config/PEShaderConfig.h"

namespace VideoCommon::PE
{
struct ShaderConfigGroup
{
  std::vector<ShaderConfig> m_shaders;
  std::vector<u32> m_shader_order;
  u32 m_changes = 0;

  bool AddShader(const std::string& path);
  void RemoveShader(u32 index);

  void SerializeToProfile(picojson::object& obj) const;
  void DeserializeFromProfile(picojson::object& obj);

  static ShaderConfigGroup CreateDefaultGroup();
};
}  // namespace VideoCommon::PE
