// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/PostProcessingConfig.h"

#ifdef _MSC_VER
#include <filesystem>
namespace fs = std::filesystem;
#define HAS_STD_FILESYSTEM
#endif

#include <string>
#include <string_view>

#include <fmt/format.h>

#include "Common/File.h"
#include "Common/FileUtil.h"
#include "Common/Image.h"
#include "Common/IniFile.h"
#include "Common/Logging/Log.h"

#include "VideoCommon/VideoCommon.h"
#include "VideoCommon/VideoConfig.h"

namespace VideoCommon::PostProcessing
{
u32 Option::GetComponents(OptionType type)
{
  switch (type)
  {
  case OptionType::Bool:
  case OptionType::Float:
  case OptionType::Int:
    return 1;

  case OptionType::Float2:
  case OptionType::Int2:
    return 2;

  case OptionType::Float3:
  case OptionType::Int3:
  case OptionType::RGB:
    return 3;

  case OptionType::Float4:
  case OptionType::Int4:
  case OptionType::RGBA:
    return 4;

  case OptionType::Float16:
    return 16;
  }

  return 0;
}

void RuntimeInfo::SetError(bool error)
{
  std::lock_guard lock(m_error_mutex);
  m_has_error = error;
}

bool RuntimeInfo::HasError() const
{
  std::lock_guard lock(m_error_mutex);
  return m_has_error;
}

Shader::Shader() = default;
Shader::~Shader() = default;

bool Shader::LoadShader(const std::string& path)
{
  if (!File::Exists(path))
    return false;

  // Read to a single string we can work with
  std::string code;
  if (!File::ReadFileToString(path, code))
    return false;

  std::string replaced_code = ReplaceAll(std::move(code), "\r\n", "\n");

  SplitPath(path, &m_base_path, &m_name, nullptr);
  m_full_path = path;
  const bool result = ParseShader(std::move(replaced_code));
  if (result)
    m_runtime_info = std::make_shared<RuntimeInfo>();

  BuildGroupFromOptions();

  return result;
}

bool Shader::ParseShader(const std::string& source)
{
  // Find configuration block, if any
  constexpr std::string_view config_start_delimiter = "[configuration]";
  constexpr std::string_view config_end_delimiter = "[/configuration]";
  const size_t configuration_start = source.find(config_start_delimiter);
  const size_t configuration_end = source.find(config_end_delimiter);

  if (configuration_start == std::string::npos && configuration_end == std::string::npos)
  {
    // If there is no configuration block. Assume the entire file is code.
    m_source = source;
    CreateDefaultPass();
    return true;
  }

  // Remove the configuration area from the source string, leaving only the GLSL code.
  if (configuration_start > 0)
    m_source = source.substr(0, configuration_start);
  if (configuration_end != source.length())
    m_source += source.substr(configuration_end + config_end_delimiter.size());

  // Extract configuration string, and parse options/passes
  std::string configuration_string =
      source.substr(configuration_start + config_start_delimiter.size(),
                    configuration_end - configuration_start - config_start_delimiter.size());
  return ParseConfiguration(configuration_string);
}

bool Shader::ParseConfiguration(const std::string& source)
{
  std::vector<ConfigBlock> config_blocks = ReadConfigSections(source);
  if (!ParseConfigSections(config_blocks))
    return false;

  // If no render passes are specified, create a default pass.
  if (m_passes.empty())
    CreateDefaultPass();

  return true;
}

std::vector<Shader::ConfigBlock> Shader::ReadConfigSections(const std::string& source)
{
  std::istringstream in(source);

  std::vector<ConfigBlock> config_blocks;
  ConfigBlock* current_block = nullptr;

  while (!in.eof())
  {
    std::string line_str;
    if (std::getline(in, line_str))
    {
      std::string_view line = line_str;

      if (line.empty())
        continue;

      if (line[0] == '[')
      {
        size_t endpos = line.find("]");

        if (endpos != std::string::npos)
        {
          const std::string sub{line.substr(1, endpos - 1)};
          ConfigBlock section;
          section.name = sub;
          config_blocks.push_back(std::move(section));
          current_block = &config_blocks.back();
        }
      }
      else
      {
        std::string key, value;
        IniFile::ParseLine(line, &key, &value);
        if (!key.empty() && !value.empty())
        {
          if (current_block)
            current_block->values.emplace_back(key, value);
        }
      }
    }
  }

  return config_blocks;
}

bool Shader::ParseConfigSections(const std::vector<ConfigBlock>& config_blocks)
{
  for (const ConfigBlock& option : config_blocks)
  {
    if (option.name == "Pass")
    {
      if (!ParsePassBlock(option))
        return false;
    }
    else if (option.name == "Meta")
    {
      if (!ParseMetaBlock(option))
        return false;
    }
    else
    {
      if (!ParseOptionBlock(option))
        return false;
    }
  }

  return true;
}

bool Shader::ParseMetaBlock(const ConfigBlock& block)
{
  for (const auto& key : block.values)
  {
    if (key.first == "Author")
    {
      m_author = key.second;
    }
    else if (key.first == "Description")
    {
      m_description = key.second;
    }
  }

  return true;
}

bool Shader::ParseOptionBlock(const ConfigBlock& block)
{
  // Initialize to default values, in case the configuration section is incomplete.
  Option option;
  u32 num_values = 0;

  static constexpr std::array<std::tuple<const char*, OptionType, u32>, 12> types = {{
      {"OptionBool", OptionType::Bool, 1},
      {"OptionFloat", OptionType::Float, 1},
      {"OptionFloat2", OptionType::Float2, 2},
      {"OptionFloat3", OptionType::Float3, 3},
      {"OptionFloat4", OptionType::Float4, 4},
      {"OptionFloat16", OptionType::Float16, 16},
      {"OptionInt", OptionType::Int, 1},
      {"OptionInt2", OptionType::Int2, 2},
      {"OptionInt3", OptionType::Int3, 3},
      {"OptionInt4", OptionType::Int4, 4},
      {"OptionRGB", OptionType::RGB, 3},
      {"OptionRGBA", OptionType::RGBA, 4},
  }};

  for (const auto& it : types)
  {
    if (block.name == std::get<0>(it))
    {
      option.type = std::get<1>(it);
      num_values = std::get<2>(it);
      break;
    }
  }
  if (num_values == 0)
  {
    ERROR_LOG_FMT(VIDEO, "Unknown section name in post-processing shader config: '{}'",
              block.name);
    return false;
  }

  for (const auto& key : block.values)
  {
    if (key.first == "GUIName")
    {
      option.gui_name = key.second;
    }
    else if (key.first == "OptionName")
    {
      option.option_name = key.second;
    }
    else if (key.first == "Description")
    {
      option.description = key.second;
    }
    else if (key.first == "Group")
    {
      option.group_name = key.second;
    }
    else if (key.first == "DependentOption")
    {
      option.dependent_option = key.second;
    }
    else if (key.first == "ResolveAtCompilation")
    {
      TryParse(key.second, &option.compile_time_constant);
    }
    else if (key.first == "MinValue" || key.first == "MaxValue" || key.first == "DefaultValue" ||
             key.first == "StepAmount")
    {
      OptionValue* value_ptr;

      if (key.first == "MinValue")
        value_ptr = &option.min_value;
      else if (key.first == "MaxValue")
        value_ptr = &option.max_value;
      else if (key.first == "DefaultValue")
        value_ptr = &option.default_value;
      else if (key.first == "StepAmount")
        value_ptr = &option.step_amount;
      else
        continue;

      bool result = false;
      switch (option.type)
      {
      case OptionType::Bool:
        result = TryParse(key.second, &value_ptr->bool_value);
        break;

      case OptionType::Float:
      case OptionType::Float2:
      case OptionType::Float3:
      case OptionType::Float4:
      case OptionType::RGB:
      case OptionType::RGBA:
      case OptionType::Float16:
      {
        std::vector<float> temp;
        result = TryParseVector(key.second, &temp);
        if (result && !temp.empty())
        {
          std::copy(temp.begin(), temp.end(), value_ptr->float_values.begin());
        }
      }
      break;

      case OptionType::Int:
      case OptionType::Int2:
      case OptionType::Int3:
      case OptionType::Int4:
      {
        std::vector<s32> temp;
        result = TryParseVector(key.second, &temp);
        if (result && !temp.empty())
        {
          std::copy(temp.begin(), temp.end(), value_ptr->int_values.begin());
        }
      }
      break;
      }

      if (!result)
      {
        ERROR_LOG_FMT(VIDEO, "File '{}' value parse fail at section '{}' key '{}' value '{}'",
                  m_name, block.name, key.first, key.second);
        return false;
      }
    }
    else
    {
      ERROR_LOG_FMT(VIDEO, "Unknown key '{}' in section '{}'", block.name, key.first);
      return false;
    }
  }

  option.value = option.default_value;
  m_options[option.option_name] = option;
  return true;
}

bool Shader::ParsePassBlock(const ConfigBlock& block)
{
  Pass pass;
  for (const auto& option : block.values)
  {
    const std::string& key = option.first;
    const std::string& value = option.second;
    if (key == "GUIName")
    {
      pass.gui_name = value;
    }
    else if (key == "EntryPoint")
    {
      pass.entry_point = value;
    }
    else if (key == "OutputScale")
    {
      TryParse(value, &pass.output_scale);
      if (pass.output_scale <= 0.0f)
        return false;
    }
    else if (key == "OutputScaleNative")
    {
      TryParse(value, &pass.output_scale);
      if (pass.output_scale <= 0.0f)
        return false;

      // negative means native scale
      pass.output_scale = -pass.output_scale;
    }
    else if (key == "DependantOption")
    {
      const auto& dependant_option = m_options.find(value);
      if (dependant_option == m_options.end())
      {
        ERROR_LOG_FMT(VIDEO, "Unknown dependant option: {}", value);
        return false;
      }

      pass.dependent_option = value;
      dependant_option->second.is_pass_dependent_option = true;
    }
    else if (key.compare(0, 5, "Input") == 0 && key.length() > 5)
    {
      u32 texture_unit = key[5] - '0';
      if (texture_unit > POST_PROCESSING_MAX_TEXTURE_INPUTS)
      {
        ERROR_LOG_FMT(VIDEO, "Post processing configuration error: Out-of-range texture unit: {}",
                  texture_unit);
        return false;
      }

      // Input declared yet?
      Input* input = nullptr;
      for (Input& input_it : pass.inputs)
      {
        if (input_it.texture_unit == texture_unit)
        {
          input = &input_it;
          break;
        }
      }
      if (!input)
      {
        Input new_input;
        new_input.texture_unit = texture_unit;
        pass.inputs.push_back(std::move(new_input));
        input = &pass.inputs.back();
      }

      // Input#(Filter|Mode|Source)
      std::string extra = (key.length() > 6) ? key.substr(6) : "";
      if (extra.empty())
      {
        // Type
        if (value == "ColorBuffer")
        {
          input->type = InputType::ColorBuffer;
        }
        else if (value == "DepthBuffer")
        {
          input->type = InputType::DepthBuffer;
          m_requires_depth_buffer = true;
        }
        else if (value == "PreviousPass")
        {
          input->type = InputType::PreviousPassOutput;
        }
        else if (value.compare(0, 4, "Pass") == 0)
        {
          input->type = InputType::PassOutput;
          if (!TryParse(value.substr(4), &input->pass_output_index) ||
              input->pass_output_index >= m_passes.size())
          {
            ERROR_LOG_FMT(VIDEO, "Out-of-range render pass reference: {}", input->pass_output_index);
            return false;
          }
        }
        else
        {
          ERROR_LOG_FMT(VIDEO, "Invalid input type: {}", value);
          return false;
        }
      }
      else if (extra == "Filter")
      {
        if (value == "Nearest")
        {
          input->sampler_state.min_filter = SamplerState::Filter::Linear;
          input->sampler_state.mag_filter = SamplerState::Filter::Linear;
          input->sampler_state.mipmap_filter = SamplerState::Filter::Linear;
        }
        else if (value == "Linear")
        {
          input->sampler_state.min_filter = SamplerState::Filter::Point;
          input->sampler_state.mag_filter = SamplerState::Filter::Point;
          input->sampler_state.mipmap_filter = SamplerState::Filter::Point;
        }
        else
        {
          ERROR_LOG_FMT(VIDEO, "Invalid input filter: {}", value);
          return false;
        }
      }
      else if (extra == "Mode")
      {
        if (value == "Clamp")
        {
          input->sampler_state.wrap_u = SamplerState::AddressMode::Clamp;
          input->sampler_state.wrap_v = SamplerState::AddressMode::Clamp;
        }
        else if (value == "Wrap")
        {
          input->sampler_state.wrap_u = SamplerState::AddressMode::Repeat;
          input->sampler_state.wrap_v = SamplerState::AddressMode::Repeat;
        }
        else if (value == "WrapMirror")
        {
          input->sampler_state.wrap_u = SamplerState::AddressMode::MirroredRepeat;
          input->sampler_state.wrap_v = SamplerState::AddressMode::MirroredRepeat;
        }
        else if (value == "Border")
        {
          input->sampler_state.wrap_u = SamplerState::AddressMode::Border;
          input->sampler_state.wrap_v = SamplerState::AddressMode::Border;
        }
        else
        {
          ERROR_LOG_FMT(VIDEO, "Invalid input mode: {}", value);
          return false;
        }
      }
      else if (extra == "Source")
      {
        // Load external image
#ifdef HAS_STD_FILESYSTEM
        const fs::path path_value = fs::path(value);
        const std::string image_path =
            path_value.is_relative() ?
                fs::path(m_base_path).append(value).string() :
                value;
#else
        const std::string image_path = value.front() != '/' ? m_base_path + value : value;
#endif
        input->type = InputType::ExternalImage;
        if (!LoadExternalImage(image_path, input->external_image))
        {
          ERROR_LOG_FMT(VIDEO, "Unable to load external image at '{}'", image_path);
          return false;
        }
      }
      else
      {
        ERROR_LOG_FMT(VIDEO, "Unknown input key: {}", key);
        return false;
      }
    }
  }

  m_passes.push_back(std::move(pass));
  return true;
}

void Shader::BuildGroupFromOptions()
{
  m_groups.clear();

  for (auto& [option_name, option] : m_options)
  {
    auto [iter, success] = m_groups.try_emplace(option.group_name, std::vector<std::string>{});
    iter->second.push_back(option_name);
  }
}

bool Shader::LoadExternalImage(const std::string& path, ExternalImage& image)
{
  File::IOFile file(path, "rb");
  std::vector<u8> buffer(file.GetSize());
  if (!file.IsOpen() || !file.ReadBytes(buffer.data(), file.GetSize()))
    return false;

  return Common::LoadPNG(buffer, &image.data, &image.width, &image.height);
}

void Shader::CreateDefaultPass()
{
  Input input;
  input.type = InputType::PreviousPassOutput;
  input.sampler_state = RenderState::GetLinearSamplerState();

  Pass pass;
  pass.entry_point = "main";
  pass.inputs.push_back(std::move(input));
  pass.output_scale = 1.0f;
  m_passes.push_back(std::move(pass));
}

void Shader::SetOptionf(const char* option, int index, float value)
{
  auto it = m_options.find(option);
  SetOptionf(it->second, index, value);
}

void Shader::SetOptioni(const char* option, int index, s32 value)
{
  auto it = m_options.find(option);
  SetOptioni(it->second, index, value);
}

void Shader::SetOptionb(const char* option, bool value)
{
  auto it = m_options.find(option);
  SetOptionb(it->second, value);
}

void Shader::SetOptionf(Option& option, int index, float value)
{
  if (index >= option.value.float_values.size())
    return;

  if (option.value.float_values[index] == value)
    return;

  option.value.float_values[index] = value;
}

void Shader::SetOptioni(Option& option, int index, s32 value)
{
  if (index >= option.value.int_values.size())
    return;

  if (option.value.int_values[index] == value)
    return;

  option.value.int_values[index] = value;
}

void Shader::SetOptionb(Option& option, bool value)
{
  if (option.value.bool_value == value)
    return;

  option.value.bool_value = value;
}

const Option* Shader::FindOption(const char* option) const
{
  auto iter = m_options.find(option);
  return iter != m_options.end() ? &iter->second : nullptr;
}

Option* Shader::FindOption(const char* option)
{
  auto iter = m_options.find(option);
  return iter != m_options.end() ? &iter->second : nullptr;
}

void Shader::Reset()
{
  for (auto& [option_name, option] : m_options)
  {
    option.value = option.default_value;
  }
}

void Shader::Reload()
{
  LoadShader(m_full_path);
}

std::shared_ptr<RuntimeInfo> Shader::GetRuntimeInfo() const
{
  return m_runtime_info;
}

void Shader::SetToSlot(SnapshotSlot slot)
{
  m_snapshot_history[static_cast<int>(slot)] = m_options;
}

void Shader::LoadFromSlot(SnapshotSlot slot)
{
  auto& options = m_snapshot_history[static_cast<int>(slot)];
  if (options)
    m_options = *options;
}

bool Shader::IsSlotSet(SnapshotSlot slot) const
{
  auto& options = m_snapshot_history[static_cast<int>(slot)];
  return options.has_value();
}

Config::Config(std::string type) : m_type(std::move(type))
{
}

Config::~Config() = default;

bool Config::IsValid() const
{
  int shaders_valid = 0;
  for (auto& shader : m_shaders)
  {
    if (!shader.IsEnabled())
      continue;

    if (shader.GetRuntimeInfo()->HasError())
      return false;

    shaders_valid++;
  }

  if (shaders_valid == 0)
    return false;

  return true;
}

void Config::AddShader(const std::string& shader_file)
{
  Shader shader;
  shader.LoadShader(shader_file);
  AddShader(std::move(shader));
}

void Config::AddShader(Shader shader)
{
  m_shaders.push_back(std::move(shader));
  const u32 index = static_cast<u32>(m_shaders.size());
  m_shader_indices.push_back(index - 1);
  m_change_count++;
}

void Config::RemoveShader(u32 index_to_remove)
{
  auto it = std::find_if(m_shader_indices.begin(), m_shader_indices.end(),
                         [index_to_remove](u32 index) { return index == index_to_remove; });
  if (it == m_shader_indices.end())
    return;
  m_shader_indices.erase(it);

  m_shaders.erase(m_shaders.begin() + index_to_remove);
  m_change_count++;
}

void Config::ClearShaders()
{
  m_shaders.clear();
  m_shader_indices.clear();
  m_change_count++;
}

bool Config::LoadFromFile(const std::string& config)
{
  m_requires_depth_buffer = false;

  IniFile file;
  file.Load(config);

  IniFile::Section* general_section = file.GetOrCreateSection(m_type + ".General");

  std::string shader_list_str;
  general_section->Get("Shaders", &shader_list_str, "");
  std::vector<std::string> shader_pathes = SplitString(shader_list_str, ',');

  std::string shader_order_str;
  general_section->Get("Order", &shader_order_str, "");
  std::vector<std::string> shader_order = SplitString(shader_order_str, ',');

  if (shader_pathes.size() != shader_order.size())
    return false;

  std::vector<u32> shader_order_indices;
  shader_order_indices.reserve(shader_order.size());
  for (std::size_t i = 0; i < shader_order.size(); i++)
  {
    const std::string index_as_str = shader_order[i];
    u32 index;
    if (!TryParse<u32>(index_as_str, &index))
      return false;

    shader_order_indices.push_back(index);
  }

  bool change_occurred = m_shaders.size() != shader_order.size();
  m_shaders.resize(shader_order.size());
  m_shader_indices.resize(shader_order.size());
  for (size_t i = 0; i < shader_order.size(); i++)
  {
    const std::string shader_path = shader_pathes[i];
    if (m_shader_indices[i] != shader_order_indices[i])
    {
      m_shader_indices[i] = shader_order_indices[i];
      change_occurred = true;
    }
    Shader& shader = m_shaders[i];
    if (shader.GetSourcePath() != shader_path)
    {
      shader = Shader{};
      if (!shader.LoadShader(shader_path))
        return false;
    }

    IniFile::Section* shader_section =
        file.GetOrCreateSection(m_type + "." + shader.GetShaderName());

    bool enabled_config_value;
    if (shader_section->Get<bool>("Enabled", &enabled_config_value))
      shader.SetEnabled(enabled_config_value);

    const auto set_option_floats_from_string = [shader_section, &shader,
                                                &change_occurred](Option& option) {
      std::string config_value;
      if (shader_section->Get(option.option_name, &config_value))
      {
        const std::vector<std::string> values = SplitString(config_value, ',');
        for (int i = 0; i < static_cast<int>(values.size()); i++)
        {
          const std::string str_value = values[i];
          float value;
          if (TryParse(str_value, &value))
          {
            shader.SetOptionf(option, i, value);
            if (option.compile_time_constant || option.is_pass_dependent_option)
              change_occurred = true;
          }
        }
      }
    };

    const auto set_option_ints_from_string = [shader_section, &shader,
                                              &change_occurred](Option& option) {
      std::string config_value;
      if (shader_section->Get(option.option_name, &config_value))
      {
        const std::vector<std::string> values = SplitString(config_value, ',');
        for (int i = 0; i < static_cast<int>(values.size()); i++)
        {
          const std::string str_value = values[i];
          int value;
          if (TryParse(str_value, &value))
          {
            shader.SetOptionf(option, i, value);
            if (option.compile_time_constant || option.is_pass_dependent_option)
              change_occurred = true;
          }
        }
      }
    };

    for (auto& [option_name, option] : shader.GetOptions())
    {
      switch (option.type)
      {
      case OptionType::Bool:
      {
        bool config_value;
        if (shader_section->Get(option.option_name, &config_value))
          shader.SetOptionb(option, config_value);
      }
      break;
      case OptionType::Float:
      {
        float config_value;
        if (shader_section->Get(option.option_name, &config_value))
          shader.SetOptionf(option, 0, config_value);
      }
      break;
      case OptionType::Float2:
      case OptionType::Float3:
      case OptionType::Float4:
      case OptionType::RGB:
      case OptionType::RGBA:
      {
        set_option_floats_from_string(option);
      }
      break;
      case OptionType::Float16:
      {
        set_option_floats_from_string(option);
      }
      break;
      case OptionType::Int:
      {
        int config_value;
        if (shader_section->Get(option.option_name, &config_value))
          shader.SetOptioni(option, 0, config_value);
      }
      break;
      case OptionType::Int2:
      {
        set_option_ints_from_string(option);
      }
      break;
      case OptionType::Int3:
      {
        set_option_ints_from_string(option);
      }
      break;
      case OptionType::Int4:
      {
        set_option_ints_from_string(option);
      }
      break;
      };
    }

    m_requires_depth_buffer |= shader.RequiresDepthBuffer();
  }

  if (change_occurred)
    IncrementChangeCount();

  return true;
}

void Config::SaveToFile(const std::string& filename)
{
  IniFile file;
  file.Load(filename);

  std::vector<std::string> shader_pathes;
  for (const auto& shader : m_shaders)
  {
    shader_pathes.push_back(shader.GetSourcePath());
    IniFile::Section* shader_section =
        file.GetOrCreateSection(m_type + "." + shader.GetShaderName());

    shader_section->Set<bool>("Enabled", shader.IsEnabled());

    for (auto& [option_name, option] : shader.GetOptions())
    {
      switch (option.type)
      {
      case OptionType::Bool:
      {
        shader_section->Set(option.option_name, ValueToString(option.value.bool_value));
      }
      break;
      case OptionType::Float:
      {
        shader_section->Set(option.option_name, ValueToString(option.value.float_values[0]));
      }
      break;
      case OptionType::Float2:
      case OptionType::Float3:
      case OptionType::Float4:
      case OptionType::RGB:
      case OptionType::RGBA:
      {
        std::vector<std::string> values;
        for (u32 i = 0; i < Option::GetComponents(option.type); i++)
        {
          values.push_back(ValueToString(option.value.float_values[i]));
        }
        shader_section->Set(option.option_name, JoinStrings(values, ","));
      }
      break;
      case OptionType::Float16:
      {
        std::vector<std::string> values;
        for (int i = 0; i < 16; i++)
        {
          values.push_back(ValueToString(option.value.float_values[i]));
        }
        shader_section->Set(option.option_name, JoinStrings(values, ","));
      }
      break;
      case OptionType::Int:
      {
        shader_section->Set(option.option_name, ValueToString(option.value.int_values[0]));
      }
      break;
      case OptionType::Int2:
      case OptionType::Int3:
      case OptionType::Int4:
      {
        std::vector<std::string> values;
        for (u32 i = 0; i < Option::GetComponents(option.type); i++)
        {
          values.push_back(ValueToString(option.value.int_values[i]));
        }
        shader_section->Set(option.option_name, JoinStrings(values, ","));
      }
      break;
      };
    }
  }

  std::vector<std::string> shader_order;
  for (const u32 index : m_shader_indices)
  {
    shader_order.push_back(fmt::format("{}", index));
  }

  IniFile::Section* general_section = file.GetOrCreateSection(m_type + ".General");
  general_section->Set("Order", JoinStrings(shader_order, ","));
  general_section->Set("Shaders", JoinStrings(shader_pathes, ","));

  file.Save(filename);
}

}  // namespace VideoCommon::PostProcessing
