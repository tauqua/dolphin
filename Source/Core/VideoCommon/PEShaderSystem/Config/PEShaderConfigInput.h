// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <optional>
#include <string>
#include <variant>

#include <picojson.h>

#include "Common/CommonTypes.h"
#include "VideoCommon/RenderState.h"

namespace VideoCommon::PE
{
struct UserImage
{
  SamplerState m_sampler_state;
  u32 m_texture_unit = 0;
  std::string m_path;
};

struct InternalImage
{
  SamplerState m_sampler_state;
  u32 m_texture_unit;
  std::string m_path;
};

struct ColorBuffer
{
  SamplerState m_sampler_state;
  u32 m_texture_unit = 0;
};

struct DepthBuffer
{
  SamplerState m_sampler_state;
  u32 m_texture_unit = 0;
};

struct PreviousPass
{
  SamplerState m_sampler_state;
  u32 m_texture_unit = 0;
};

struct ExplicitPass
{
  SamplerState m_sampler_state;
  u32 m_texture_unit = 0;
  u16 m_pass_index = 0;
};

using ShaderConfigInput =
    std::variant<UserImage, InternalImage, ColorBuffer, DepthBuffer, PreviousPass, ExplicitPass>;

std::optional<ShaderConfigInput> DeserializeInputFromConfig(const picojson::object& obj, std::size_t texture_unit, std::size_t total_passes);

}  // namespace VideoCommon::PE
