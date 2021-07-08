// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <string>

#include <fmt/format.h>

#include "Common/CommonTypes.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShaderOption.h"
#include "VideoCommon/ShaderGenCommon.h"

namespace VideoCommon::PE
{
template <std::size_t N>
class RuntimeFloatOption final : public ShaderOption
{
public:
  using OptionType = std::array<float, N>;

  explicit RuntimeFloatOption(std::string name) : m_name(std::move(name)) {}
  RuntimeFloatOption(std::string name, bool compile_time_constant, OptionType value)
      : m_name(std::move(name)), m_value(std::move(value))
  {
    m_evaluate_at_compile_time = compile_time_constant;
  }

  void Update(const OptionType& value) { m_value = value; }

  void Update(const ShaderConfigOption& config_option) override
  {
    using ConfigFloatOption = FloatOption<N>;
    if (auto v = std::get_if<ConfigFloatOption>(&config_option))
      m_value = v->m_value;
  }

  void WriteToMemory(u8*& buffer) const override
  {
    std::memcpy(buffer, m_value.data(), m_size);
    buffer += Size();
  }

  void WriteShaderUniformsImpl(ShaderCode& shader_source) const override
  {
    if constexpr (N == 1UL)
    {
      shader_source.Write("float {};\n", m_name);
    }
    else
    {
      shader_source.Write("float{} {};\n", N, m_name);
    }
    for (int i = 0; i < m_padding; i++)
    {
      shader_source.Write("float _{}{}_padding;\n", m_name, i);
    }
  }

  void WriteShaderConstantsImpl(ShaderCode& shader_source) const override
  {
    if constexpr (N == 1UL)
    {
      shader_source.Write("#define {} float({})\n", m_name, m_value[0]);
    }
    else
    {
      shader_source.Write("#define {} float{}({})\n", m_name, N, fmt::join(m_value, ","));
    }
  }

  std::size_t Size() const override
  {
    return m_size + (m_padding * sizeof(float));
  }

private:
  static_assert(N <= 4UL);

  static constexpr std::size_t m_size = N * sizeof(float);
  static constexpr std::size_t m_padding = 4UL - N;

  OptionType m_value;
  std::string m_name;
};

template <std::size_t N>
class RuntimeIntOption final : public ShaderOption
{
public:
  using OptionType = std::array<s32, N>;

  explicit RuntimeIntOption(std::string name) : m_name(std::move(name)) {}
  RuntimeIntOption(std::string name, bool compile_time_constant, OptionType value)
      : m_name(std::move(name)), m_value(std::move(value))
  {
    m_evaluate_at_compile_time = compile_time_constant;
  }

  void Update(const OptionType& value) { m_value = value; }

  void Update(const ShaderConfigOption& config_option) override
  {
    using ConfigIntOption = IntOption<N>;
    if (auto v = std::get_if<ConfigIntOption>(&config_option))
      m_value = v->m_value;
  }

  void WriteToMemory(u8*& buffer) const override
  {
    std::memcpy(buffer, m_value.data(), m_size);
    buffer += Size();
  }

  void WriteShaderUniformsImpl(ShaderCode& shader_source) const override
  {
    if constexpr (N == 1UL)
    {
      shader_source.Write("int {};\n", m_name);
    }
    else
    {
      shader_source.Write("int{} {};\n", N, m_name);
    }
    for (int i = 0; i < m_padding; i++)
    {
      shader_source.Write("int _{}{}_padding;\n", m_name, i);
    }
  }

  void WriteShaderConstantsImpl(ShaderCode& shader_source) const override
  {
    if constexpr (N == 1UL)
    {
      shader_source.Write("#define {} int({})\n", m_name, m_value[0]);
    }
    else
    {
      shader_source.Write("#define {} int{}({})\n", m_name, N, fmt::join(m_value, ","));
    }
  }

  std::size_t Size() const override { return m_size + (m_padding * sizeof(s32)); }

private:
  static_assert(N <= 4UL);

  static constexpr std::size_t m_size = N * sizeof(s32);
  static constexpr std::size_t m_padding = 4UL - N;

  OptionType m_value;
  std::string m_name;
};

class RuntimeBoolOption final : public ShaderOption
{
public:
  explicit RuntimeBoolOption(std::string name) : m_name(std::move(name)) {}
  RuntimeBoolOption(std::string name, bool compile_time_constant, bool value)
      : m_name(std::move(name)), m_value(value)
  {
    m_evaluate_at_compile_time = compile_time_constant;
  }

  void Update(bool value) { m_value = value; }

  void Update(const ShaderConfigOption& config_option) override
  {
    if (auto v = std::get_if<BoolOption>(&config_option))
      m_value = v->m_value;
  }

  void WriteToMemory(u8*& buffer) const override
  {
    std::memcpy(buffer, &m_value, m_size);
    buffer += Size();
  }

  void WriteShaderUniformsImpl(ShaderCode& shader_source) const override
  {
    shader_source.Write("bool {};\n", m_name);
    for (int i = 0; i < m_padding; i++)
    {
      shader_source.Write("bool _{}{}_padding;\n", m_name, i);
    }
  }

  void WriteShaderConstantsImpl(ShaderCode& shader_source) const override
  {
    shader_source.Write("#define {} bool({})\n", m_name, m_value);
  }

  std::size_t Size() const override { return m_size + (m_padding * sizeof(bool)); }

private:
  static constexpr std::size_t m_size = sizeof(bool);
  static constexpr std::size_t m_padding = 3;

  bool m_value;
  std::string m_name;
};
}  // namespace VideoCommon::PE
