// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <vector>

#include <picojson.h>

#include "VideoCommon/PEShaderSystem/Config/PEShaderConfigInput.h"

namespace VideoCommon::PE
{
struct ShaderConfigPass
{
  std::vector<ShaderConfigInput> m_inputs;
  std::string m_entry_point;
  std::string m_dependent_option;

  float m_output_scale = 1.0f;

  bool DeserializeFromConfig(const picojson::object& obj, std::size_t total_passes);

  static ShaderConfigPass CreateDefaultPass();
};
}  // namespace VideoCommon::PE
