// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/PEShaderSystem/Config/PEShaderConfigPass.h"

#include "Common/Logging/Log.h"

namespace VideoCommon::PE
{
bool ShaderConfigPass::DeserializeFromConfig(const picojson::object& obj, std::size_t total_passes)
{
  const auto entry_point_iter = obj.find("entry_point");
  if (entry_point_iter == obj.end())
  {
    ERROR_LOG_FMT(VIDEO, "Failed to load shader configuration file, 'entry_point' in pass not found");
    return false;
  }
  m_entry_point = entry_point_iter->second.to_str();

  const auto inputs_iter = obj.find("inputs");
  if (inputs_iter == obj.end())
  {
    ERROR_LOG_FMT(VIDEO,
                  "Failed to load shader configuration file, 'inputs' in pass not found");
    return false;
  }
  if (inputs_iter->second.is<picojson::array>())
  {
    const auto& inputs_arr = inputs_iter->second.get<picojson::array>();
    for (std::size_t i = 0; i < inputs_arr.size(); i++)
    {
      const auto& input_val = inputs_arr[i];
      if (!input_val.is<picojson::object>())
      {
        ERROR_LOG_FMT(
            VIDEO, "Failed to load shader configuration file, specified input is not a json object");
      }

      if (const auto input =
              DeserializeInputFromConfig(input_val.get<picojson::object>(), i, total_passes))
      {
        m_inputs.push_back(*input);
      }
      else
      {
        return false;
      }
    }
  }

  return true;
}

ShaderConfigPass ShaderConfigPass::CreateDefaultPass()
{
  PreviousPass previous_pass;
  previous_pass.m_sampler_state = RenderState::GetLinearSamplerState();

  ShaderConfigPass result;
  result.m_inputs.push_back(previous_pass);
  result.m_entry_point = "main";
  result.m_output_scale = 1.0f;

  return result;
}
}
