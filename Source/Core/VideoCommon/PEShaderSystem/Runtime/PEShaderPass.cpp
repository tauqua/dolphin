// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/PEShaderSystem/Runtime/PEShaderPass.h"

#include <fmt/format.h>

#include "VideoCommon/ShaderGenCommon.h"
#include "VideoCommon/VideoConfig.h"

namespace VideoCommon::PE
{
void ShaderPass::WriteShaderIndices(ShaderCode& shader_source) const
{
  // Figure out which input indices map to color/depth/previous buffers.
  // If any of these buffers is not bound, defaults of zero are fine here,
  // since that buffer may not even be used by the shader.
  u32 color_buffer_index = 0;
  u32 depth_buffer_index = 0;
  u32 prev_output_index = 0;
  for (auto&& input : m_inputs)
  {
    switch (input->GetType())
    {
    case InputType::ColorBuffer:
      color_buffer_index = input->GetTextureUnit();
      break;

    case InputType::DepthBuffer:
      depth_buffer_index = input->GetTextureUnit();
      break;

    case InputType::PreviousPassOutput:
      prev_output_index = input->GetTextureUnit();
      break;

    case InputType::PreviousShaderOutput:
      prev_output_index = input->GetTextureUnit();
      break;

    default:
      break;
    }
  }

  // Hook the discovered indices up to macros.
  // This is to support the SampleDepth, SamplePrev, etc. macros.
  shader_source.Write("#define COLOR_BUFFER_INPUT_INDEX {}\n", color_buffer_index);
  shader_source.Write("#define DEPTH_BUFFER_INPUT_INDEX {}\n", depth_buffer_index);
  shader_source.Write("#define PREV_OUTPUT_INPUT_INDEX {}\n", prev_output_index);
}
}
