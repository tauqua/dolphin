// Copyright 2021 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/PEShaderSystem/Config/PEShaderConfig.h"

#include <fmt/format.h>

#include <algorithm>
#include <sstream>

#include "Common/FileUtil.h"
#include "Common/Logging/Log.h"
#include "VideoCommon/PEShaderSystem/Constants.h"

namespace
{
std::string GetStreamAsString(std::ifstream& stream)
{
  std::stringstream ss;
  ss << stream.rdbuf();
  return ss.str();
}
}  // namespace

namespace VideoCommon::PE
{
bool RuntimeInfo::HasError() const
{
  return m_error.load();
}

void RuntimeInfo::SetError(bool error)
{
  m_error.store(error);
}

std::map<std::string, std::vector<ShaderConfigOption*>> ShaderConfig::GetGroups()
{
  std::map<std::string, std::vector<ShaderConfigOption*>> result;
  for (auto& option : m_options)
  {
    const std::string group = GetOptionGroup(option);
    result[group].push_back(&option);
  }
  return result;
}

void ShaderConfig::Reset()
{
  for (auto& option : m_options)
  {
    PE::Reset(option);
  }
  m_changes += 1;
}

void ShaderConfig::Reload()
{
  m_compiletime_changes += 1;
}

void ShaderConfig::LoadSnapshot(std::size_t channel)
{
  for (auto& option : m_options)
  {
    PE::LoadSnapshot(option, channel);
  }
  m_changes += 1;
}

void ShaderConfig::SaveSnapshot(std::size_t channel)
{
  for (auto& option : m_options)
  {
    PE::SaveSnapshot(option, channel);
  }
}

bool ShaderConfig::HasSnapshot(std::size_t channel) const
{
  return std::any_of(m_options.begin(), m_options.end(), [&](const ShaderConfigOption& option) {
    return PE::HasSnapshot(option, channel);
  });
}

bool ShaderConfig::DeserializeFromConfig(const picojson::value& value)
{
  const auto& meta = value.get("meta");
  if (meta.is<picojson::object>())
  {
    const auto& author = meta.get("author");
    if (author.is<std::string>())
    {
      m_author = author.to_str();
    }

    const auto& description = meta.get("description");
    if (description.is<std::string>())
    {
      m_description = description.to_str();
    }
  }

  const auto& options = value.get("options");
  if (options.is<picojson::array>())
  {
    for (const auto& option_val : options.get<picojson::array>())
    {
      if (!option_val.is<picojson::object>())
      {
        ERROR_LOG_FMT(VIDEO, "Failed to load shader configuration file, specified option is not a json object");
        return false;
      }
      const auto option = DeserializeOptionFromConfig(option_val.get<picojson::object>());
      if (!option)
      {
        return false;
      }

      m_options.push_back(*option);
    }
  }

  const auto& passes = value.get("passes");
  if (passes.is<picojson::array>())
  {
    const auto& passes_arr = passes.get<picojson::array>();
    for (const auto& pass_val : passes_arr)
    {
      if (!pass_val.is<picojson::object>())
      {
        ERROR_LOG_FMT(
            VIDEO,
            "Failed to load shader configuration file, specified pass is not a json object");
        return false;
      }
      ShaderConfigPass pass;
      if (!pass.DeserializeFromConfig(pass_val.get<picojson::object>(), passes_arr.size()))
      {
        return false;
      }

      m_passes.push_back(std::move(pass));
    }
  }

  return true;
}

std::optional<ShaderConfig> ShaderConfig::Load(const std::string& shader_path)
{
  ShaderConfig result;
  result.m_runtime_info = std::make_shared<RuntimeInfo>();

  std::string basename;
  std::string base_path;
  SplitPath(shader_path, &base_path, &basename, nullptr);

  result.m_name = basename;
  result.m_shader_path = shader_path;

  const std::string config_file = fmt::format("{}/{}.json", base_path, basename);
  if (File::IsFile(config_file))
  {
    std::ifstream json_stream;
    File::OpenFStream(json_stream, config_file, std::ios_base::in);
    if (!json_stream.is_open())
    {
      ERROR_LOG_FMT(VIDEO, "Failed to load shader configuration file '{}'", config_file);
      return std::nullopt;
    }

    picojson::value root;
    const auto error = picojson::parse(root, GetStreamAsString(json_stream));

    if (!error.empty())
    {
      ERROR_LOG_FMT(VIDEO, "Failed to load shader configuration file '{}', due to parse error: {}",
                    config_file, error);
      return std::nullopt;
    }
    if (!result.DeserializeFromConfig(root))
    {
      return std::nullopt;
    }
  }
  else
  {
    result.m_description = "No description provided";
    result.m_author = "Unknown";
    result.m_passes.push_back(ShaderConfigPass::CreateDefaultPass());
  }

  return result;
}

ShaderConfig ShaderConfig::CreateDefaultShader()
{
  ShaderConfig result;
  result.m_description = "Default vertex/pixel shader";
  result.m_shader_path = fmt::format("{}/{}/{}", File::GetSysDirectory(),
                                     Constants::dolphin_shipped_internal_shader_directory, "DefaultVertexPixelShader.glsl");
  result.m_author = "Dolphin";
  result.m_passes.push_back(ShaderConfigPass::CreateDefaultPass());
  return result;
}

}  // namespace VideoCommon::PE
