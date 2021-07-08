// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <memory>

#include "Common/CommonTypes.h"
#include "VideoCommon/PEShaderSystem/Config/PEShaderConfigOption.h"

class ShaderCode;

namespace VideoCommon::PE
{
class ShaderOption
{
public:
  static std::unique_ptr<ShaderOption> Create(const ShaderConfigOption& config_option);
  ShaderOption() = default;
  virtual ~ShaderOption() = default;
  ShaderOption(const ShaderOption&) = default;
  ShaderOption(ShaderOption&&) = default;
  ShaderOption& operator=(const ShaderOption&) = default;
  ShaderOption& operator=(ShaderOption&&) = default;
  virtual void Update(const ShaderConfigOption& config_option) = 0;
  virtual void WriteToMemory(u8*& buffer) const = 0;
  void WriteShaderUniforms(ShaderCode& shader_source) const;
  void WriteShaderConstants(ShaderCode& shader_source) const;
  virtual std::size_t Size() const = 0;

protected:
  bool m_evaluate_at_compile_time = false;

private:
  virtual void WriteShaderUniformsImpl(ShaderCode& shader_source) const = 0;
  virtual void WriteShaderConstantsImpl(ShaderCode& shader_source) const = 0;
};
}  // namespace VideoCommon::PE
