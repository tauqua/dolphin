// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "VideoCommon/AbstractFramebuffer.h"
#include "VideoCommon/AbstractPipeline.h"
#include "VideoCommon/AbstractShader.h"
#include "VideoCommon/AbstractTexture.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShaderInput.h"

class AbstractFramebuffer;
class AbstractPipeline;
class AbstractShader;
class AbstractTexture;
class ShaderCode;

namespace VideoCommon::PE
{
struct ShaderPass
{
  std::shared_ptr<AbstractShader> m_vertex_shader;
  std::unique_ptr<AbstractShader> m_pixel_shader;
  std::unique_ptr<AbstractPipeline> m_pipeline;
  std::vector<std::unique_ptr<ShaderInput>> m_inputs;

  std::unique_ptr<AbstractTexture> m_output_texture;
  std::unique_ptr<AbstractFramebuffer> m_output_framebuffer;
  float m_output_scale;

  // u32 shader_index;
  // u32 shader_pass_index;

  void WriteShaderIndices(ShaderCode& shader_source) const;
};
}  // namespace VideoCommon::PE
