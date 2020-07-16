// Copyright 2020 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "Common/CommonTypes.h"
#include "VideoCommon/RenderState.h"

namespace VideoCommon::PostProcessing
{
enum class OptionType : u32
{
  Bool,
  Float,
  Float2,
  Float3,
  Float4,
  Float16,
  Int,
  Int2,
  Int3,
  Int4,
  RGB,
  RGBA
};

enum class GUIType
{
  Spinbox,
  Slider,
  Combobox
};

enum class InputType : u32
{
  ExternalImage,       // external image loaded from file
  ColorBuffer,         // colorbuffer at internal resolution
  DepthBuffer,         // depthbuffer at internal resolution
  PreviousPassOutput,  // output of the previous pass
  PassOutput           // output of a previous pass
};

// Maximum number of texture inputs to a post-processing shader.
static const size_t POST_PROCESSING_MAX_TEXTURE_INPUTS = 4;

union OptionValue
{
  bool bool_value;
  std::array<float, 4> float_values;
  std::array<s32, 4> int_values;
};

struct Option final
{
  static u32 GetComponents(OptionType type);
  static bool GUITypeValid(OptionType type, GUIType gui_type);

  OptionType type = OptionType::Float;
  GUIType gui_type;

  OptionValue default_value = {};
  OptionValue min_value = {};
  OptionValue max_value = {};
  OptionValue step_amount = {};

  OptionValue value = {};

  std::string description;
  std::string gui_name;
  std::string option_name;
  std::string dependent_option;
  std::string group_name;

  bool compile_time_constant = false;
  bool is_pass_dependent_option = false;
};

struct ExternalImage final
{
  ExternalImage() = default;
  ~ExternalImage() = default;
  ExternalImage(const ExternalImage& other)
  {
    std::copy(other.data.begin(), other.data.end(), data.begin());
    width = other.width;
    height = other.height;
  }
  ExternalImage(ExternalImage&&) = default;

  ExternalImage& operator=(const ExternalImage& other)
  {
    std::copy(other.data.begin(), other.data.end(), data.begin());
    width = other.width;
    height = other.height;
    return *this;
  }
  ExternalImage& operator=(ExternalImage&&) = default;

  std::vector<u8> data;
  u32 width;
  u32 height;
};

struct Input final
{
  Input() = default;
  ~Input() = default;
  Input(const Input&) = default;
  Input(Input&&) = default;

  Input& operator=(const Input&) = default;
  Input& operator=(Input&&) = default;

  InputType type = InputType::PreviousPassOutput;
  SamplerState sampler_state = RenderState::GetLinearSamplerState();
  u32 texture_unit = 0;
  u32 pass_output_index = 0;

  ExternalImage external_image;
};

struct Pass final
{
  std::vector<Input> inputs;
  std::string entry_point;
  std::string dependent_option;
  float output_scale = 1.0f;
  std::string gui_name;
};

// Note: this class can be accessed by both
// the video thread and the UI
// thread
class RuntimeInfo
{
public:
  bool HasError() const;
  void SetError(bool error);

private:
  mutable std::mutex m_error_mutex;
  bool m_has_error = false;
};

class Shader
{
public:
  using OptionMap = std::map<std::string, Option>;
  using GroupMap = std::map<std::string, std::vector<std::string>>;
  using RenderPassList = std::vector<Pass>;

  Shader();
  ~Shader();

  // Loads the configuration with a shader
  bool LoadShader(const std::string& filename);

  const std::string& GetShaderName() const { return m_name; }
  const std::string& GetAuthor() const { return m_author; }
  const std::string& GetDescription() const { return m_description; }
  std::string GetSourcePath() const { return m_full_path; }
  const std::string& GetShaderSource() const { return m_source; }

  bool RequiresDepthBuffer() const { return m_requires_depth_buffer; }

  bool HasOptions() const { return !m_options.empty(); }
  OptionMap& GetOptions() { return m_options; }
  const OptionMap& GetOptions() const { return m_options; }
  const Option* FindOption(const char* option) const;
  Option* FindOption(const char* option);

  const GroupMap& GetGroups() const { return m_groups; }

  const RenderPassList& GetPasses() const { return m_passes; }
  const Pass& GetPass(u32 index) const { return m_passes.at(index); }
  Pass& GetPass(u32 index) { return m_passes.at(index); }
  const u32 GetPassCount() const { return static_cast<u32>(m_passes.size()); }

  bool IsEnabled() const { return m_enabled; }
  void SetEnabled(bool enabled) { m_enabled = enabled; }

  // For updating option's values
  void SetOptionf(const char* option, int index, float value);
  void SetOptioni(const char* option, int index, s32 value);
  void SetOptionb(const char* option, bool value);
  void SetOptionf(Option& option, int index, float value);
  void SetOptioni(Option& option, int index, s32 value);
  void SetOptionb(Option& option, bool value);

  void Reset();
  void Reload();

  std::shared_ptr<RuntimeInfo> GetRuntimeInfo() const;

  enum class SnapshotSlot
  {
    First = 0,
    Second = 1,
    Third = 2,
    Fourth = 3,
    Fifth = 4,
    Max = 5
  };

  void SetToSlot(SnapshotSlot slot);
  void LoadFromSlot(SnapshotSlot slot);
  bool IsSlotSet(SnapshotSlot slot) const;

private:
  // Intermediate block used while parsing.
  struct ConfigBlock final
  {
    std::string name;
    std::vector<std::pair<std::string, std::string>> values;
  };

  static bool LoadExternalImage(const std::string& path, ExternalImage& image);

  bool ParseShader(const std::string& source);

  void CreateDefaultPass();
  bool ParseConfiguration(const std::string& source);
  std::vector<ConfigBlock> ReadConfigSections(const std::string& source);
  bool ParseConfigSections(const std::vector<ConfigBlock>& blocks);
  bool ParseMetaBlock(const ConfigBlock& block);
  bool ParseOptionBlock(const ConfigBlock& block);
  bool ParsePassBlock(const ConfigBlock& block);

  void BuildGroupFromOptions();

  std::string m_name;
  std::string m_author;
  std::string m_description;
  std::string m_source;
  std::string m_base_path;
  std::string m_full_path;
  OptionMap m_options;
  GroupMap m_groups;
  std::array<std::optional<OptionMap>, static_cast<int>(SnapshotSlot::Max)> m_snapshot_history;
  RenderPassList m_passes;
  bool m_requires_depth_buffer = false;
  bool m_enabled = false;
  std::shared_ptr<RuntimeInfo> m_runtime_info;
};

class Config final
{
public:
  explicit Config(std::string type);
  Config(Config&&) = delete;
  Config(const Config&) = default;
  Config& operator=(Config&&) = delete;
  Config& operator=(const Config&) = default;
  ~Config();

  bool IsValid() const;
  bool RequiresDepthBuffer() const { return m_requires_depth_buffer; }

  void AddShader(const std::string& shader_path);
  void RemoveShader(u32 index_to_remove);
  void ClearShaders();
  const Shader& GetShader(u32 index) const { return m_shaders.at(index); }
  Shader& GetShader(u32 index) { return m_shaders.at(index); }
  const u32 GetShaderCount() const { return static_cast<u32>(m_shaders.size()); }

  void IncrementChangeCount() { m_change_count++; }
  u64 GetChangeCount() const { return m_change_count; }

  const std::vector<u32>& GetShaderIndices() const { return m_shader_indices; }
  void SetShaderIndices(std::vector<u32> indices) { m_shader_indices = std::move(indices); }

  bool LoadFromFile(const std::string& file);
  void SaveToFile(const std::string& file);

private:
  void AddShader(Shader shader);

  std::vector<Shader> m_shaders;
  std::vector<u32> m_shader_indices;

  std::string m_type;

  u64 m_change_count = 0;
  bool m_requires_depth_buffer = false;
};
}  // namespace VideoCommon::PostProcessing
