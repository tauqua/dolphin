// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>

#include "Common/CommonTypes.h"
#include "VideoCommon/AbstractShader.h"
#include "VideoCommon/PEShaderSystem/Config/PEShaderConfig.h"
#include "VideoCommon/PEShaderSystem/Runtime/BuiltinUniforms.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShaderApplyOptions.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShaderPass.h"

class AbstractTexture;
class ShaderCode;

namespace VideoCommon::PE
{
class ShaderOption;
class Shader
{
public:
  bool UpdateConfig(const ShaderConfig& config);
  void CreateOptions(const ShaderConfig& config);
  bool CreatePasses(const ShaderConfig& config);
  bool CreateAllPassOutputTextures(u32 width, u32 height, u32 layers, AbstractTextureFormat format);
  void UpdatePassInputs(const AbstractTexture* previous_texture);
  bool RebuildPipeline(AbstractTextureFormat format, u32 layers);
  void Apply(bool skip_final_copy, const ShaderApplyOptions& options,
             const AbstractTexture* prev_pass_texture, float prev_pass_output_scale);

  const std::vector<ShaderPass>& GetPasses() const;

private:
  bool RecompilePasses(const ShaderConfig& config);
  void UploadUniformBuffer();
  std::unique_ptr<AbstractShader> CompileGeometryShader() const;
  void PrepareUniformHeader(ShaderCode& shader_source) const;
  std::shared_ptr<AbstractShader> CompileVertexShader(const ShaderConfig& config) const;
  std::unique_ptr<AbstractShader> CompilePixelShader(const ShaderConfig& config,
                                                     const ShaderPass& pass,
                                                     const ShaderConfigPass& config_pass) const;
  void PixelShaderFooter(ShaderCode& shader_source, const ShaderConfigPass& config_pass) const;
  void PixelShaderHeader(ShaderCode& shader_source, const ShaderPass& pass) const;
  void VertexShaderMain(ShaderCode& shader_source) const;
  std::vector<u8> m_uniform_staging_buffer;
  std::vector<ShaderPass> m_passes;
  std::vector<std::unique_ptr<ShaderOption>> m_options;
  BuiltinUniforms m_builtin_uniforms;
  u32 m_last_static_option_change_count = 0;
  u32 m_last_compile_option_change_count = 0;
  std::unique_ptr<AbstractShader> m_geometry_shader;
};
}  // namespace VideoCommon::PE
