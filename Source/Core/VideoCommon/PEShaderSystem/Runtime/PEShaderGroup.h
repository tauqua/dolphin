// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <optional>
#include <vector>

#include "Common/CommonTypes.h"
#include "VideoCommon/PEShaderSystem/Config/PEShaderConfigGroup.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShader.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShaderApplyOptions.h"
#include "VideoCommon/TextureConfig.h"

namespace VideoCommon::PE
{
class ShaderGroup
{
public:
  void UpdateConfig(const ShaderConfigGroup& group, bool force_recompile);
  void Apply(const ShaderApplyOptions& options);

private:
  bool CreateShaders(const ShaderConfigGroup& group);
  std::optional<u32> m_last_change_count = 0;
  std::vector<Shader> m_shaders;

  u32 m_target_width;
  u32 m_target_height;
  u32 m_target_layers;
  AbstractTextureFormat m_target_format;
};
}  // namespace VideoCommon::PE
