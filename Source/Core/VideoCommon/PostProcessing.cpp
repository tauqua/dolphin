// Copyright 2014 Dolphilator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "VideoCommon/PostProcessing.h"

#include <sstream>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include "Common/Assert.h"
#include "Common/CommonFuncs.h"
#include "Common/CommonPaths.h"
#include "Common/CommonTypes.h"
#include "Common/FileSearch.h"
#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "Common/Logging/Log.h"
#include "Common/MsgHandler.h"
#include "Common/StringUtil.h"
#include "common/CommonFuncs.h"

#include "VideoCommon/AbstractFramebuffer.h"
#include "VideoCommon/AbstractPipeline.h"
#include "VideoCommon/AbstractShader.h"
#include "VideoCommon/AbstractTexture.h"
#include "VideoCommon/FramebufferManager.h"
#include "VideoCommon/FramebufferShaderGen.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/ShaderCache.h"
#include "VideoCommon/VertexManagerBase.h"
#include "VideoCommon/VideoCommon.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/XFMemory.h"

#include "imgui.h"

namespace VideoCommon::PostProcessing
{
Instance::Instance(Config* config) : m_config(config)
{
  m_timer.Start();

  if (m_config->IsValid())
  {
    CompilePasses();
  }
}

Instance::~Instance()
{
  m_timer.Stop();
}

bool Instance::CompilePasses()
{
  if (g_ActiveConfig.backend_info.bSupportsGeometryShaders && !m_geometry_shader)
  {
    m_geometry_shader = CompileGeometryShader();
    if (!m_geometry_shader)
      return false;
  }

  m_render_passes.clear();
  const auto& indices = m_config->GetShaderIndices();
  for (u32 indice_index = 0; indice_index < indices.size(); indice_index++)
  {
    const u32 shader_index = indices[indice_index];
    const Shader& shader = m_config->GetShader(shader_index);
    if (!shader.IsEnabled())
      continue;
    std::shared_ptr<AbstractShader> vertex_shader = CompileVertexShader(shader);
    if (!vertex_shader)
      return false;

    for (u32 pass_index = 0; pass_index < shader.GetPassCount(); pass_index++)
    {
      const Pass& pass = shader.GetPass(pass_index);

      // don't add passes which aren't enabled
      if (!pass.dependent_option.empty())
      {
        const Option* option =
            m_config->GetShader(shader_index).FindOption(pass.dependent_option.c_str());
        if (!option || option->type != OptionType::Bool || !option->value.bool_value)
          continue;
      }

      RenderPass render_pass;
      render_pass.output_scale = pass.output_scale;
      render_pass.shader_index = shader_index;
      render_pass.shader_pass_index = pass_index;
      render_pass.vertex_shader = vertex_shader;
      render_pass.pixel_shader = CompilePixelShader(shader, pass);
      if (!render_pass.pixel_shader)
        return false;

      render_pass.inputs.reserve(pass.inputs.size());
      for (const auto& input : pass.inputs)
      {
        InputBinding binding;
        if (!CreateInputBinding(shader_index, input, &binding))
          return false;

        render_pass.inputs.push_back(std::move(binding));
      }

      m_render_passes.push_back(std::move(render_pass));
    }
  }

  m_target_width = 0;
  m_target_height = 0;
  m_target_format = AbstractTextureFormat::Undefined;
  return true;
}

bool Instance::CreateInputBinding(u32 shader_index, const Input& input, InputBinding* binding)
{
  binding->type = input.type;
  binding->texture_unit = input.texture_unit;
  binding->sampler_state = input.sampler_state;
  binding->texture_ptr = nullptr;
  binding->source_pass_index = 0;

  if (binding->type == InputType::ExternalImage)
  {
    binding->owned_texture_ptr = g_renderer->CreateTexture(
        TextureConfig(input.external_image.width, input.external_image.height, 1, 1, 1,
                      AbstractTextureFormat::RGBA8, 0));
    if (!binding->owned_texture_ptr)
      return false;

    binding->owned_texture_ptr->Load(0, input.external_image.width, input.external_image.height,
                                     input.external_image.width, input.external_image.data.data(),
                                     input.external_image.width * sizeof(u32) *
                                         input.external_image.height);
    binding->texture_ptr = binding->owned_texture_ptr.get();
  }
  else if (binding->type == InputType::PassOutput)
  {
    binding->source_pass_index = GetRenderPassIndex(shader_index, input.pass_output_index);
    if (binding->source_pass_index >= m_render_passes.size())
    {
      // This should have been checked as part of the configuration parsing.
      return false;
    }
  }

  return true;
}

bool Instance::CreateOutputTextures(u32 width, u32 height, u32 layers, AbstractTextureFormat format)
{
  // Pipelines must be recompiled if format or layers(->GS) changes.
  const bool recompile_pipelines = m_target_format != format || m_target_layers != layers;

  m_target_width = width;
  m_target_height = height;
  m_target_layers = layers;
  m_target_format = format;

  for (RenderPass& pass : m_render_passes)
  {
    u32 output_width;
    u32 output_height;
    if (pass.output_scale < 0.0f)
    {
      const float native_scale = -pass.output_scale;
      const int native_width = width * EFB_WIDTH / g_renderer->GetTargetWidth();
      const int native_height = height * EFB_HEIGHT / g_renderer->GetTargetHeight();
      output_width =
          std::max(static_cast<u32>(std::round(((float)native_width * native_scale))), 1u);
      output_height =
          std::max(static_cast<u32>(std::round(((float)native_height * native_scale))), 1u);
    }
    else
    {
      output_width = std::max(1u, static_cast<u32>(width * pass.output_scale));
      output_height = std::max(1u, static_cast<u32>(height * pass.output_scale));
    }

    pass.output_framebuffer.reset();
    pass.output_texture.reset();
    pass.output_texture = g_renderer->CreateTexture(TextureConfig(
        output_width, output_height, 1, layers, 1, format, AbstractTextureFlag_RenderTarget));
    if (!pass.output_texture)
      return false;
    pass.output_framebuffer = g_renderer->CreateFramebuffer(pass.output_texture.get(), nullptr);
    if (!pass.output_framebuffer)
      return false;
  }

  LinkPassOutputs();

  if (recompile_pipelines && !CompilePipelines())
    return false;

  return true;
}

void Instance::LinkPassOutputs()
{
  m_last_pass_uses_color_buffer = false;
  for (RenderPass& render_pass : m_render_passes)
  {
    m_last_pass_uses_color_buffer = false;
    m_last_pass_scaled = render_pass.output_scale != 1.0f;
    for (InputBinding& binding : render_pass.inputs)
    {
      switch (binding.type)
      {
      case InputType::ColorBuffer:
        m_last_pass_uses_color_buffer = true;
        break;

      case InputType::PassOutput:
        binding.texture_ptr = m_render_passes.at(binding.source_pass_index).output_texture.get();
        break;

      default:
        break;
      }
    }
  }
}

bool Instance::IsValid() const
{
  return m_config->IsValid();
}

void Instance::SetDepthNearFar(float depth_near, float depth_far)
{
  m_depth_near = depth_near;
  m_depth_far = depth_far;
}

bool Instance::ShadersCompiled()
{
  if (m_last_change_count == m_config->GetChangeCount())
    return true;

  m_last_change_count = m_config->GetChangeCount();
  return CompilePasses();
}

bool Instance::Apply(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                     const AbstractTexture* source_color_tex,
                     const AbstractTexture* source_depth_tex,
                     const MathUtil::Rectangle<int>& source_rect, int source_layer)
{
  if (!ShadersCompiled())
    return false;

  const u32 dest_rect_width = static_cast<u32>(dest_rect.GetWidth());
  const u32 dest_rect_height = static_cast<u32>(dest_rect.GetHeight());
  if (m_target_width != dest_rect_width || m_target_height != dest_rect_height ||
      m_target_layers != dest_fb->GetLayers() || m_target_format != dest_fb->GetColorFormat())
  {
    if (!CreateOutputTextures(dest_rect_width, dest_rect_height, dest_fb->GetLayers(),
                              dest_fb->GetColorFormat()))
    {
      return false;
    }
  }

  // Determine whether we can skip the final copy by writing directly to the output texture, if the
  // last pass is not scaled, and the target isn't multisampled.
  const bool skip_final_copy = !m_last_pass_scaled && !m_last_pass_uses_color_buffer &&
                               dest_fb->GetColorAttachment() != source_color_tex &&
                               dest_fb->GetSamples() == 1;

  // Draw each pass.
  float last_pass_output_scale = 1.0f;
  const AbstractTexture* last_pass_tex = source_color_tex;
  MathUtil::Rectangle<int> last_pass_rect = source_rect;
  u32 last_shader_index = m_config->GetShaderCount();
  bool uniforms_changed = true;
  for (size_t pass_index = 0; pass_index < m_render_passes.size(); pass_index++)
  {
    const RenderPass& pass = m_render_passes[pass_index];

    // If this is the last pass and we can skip the final copy, write directly to output texture.
    AbstractFramebuffer* output_fb;
    MathUtil::Rectangle<int> output_rect;
    if (pass_index == m_render_passes.size() - 1 && skip_final_copy)
    {
      output_fb = dest_fb;
      output_rect = dest_rect;
      g_renderer->SetFramebuffer(dest_fb);
    }
    else
    {
      output_fb = pass.output_framebuffer.get();
      output_rect = pass.output_framebuffer->GetRect();
      g_renderer->SetAndDiscardFramebuffer(output_fb);
    }

    g_renderer->SetPipeline(pass.pipeline.get());
    g_renderer->SetViewportAndScissor(
        g_renderer->ConvertFramebufferRectangle(output_rect, output_fb));

    for (const InputBinding& input : pass.inputs)
    {
      switch (input.type)
      {
      case InputType::ColorBuffer:
        g_renderer->SetTexture(input.texture_unit, source_color_tex);
        break;

      case InputType::DepthBuffer:
        g_renderer->SetTexture(input.texture_unit, source_depth_tex);
        break;

      case InputType::PreviousPassOutput:
        g_renderer->SetTexture(input.texture_unit, last_pass_tex);
        break;

      case InputType::ExternalImage:
      case InputType::PassOutput:
        g_renderer->SetTexture(input.texture_unit, input.texture_ptr);
        break;
      }

      g_renderer->SetSamplerState(input.texture_unit, input.sampler_state);
    }

    // Skip uniform update where possible to save bandwidth.
    uniforms_changed |= pass.shader_index != last_shader_index;
    if (uniforms_changed)
    {
      UploadUniformBuffer(m_config->GetShader(pass.shader_index), last_pass_tex, last_pass_rect,
                          source_color_tex, source_rect, source_layer, last_pass_output_scale);
    }

    g_renderer->Draw(0, 3);

    if (output_fb != dest_fb)
      output_fb->GetColorAttachment()->FinishedRendering();

    last_shader_index = pass.shader_index;
    last_pass_tex = pass.output_texture.get();
    last_pass_rect = pass.output_texture->GetRect();
    last_pass_output_scale = pass.output_scale;
  }

  // Copy the last pass output to the target if not done already
  if (!skip_final_copy)
    g_renderer->ScaleTexture(dest_fb, dest_rect, last_pass_tex, last_pass_rect);

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Uniform buffer management
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr u32 MAX_UBO_PACK_OFFSET = 4;

static const char* GetOptionShaderType(OptionType type)
{
  switch (type)
  {
  case OptionType::Bool:
    return "bool";

  case OptionType::Float:
    return "float";

  case OptionType::Int:
    return "int";

  case OptionType::Float2:
    return "float2";

  case OptionType::Int2:
    return "int2";

  case OptionType::Float3:
    return "float3";

  case OptionType::Int3:
    return "int3";

  case OptionType::Float4:
    return "float4";

  case OptionType::Int4:
    return "int4";

  case OptionType::Float16:
    return "float4[4]";

  case OptionType::RGB:
    return "float3";

  case OptionType::RGBA:
    return "float4";
  }

  return "";
}

struct BuiltinUniforms
{
  float prev_resolution[4];
  float prev_rect[4];
  float src_resolution[4];
  float window_resolution[4];
  float src_rect[4];
  s32 u_time;
  u32 u_layer;
  float z_depth_near;
  float z_depth_far;
  float output_scale;

  static constexpr u32 total_components = 25;
  static constexpr u32 max_components = 4;
};

size_t Instance::CalculateUniformsSize(const Shader& shader) const
{
  return 0;
}

void Instance::UploadUniformBuffer(const Shader& shader, const AbstractTexture* prev_texture,
                                   const MathUtil::Rectangle<int>& prev_rect,
                                   const AbstractTexture* source_color_texture,
                                   const MathUtil::Rectangle<int>& source_rect, int source_layer,
                                   float last_pass_output_scale)
{
  BuiltinUniforms builtin_uniforms;

  // prev_resolution
  const float prev_width_f = static_cast<float>(prev_texture->GetWidth());
  const float prev_height_f = static_cast<float>(prev_texture->GetHeight());
  const float rcp_prev_width = 1.0f / prev_width_f;
  const float rcp_prev_height = 1.0f / prev_height_f;
  builtin_uniforms.prev_resolution[0] = prev_width_f;
  builtin_uniforms.prev_resolution[1] = prev_height_f;
  builtin_uniforms.prev_resolution[2] = rcp_prev_width;
  builtin_uniforms.prev_resolution[3] = rcp_prev_height;

  // prev_rect
  builtin_uniforms.prev_rect[0] = static_cast<float>(source_rect.left) * rcp_prev_width;
  builtin_uniforms.prev_rect[1] = static_cast<float>(source_rect.top) * rcp_prev_height;
  builtin_uniforms.prev_rect[2] = static_cast<float>(source_rect.GetWidth()) * rcp_prev_width;
  builtin_uniforms.prev_rect[3] = static_cast<float>(source_rect.GetHeight()) * rcp_prev_height;

  // src_resolution
  const float src_width_f = static_cast<float>(source_color_texture->GetWidth());
  const float src_height_f = static_cast<float>(source_color_texture->GetHeight());
  const float rcp_src_width = 1.0f / src_width_f;
  const float rcp_src_height = 1.0f / src_height_f;
  builtin_uniforms.src_resolution[0] = src_width_f;
  builtin_uniforms.src_resolution[1] = src_height_f;
  builtin_uniforms.src_resolution[2] = rcp_src_width;
  builtin_uniforms.src_resolution[3] = rcp_src_height;

  // window_resolution
  const auto& window_rect = g_renderer->GetTargetRectangle();
  builtin_uniforms.window_resolution[0] = static_cast<float>(window_rect.GetWidth());
  builtin_uniforms.window_resolution[1] = static_cast<float>(window_rect.GetHeight());
  builtin_uniforms.window_resolution[2] = 0.0f;
  builtin_uniforms.window_resolution[3] = 0.0f;

  // src_rect
  builtin_uniforms.src_rect[0] = static_cast<float>(source_rect.left) * rcp_src_width;
  builtin_uniforms.src_rect[1] = static_cast<float>(source_rect.top) * rcp_src_height;
  builtin_uniforms.src_rect[2] = static_cast<float>(source_rect.GetWidth()) * rcp_src_width;
  builtin_uniforms.src_rect[3] = static_cast<float>(source_rect.GetHeight()) * rcp_src_height;

  builtin_uniforms.u_time = static_cast<s32>(m_timer.GetTimeElapsed());
  builtin_uniforms.u_layer = static_cast<u32>(source_layer);
  builtin_uniforms.z_depth_near = m_depth_near;
  builtin_uniforms.z_depth_far = m_depth_far;
  builtin_uniforms.output_scale = last_pass_output_scale;

  u8* buf = m_uniform_staging_buffer.data();
  std::memcpy(buf, &builtin_uniforms, sizeof(builtin_uniforms));
  buf += sizeof(builtin_uniforms);

  u32 max_components = BuiltinUniforms::max_components;
  u32 total_components = BuiltinUniforms::total_components;
  for (const auto& it : shader.GetOptions())
  {
    const Option& option = it.second;
    if (option.compile_time_constant)
      continue;

    const u32 components = Option::GetComponents(option.type);

    const u32 remainder = total_components % components;

    u32 padding = 0;
    if (remainder != 0)
      padding = max_components - remainder;

    total_components += padding + components;

    buf += padding * sizeof(u32);

    std::memcpy(buf, &option.value, sizeof(u32) * components);

    padding = components;
    if (components == 3)
    {
      padding = components + 1;
      total_components += 1;
    }

    buf += padding * sizeof(u32);

    max_components = std::max(max_components, components);
  }

  u32 remainder = total_components % max_components;

  u32 padding = 0;
  if (remainder != 0)
    padding = max_components - remainder;

  buf += padding * sizeof(u32);

  g_vertex_manager->UploadUtilityUniforms(m_uniform_staging_buffer.data(),
                                          static_cast<u32>(buf - m_uniform_staging_buffer.data()));
}

std::string Instance::GetUniformBufferHeader(const Shader& shader) const
{
  std::stringstream ss;
  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
    ss << "cbuffer PSBlock : register(b0) {\n";
  else
    ss << "UBO_BINDING(std140, 1) uniform PSBlock {\n";

  // Builtin uniforms
  ss << "  float4 prev_resolution;\n";
  ss << "  float4 prev_rect;\n";
  ss << "  float4 src_resolution;\n";
  ss << "  float4 window_resolution;\n";
  ss << "  float4 src_rect;\n";
  ss << "  uint u_time;\n";
  ss << "  int u_layer;\n";
  ss << "  float z_depth_near;\n";
  ss << "  float z_depth_far;\n";
  ss << "  float output_scale;\n";
  ss << "\n";

  u32 max_components = BuiltinUniforms::max_components;
  u32 total_components = BuiltinUniforms::total_components;
  u32 align_counter = 0;

  // Custom options/uniforms - pack with std140 layout.
  for (const auto& it : shader.GetOptions())
  {
    const Option& option = it.second;
    if (option.compile_time_constant)
      continue;

    const u32 components = Option::GetComponents(option.type);

    const u32 remainder = total_components % components;

    u32 padding = 0;
    if (remainder != 0)
      padding = max_components - remainder;

    total_components += padding + components;

    for (u32 i = 0; i < padding; i++)
    {
      ss << "  "
         << "float _align_counter_" << align_counter++ << ";\n ";
    }

    ss << "  " << GetOptionShaderType(option.type) << " " << option.option_name << ";\n";

    if (components == 3)
    {
      ss << "  "
         << "float _align_counter_" << align_counter++ << ";\n ";

      total_components += 1;
    }

    max_components = std::max(max_components, components);
  }

  u32 remainder = total_components % max_components;

  u32 padding = 0;
  if (remainder != 0)
    padding = max_components - remainder;

  for (u32 i = 0; i < padding; i++)
  {
    ss << "  "
       << "float _align_counter_" << align_counter++ << ";\n ";
  }

  ss << "};\n\n";
  return ss.str();
}

std::string Instance::GetPixelShaderHeader(const Shader& shader, const Pass& pass) const
{
  // NOTE: uv0 contains the texture coordinates in previous pass space.
  // uv1 contains the texture coordinates in color/depth buffer space.

  std::stringstream ss;
  ss << GetUniformBufferHeader(shader);

  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
  {
    // Rename main, since we need to set up globals
    ss << R"(
#define HLSL 1
#define main real_main
#define gl_FragCoord v_fragcoord

Texture2DArray samp[4] : register(t0);
SamplerState samp_ss[4] : register(s0);

static float3 v_tex0;
static float3 v_tex1;
static float4 v_fragcoord;
static float4 ocol0;

// Type aliases.
#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4
#define mat4x3 float4x3

// function aliases. (see https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/)
#define dFdx ddx
#define dFdxCoarse ddx_coarse
#define dFdxFine ddx_fine
#define dFdy ddy
#define dFdyCoarse ddy_coarse
#define dFdyFine ddy_fine
#define interpolateAtCentroid EvaluateAttributeAtCentroid
#define interpolateAtSample EvaluateAttributeAtSample
#define interpolateAtOffset EvaluateAttributeSnapped
#define fract frac
#define mix lerp
#define fma mad

// Wrappers for sampling functions.
float4 SampleInput(int value) { return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(u_layer))); }
float4 SampleInputLocation(int value, float2 location) { return samp[value].Sample(samp_ss[value], float3(location, float(u_layer))); }
float4 SampleInputLayer(int value, int layer) { return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(layer))); }
float4 SampleInputLocationLayer(int value, float2 location, int layer) { return samp[value].Sample(samp_ss[value], float3(location, float(layer))); }

float4 SampleInputLod(int value, float lod) { return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(u_layer)), lod); }
float4 SampleInputLodLocation(int value, float lod, float2 location) { return samp[value].SampleLevel(samp_ss[value], float3(location, float(u_layer)), lod); }
float4 SampleInputLodLayer(int value, float lod, int layer) { return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(layer)), lod); }
float4 SampleInputLodLocationLayer(int value, float lod, float2 location, int layer) { return samp[value].SampleLevel(samp_ss[value], float3(location, float(layer)), lod); }

float4 SampleInputOffset(int value, int2 offset) { return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(u_layer)), offset); }
float4 SampleInputLocationOffset(int value, float2 location, int2 offset) { return samp[value].Sample(samp_ss[value], float3(location.xy, float(u_layer)), offset); }
float4 SampleInputLayerOffset(int value, int layer, int2 offset) { return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(layer)), offset); }
float4 SampleInputLocationLayerOffset(int value, float2 location, int layer, int2 offset) { return samp[value].Sample(samp_ss[value], float3(location.xy, float(layer)), offset); }

float4 SampleInputLodOffset(int value, float lod, int2 offset) { return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(u_layer)), lod, offset); }
float4 SampleInputLodLocationOffset(int value, float lod, float2 location, int2 offset) { return samp[value].SampleLevel(samp_ss[value], float3(location.xy, float(u_layer)), lod, offset); }
float4 SampleInputLodLayerOffset(int value, float lod, int layer, int2 offset) { return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(layer)), lod, offset); }
float4 SampleInputLodLocationLayerOffset(int value, float lod, float2 location, int layer, int2 offset) { return samp[value].SampleLevel(samp_ss[value], float3(location.xy, float(layer)), lod, offset); }

int2 SampleInputSize(int value, int lod)
{
  uint width;
  uint height;
  uint elements;
  uint miplevels;
  samp[value].GetDimensions(lod, width, height, elements, miplevels);
  return int2(width, height);
}

)";
  }
  else
  {
    ss << R"(
#define GLSL 1

// Type aliases.
#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4
#define float4x3 mat4x3

// Utility functions.
float saturate(float x) { return clamp(x, 0.0f, 1.0f); }
float2 saturate(float2 x) { return clamp(x, float2(0.0f, 0.0f), float2(1.0f, 1.0f)); }
float3 saturate(float3 x) { return clamp(x, float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f)); }
float4 saturate(float4 x) { return clamp(x, float4(0.0f, 0.0f, 0.0f, 0.0f), float4(1.0f, 1.0f, 1.0f, 1.0f)); }

// Flipped multiplication order because GLSL matrices use column vectors.
float2 mul(float2x2 m, float2 v) { return (v * m); }
float3 mul(float3x3 m, float3 v) { return (v * m); }
float4 mul(float4x3 m, float3 v) { return (v * m); }
float4 mul(float4x4 m, float4 v) { return (v * m); }
float2 mul(float2 v, float2x2 m) { return (m * v); }
float3 mul(float3 v, float3x3 m) { return (m * v); }
float3 mul(float4 v, float4x3 m) { return (m * v); }
float4 mul(float4 v, float4x4 m) { return (m * v); }

float4x3 mul(float4x3 m, float3x3 m2) { return (m2 * m); }

SAMPLER_BINDING(0) uniform sampler2DArray samp[4];
VARYING_LOCATION(0) in float3 v_tex0;
VARYING_LOCATION(1) in float3 v_tex1;
FRAGMENT_OUTPUT_LOCATION(0) out float4 ocol0;

// Wrappers for sampling functions.
float4 SampleInput(int value) { return texture(samp[value], float3(v_tex0.xy, float(u_layer))); }
float4 SampleInputLocation(int value, float2 location) { return texture(samp[value], float3(location, float(u_layer))); }
float4 SampleInputLayer(int value, int layer) { return texture(samp[value], float3(v_tex0.xy, float(layer))); }
float4 SampleInputLocationLayer(int value, float2 location, int layer) { return texture(samp[value], float3(location, float(layer))); }

float4 SampleInputLod(int value, float lod) { return textureLod(samp[value], float3(v_tex0.xy, float(u_layer)), lod); }
float4 SampleInputLodLocation(int value, float lod, float2 location) { return textureLod(samp[value], float3(location, float(u_layer)), lod); }
float4 SampleInputLodLayer(int value, float lod, int layer) { return textureLod(samp[value], float3(v_tex0.xy, float(layer)), lod); }
float4 SampleInputLodLocationLayer(int value, float lod, float2 location, int layer) { return textureLod(samp[value], float3(location, float(layer)), lod); }

// In Vulkan, offset can only be a compile time constant
// so instead of using a function, we use a macro
#define SampleInputOffset(value,  offset) (textureOffset(samp[value], float3(v_tex0.xy, float(u_layer)), offset))
#define SampleInputLocationOffset(value, location, offset) (textureOffset(samp[value], float3(location.xy, float(u_layer)), offset))
#define SampleInputLayerOffset(value, layer, offset) (textureOffset(samp[value], float3(v_tex0.xy, float(layer)), offset))
#define SampleInputLocationLayerOffset(value, location, layer, offset) (textureOffset(samp[value], float3(location.xy, float(layer)), offset))

#define SampleInputLodOffset(value, lod, offset) (textureLodOffset(samp[value], float3(v_tex0.xy, float(u_layer)), lod, offset))
#define SampleInputLodLocationOffset(value, lod, location, offset) (textureLodOffset(samp[value], float3(location.xy, float(u_layer)), lod, offset))
#define SampleInputLodLayerOffset(value, lod, layer, offset) (textureLodOffset(samp[value], float3(v_tex0.xy, float(layer)), lod, offset))
#define SampleInputLodLocationLayerOffset(value, lod, location, layer, offset) (textureLodOffset(samp[value], float3(location.xy, float(layer)), lod, offset))

ivec3 SampleInputSize(int value, int lod) { return textureSize(samp[value], lod); }
)";
  }

  if (g_ActiveConfig.backend_info.bSupportsReversedDepthRange)
    ss << "#define DEPTH_VALUE(val) (val)\n";
  else
    ss << "#define DEPTH_VALUE(val) (1.0 - (val))\n";

  // Figure out which input indices map to color/depth/previous buffers.
  // If any of these buffers is not bound, defaults of zero are fine here,
  // since that buffer may not even be used by the shader.
  int color_buffer_index = 0;
  int depth_buffer_index = 0;
  int prev_output_index = 0;
  for (const Input& input : pass.inputs)
  {
    switch (input.type)
    {
    case InputType::ColorBuffer:
      color_buffer_index = input.texture_unit;
      break;

    case InputType::DepthBuffer:
      depth_buffer_index = input.texture_unit;
      break;

    case InputType::PreviousPassOutput:
      prev_output_index = input.texture_unit;
      break;

    default:
      break;
    }
  }

  // Hook the discovered indices up to macros.
  // This is to support the SampleDepth, SamplePrev, etc. macros.
  ss << "#define COLOR_BUFFER_INPUT_INDEX " << color_buffer_index << "\n";
  ss << "#define DEPTH_BUFFER_INPUT_INDEX " << depth_buffer_index << "\n";
  ss << "#define PREV_OUTPUT_INPUT_INDEX " << prev_output_index << "\n";

  // Add compile-time constants
  for (const auto& it : shader.GetOptions())
  {
    const Option& option = it.second;
    if (!option.compile_time_constant)
      continue;

    const u32 components = Option::GetComponents(option.type);
    if (option.type == OptionType::Bool)
    {
      ss << "#define " << option.option_name << " (";
      ss << static_cast<u32>(option.value.bool_value);
    }
    else
    {
      ss << "#define " << option.option_name << " " << GetOptionShaderType(option.type) << "(";
      for (u32 i = 0; i < components; i++)
      {
        if (i > 0)
          ss << ", ";
        switch (option.type)
        {
        case OptionType::Float:
        case OptionType::Float2:
        case OptionType::Float3:
        case OptionType::Float4:
        case OptionType::RGB:
        case OptionType::RGBA:
          ss << option.value.float_values[i];
          break;
        case OptionType::Int:
        case OptionType::Int2:
        case OptionType::Int3:
        case OptionType::Int4:
          ss << option.value.int_values[i];
          break;
        }
      }
    }
    ss << ")\n";
  }

  ss << R"(

// Convert z/w -> linear depth
// https://gist.github.com/kovrov/a26227aeadde77b78092b8a962bd1a91
// http://dougrogers.blogspot.com/2013/02/how-to-derive-near-and-far-clip-plane.html
float ToLinearDepth(float depth)
{
  // TODO: The depth values provided by the projection matrix
  // produce invalid results, need to determine what the correct
  // values are
  //float NearZ = z_depth_near;
  //float FarZ = z_depth_far;

  // For now just hardcode our near/far planes
  float NearZ = 0.001f;
	float FarZ = 1.0f;
  const float A = (1.0f - (FarZ / NearZ)) / 2.0f;
  const float B = (1.0f + (FarZ / NearZ)) / 2.0f;
  return 1.0f / (A * depth + B);
}

// For backwards compatibility.
float4 Sample() { return SampleInput(PREV_OUTPUT_INPUT_INDEX); }
float4 SampleLocation(float2 location) { return SampleInputLocation(PREV_OUTPUT_INPUT_INDEX, location); }
float4 SampleLayer(int layer) { return SampleInputLayer(PREV_OUTPUT_INPUT_INDEX, layer); }
float4 SamplePrev() { return SampleInput(PREV_OUTPUT_INPUT_INDEX); }
float4 SamplePrevLocation(float2 location) { return SampleInputLocation(PREV_OUTPUT_INPUT_INDEX, location); }
float SampleRawDepth() { return DEPTH_VALUE(SampleInput(DEPTH_BUFFER_INPUT_INDEX).x); }
float SampleRawDepthLocation(float2 location) { return DEPTH_VALUE(SampleInputLocation(DEPTH_BUFFER_INPUT_INDEX, location).x); }
float SampleDepth() { return ToLinearDepth(SampleRawDepth()); }
float SampleDepthLocation(float2 location) { return ToLinearDepth(SampleRawDepthLocation(location)); }
#define SampleOffset(offset) (SampleInputOffset(COLOR_BUFFER_INPUT_INDEX, offset))
#define SampleLayerOffset(offset, layer) (SampleInputLayerOffset(COLOR_BUFFER_INPUT_INDEX, layer, offset))
#define SamplePrevOffset(offset) (SampleInputOffset(PREV_OUTPUT_INPUT_INDEX, offset))
#define SampleRawDepthOffset(offset) (DEPTH_VALUE(SampleInputOffset(DEPTH_BUFFER_INPUT_INDEX, offset).x))
#define SampleDepthOffset(offset) (ToLinearDepth(SampleRawDepthOffset(offset)))

float2 GetResolution() { return prev_resolution.xy; }
float2 GetInvResolution() { return prev_resolution.zw; }
float2 GetWindowResolution() {  return window_resolution.xy; }
float2 GetCoordinates() { return v_tex0.xy; }

float2 GetPrevResolution() { return prev_resolution.xy; }
float2 GetInvPrevResolution() { return prev_resolution.zw; }
float2 GetPrevRectOrigin() { return prev_rect.xy; }
float2 GetPrevRectSize() { return prev_rect.zw; }
float2 GetPrevCoordinates() { return v_tex0.xy; }

float2 GetSrcResolution() { return src_resolution.xy; }
float2 GetInvSrcResolution() { return src_resolution.zw; }
float2 GetSrcRectOrigin() { return src_rect.xy; }
float2 GetSrcRectSize() { return src_rect.zw; }
float2 GetSrcCoordinates() { return v_tex1.xy; }

float4 GetFragmentCoord() { return gl_FragCoord; }

float GetLayer() { return u_layer; }
float GetTime() { return u_time; }

void SetOutput(float4 color) { ocol0 = color; }

#define GetOption(x) (x)
#define OptionEnabled(x) (x)

)";
  return ss.str();
}

std::string Instance::GetPixelShaderFooter(const Shader& shader, const Pass& pass) const
{
  std::stringstream ss;
  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
  {
    ss << R"(

#undef main
void main(in float3 v_tex0_ : TEXCOORD0,
          in float3 v_tex1_ : TEXCOORD1,
          in float4 pos : SV_Position,
          out float4 ocol0_ : SV_Target)
{
  v_tex0 = v_tex0_;
  v_tex1 = v_tex1_;
  v_fragcoord = pos;
)";
    if (pass.entry_point == "main")
    {
      ss << "  real_main();\n";
    }
    else if (pass.entry_point.empty())
    {
      // No entry point should create a copy
      ss << "  ocol0 = SampleInput(0);\n";
    }
    else
    {
      ss << "  " << pass.entry_point << "();\n";
    }
    ss << R"(
  ocol0_ = ocol0;
})";
  }
  else
  {
    if (pass.entry_point.empty())
    {
      // No entry point should create a copy
      ss << "void main() { ocol0 = SampleInput(0);\n }\n";
    }
    else if (pass.entry_point != "main")
    {
      ss << "void main() { " << pass.entry_point << "();\n }\n";
    }
  }

  return ss.str();
}

bool Instance::CompilePipelines()
{
  AbstractPipelineConfig config = {};
  config.geometry_shader = m_target_layers > 1 ? m_geometry_shader.get() : nullptr;
  config.rasterization_state = RenderState::GetNoCullRasterizationState(PrimitiveType::Triangles);
  config.depth_state = RenderState::GetNoDepthTestingDepthState();
  config.blending_state = RenderState::GetNoBlendingBlendState();
  config.framebuffer_state = RenderState::GetColorFramebufferState(m_target_format);
  config.usage = AbstractPipelineUsage::Utility;

  size_t uniforms_size = 0;
  for (RenderPass& render_pass : m_render_passes)
  {
    config.vertex_shader = render_pass.vertex_shader.get();
    config.pixel_shader = render_pass.pixel_shader.get();
    render_pass.pipeline = g_renderer->CreatePipeline(config);
    if (!render_pass.pipeline)
    {
      PanicAlertFmt("Failed to compile post-processing pipeline");
      return false;
    }

    u32 max_components = BuiltinUniforms::max_components;
    u32 total_components = BuiltinUniforms::total_components;

    auto& shader = m_config->GetShader(render_pass.shader_index);

    // Custom options/uniforms - pack with std140 layout.
    for (const auto& it : shader.GetOptions())
    {
      const Option& option = it.second;
      if (option.compile_time_constant)
        continue;

      const u32 components = Option::GetComponents(option.type);

      const u32 remainder = total_components % components;

      u32 padding = 0;
      if (remainder != 0)
        padding = max_components - remainder;

      total_components += padding + components;

      if (components == 3)
      {
        total_components += 1;
      }

      max_components = std::max(max_components, components);
    }

    u32 remainder = total_components % max_components;

    u32 padding = 0;
    if (remainder != 0)
      padding = max_components - remainder;
    uniforms_size = std::max(uniforms_size, (total_components + padding) * sizeof(u32));
  }

  m_uniform_staging_buffer.resize(uniforms_size);
  return true;
}

std::shared_ptr<AbstractShader> Instance::CompileVertexShader(const Shader& shader) const
{
  std::stringstream ss;
  ss << GetUniformBufferHeader(shader);

  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
  {
    ss << "void main(in uint id : SV_VertexID, out float3 v_tex0 : TEXCOORD0,\n";
    ss << "          out float3 v_tex1 : TEXCOORD1, out float4 opos : SV_Position) {\n";
  }
  else
  {
    if (g_ActiveConfig.backend_info.bSupportsGeometryShaders)
    {
      ss << "VARYING_LOCATION(0) out VertexData {\n";
      ss << "  float3 v_tex0;\n";
      ss << "  float3 v_tex1;\n";
      ss << "};\n";
    }
    else
    {
      ss << "VARYING_LOCATION(0) out float3 v_tex0;\n";
      ss << "VARYING_LOCATION(0) out float3 v_tex1;\n";
    }

    ss << "#define id gl_VertexID\n";
    ss << "#define opos gl_Position\n";
    ss << "void main() {\n";
  }
  ss << "  v_tex0 = float3(float((id << 1) & 2), float(id & 2), 0.0f);\n";
  ss << "  opos = float4(v_tex0.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n";
  ss << "  v_tex1 = float3(src_rect.xy + (src_rect.zw * v_tex0.xy), 0.0f);\n";

  if (g_ActiveConfig.backend_info.api_type == APIType::Vulkan)
    ss << "  opos.y = -opos.y;\n";

  ss << "}\n";

  std::unique_ptr<AbstractShader> vs =
      g_renderer->CreateShaderFromSource(ShaderStage::Vertex, ss.str());
  if (!vs)
  {
    shader.GetRuntimeInfo()->SetError(true);
    PanicAlertFmt("Failed to compile post-processing vertex shader");
    return nullptr;
  }
  shader.GetRuntimeInfo()->SetError(false);

  // convert from unique_ptr to shared_ptr, as many passes share one VS.
  return std::shared_ptr<AbstractShader>(vs.release());
}

std::unique_ptr<AbstractShader> Instance::CompileGeometryShader() const
{
  std::string source = FramebufferShaderGen::GeneratePassthroughGeometryShader(2, 0);
  auto gs = g_renderer->CreateShaderFromSource(ShaderStage::Geometry, source);
  if (!gs)
  {
    PanicAlertFmt("Failed to compile post-processing geometry shader");
    return nullptr;
  }

  return gs;
}

std::unique_ptr<AbstractShader> Instance::CompilePixelShader(const Shader& shader,
                                                             const Pass& pass) const
{
  // Generate GLSL and compile the new shader.
  std::stringstream ss;
  ss << GetPixelShaderHeader(shader, pass);
  ss << shader.GetShaderSource();
  ss << GetPixelShaderFooter(shader, pass);

  auto ps = g_renderer->CreateShaderFromSource(ShaderStage::Pixel, ss.str());
  if (!ps)
  {
    shader.GetRuntimeInfo()->SetError(true);
    PanicAlertFmt("Failed to compile post-processing pixel shader");
    return nullptr;
  }
  shader.GetRuntimeInfo()->SetError(false);

  return ps;
}

const Instance::RenderPass* Instance::GetRenderPass(u32 shader_index, u32 pass_index) const
{
  for (size_t i = 0; i < m_render_passes.size(); i++)
  {
    const RenderPass& rp = m_render_passes[i];
    if (rp.shader_index == shader_index && rp.shader_pass_index == pass_index)
      return &rp;
  }

  return nullptr;
}

u32 Instance::GetRenderPassIndex(u32 shader_index, u32 pass_index) const
{
  for (size_t i = 0; i < m_render_passes.size(); i++)
  {
    const RenderPass& rp = m_render_passes[i];
    if (rp.shader_index == shader_index && rp.shader_pass_index == pass_index)
      return static_cast<u32>(i);
  }

  return static_cast<u32>(m_render_passes.size());
}

System::System()
    : m_efb_instance(&g_ActiveConfig.shader_config_efb),
      m_xfb_instance(&g_ActiveConfig.shader_config_xfb),
      m_post_instance(&g_ActiveConfig.shader_config_post_process),
      m_texture_instance(&g_ActiveConfig.shader_config_textures),
      m_downsample_instance(&g_ActiveConfig.shader_config_downsample)
{
}

bool System::ApplyEFB(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                      const AbstractTexture* source_color_tex,
                      const AbstractTexture* source_depth_tex,
                      const MathUtil::Rectangle<int>& source_rect, int source_layer)
{
  return m_efb_instance.Apply(dest_fb, dest_rect, source_color_tex, source_depth_tex, source_rect,
                              source_layer);
}

bool System::ApplyXFB(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                      const AbstractTexture* source_color_tex,
                      const AbstractTexture* source_depth_tex,
                      const MathUtil::Rectangle<int>& source_rect, int source_layer)
{
  return m_xfb_instance.Apply(dest_fb, dest_rect, source_color_tex, source_depth_tex, source_rect,
                              source_layer);
}

bool System::ApplyPost(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                       const AbstractTexture* source_color_tex,
                       const MathUtil::Rectangle<int>& source_rect, int source_layer)
{
  return m_post_instance.Apply(dest_fb, dest_rect, source_color_tex, nullptr, source_rect,
                               source_layer);
}

bool System::ApplyTextures(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                           const AbstractTexture* source_color_tex,
                           const MathUtil::Rectangle<int>& source_rect, int source_layer)
{
  return m_texture_instance.Apply(dest_fb, dest_rect, source_color_tex, nullptr, source_rect,
                                  source_layer);
}

bool System::ApplyDownsample(AbstractFramebuffer* dest_fb,
                             const MathUtil::Rectangle<int>& dest_rect,
                             const AbstractTexture* source_color_tex,
                             const MathUtil::Rectangle<int>& source_rect, int source_layer)
{
  return m_downsample_instance.Apply(dest_fb, dest_rect, source_color_tex, nullptr, source_rect,
                                     source_layer);
}

void System::OnProjectionChanged(ProjectionType type, const Common::Matrix44& projection_mtx)
{
  if (m_saw_2d_element && m_projection_state == Perspective)
  {
    m_projection_state = Final;
    ProcessEFB(std::nullopt);
  }
  if (type == ProjectionType::Perspective)
  {
    const float z_near = (projection_mtx.data[11] / (projection_mtx.data[10] - 1));
    const float z_far = (projection_mtx.data[11] / (projection_mtx.data[10] + 1));
    PRIM_LOG("ZNear: {}    ZFar:  {}", z_near, z_far);
    m_xfb_instance.SetDepthNearFar(z_near, z_far);
    m_efb_instance.SetDepthNearFar(z_near, z_far);

    // Only adjust the flag if this is our first perspective load.
    if (m_projection_state == Initial)
    {
      m_projection_state = Perspective;
    }
  }
  else if (type == ProjectionType::Orthographic)
  {
    if (m_projection_state == Perspective)
    {
      m_saw_2d_element = true;
    }
  }
}

void System::OnFrameEnd(std::optional<MathUtil::Rectangle<int>> xfb_rect)
{
  // If we didn't switch to orthographic after perspective, post-process now (e.g. if no HUD was
  // drawn)
  if (m_projection_state == Perspective)
    ProcessEFB(xfb_rect);

  m_saw_2d_element = false;
  m_efb_triggered = false;
  m_ignore_2d = true;
  m_draw_count = 0;
  m_projection_state = Initial;
}

void System::OnEFBWritten(const MathUtil::Rectangle<int>& efb_rect)
{
  // Fire off postprocessing on the current efb if a perspective scene has been drawn.
  if (m_projection_state == Perspective &&
      (g_renderer->EFBToScaledX(efb_rect.GetWidth()) > ((g_renderer->GetTargetWidth() * 2) / 3)))
  {
    m_projection_state = Final;
    ProcessEFB(efb_rect);
  }
}

void System::OnOpcodeDrawingStateChanged(bool is_drawing)
{
  /*if (!is_drawing && m_projection_state == Perspective)
  {
    if (m_saw_2d_element)
    {
      m_projection_state = Final;
      ProcessEFB(std::nullopt);
    }
  }*/
}

void System::OnEFBTriggered()
{
  m_saw_2d_element = false;
}

void System::ProcessEFB(std::optional<MathUtil::Rectangle<int>> efb_rect)
{
  if (!m_efb_instance.IsValid())
    return;

  if (efb_rect)
  {
    const auto scaled_src_rect = g_renderer->ConvertEFBRectangle(*efb_rect);
    const auto framebuffer_rect = g_renderer->ConvertFramebufferRectangle(
        scaled_src_rect, g_framebuffer_manager->GetEFBFramebuffer());
    ProcessEFBRect(framebuffer_rect);
  }
  else
  {
    int scissor_x_off = bpmem.scissorOffset.x * 2;
    int scissor_y_off = bpmem.scissorOffset.y * 2;
    float x = g_renderer->EFBToScaledXf(xfmem.viewport.xOrig - xfmem.viewport.wd - scissor_x_off);
    float y = g_renderer->EFBToScaledYf(xfmem.viewport.yOrig + xfmem.viewport.ht - scissor_y_off);

    float width = g_renderer->EFBToScaledXf(2.0f * xfmem.viewport.wd);
    float height = g_renderer->EFBToScaledYf(-2.0f * xfmem.viewport.ht);
    if (width < 0.f)
    {
      x += width;
      width *= -1;
    }
    if (height < 0.f)
    {
      y += height;
      height *= -1;
    }

    // Clamp to size if oversized not supported. Required for D3D.
    if (!g_ActiveConfig.backend_info.bSupportsOversizedViewports)
    {
      const float max_width = static_cast<float>(g_renderer->GetCurrentFramebuffer()->GetWidth());
      const float max_height = static_cast<float>(g_renderer->GetCurrentFramebuffer()->GetHeight());
      x = std::clamp(x, 0.0f, max_width - 1.0f);
      y = std::clamp(y, 0.0f, max_height - 1.0f);
      width = std::clamp(width, 1.0f, max_width - x);
      height = std::clamp(height, 1.0f, max_height - y);
    }

    // Lower-left flip.
    if (g_ActiveConfig.backend_info.bUsesLowerLeftOrigin)
      y = static_cast<float>(g_renderer->GetCurrentFramebuffer()->GetHeight()) - y - height;

    MathUtil::Rectangle<int> srcRect;
    srcRect.left = static_cast<int>(x);
    srcRect.top = static_cast<int>(y);
    srcRect.right = srcRect.left + static_cast<int>(width);
    srcRect.bottom = srcRect.top + static_cast<int>(height);

    const auto scaled_src_rect = g_renderer->ConvertEFBRectangle(srcRect);
    const auto framebuffer_rect = g_renderer->ConvertFramebufferRectangle(
        scaled_src_rect, g_framebuffer_manager->GetEFBFramebuffer());

    //ProcessEFBRect(framebuffer_rect);
    m_efb_instance.Apply(g_framebuffer_manager->GetEFBFramebuffer(),
                         g_framebuffer_manager->GetEFBFramebuffer()->GetRect(),
                         g_framebuffer_manager->GetEFBColorTexture(),
                         g_framebuffer_manager->GetEFBDepthTexture(),
                         g_framebuffer_manager->GetEFBFramebuffer()->GetRect(), 0);
  }
}

void System::ProcessEFBRect(const MathUtil::Rectangle<int>& framebuffer_rect)
{
  m_efb_instance.Apply(g_framebuffer_manager->GetEFBFramebuffer(),
                       g_framebuffer_manager->GetEFBFramebuffer()->GetRect(),
                       g_framebuffer_manager->ResolveEFBColorTexture(framebuffer_rect),
                       g_framebuffer_manager->ResolveEFBDepthTexture(framebuffer_rect),
                       framebuffer_rect, 0);
}

/*SystemConfig::SystemConfig()
    : m_efb_config("EFB"), m_xfb_config("XFB"), m_post_config("PostProcess"),
      m_scaling_config("Scaling")
{
}

bool SystemConfig::LoadFile(const std::string& config_file)
{
  const std::string filename = config_file.empty() ? GetDefaultFilename() : config_file;
  if (!m_efb_config.LoadFromFile(filename))
    return false;
  if (!m_xfb_config.LoadFromFile(filename))
    return false;
  if (!m_post_config.LoadFromFile(filename))
    return false;
  if (!m_scaling_config.LoadFromFile(filename))
    return false;

  return true;
}

void SystemConfig::SaveToFile(const std::string& config_file)
{
  const std::string filename = config_file.empty() ? GetDefaultFilename() : config_file;
  m_efb_config.SaveToFile(filename);
  m_xfb_config.SaveToFile(filename);
  m_post_config.SaveToFile(filename);
  m_scaling_config.SaveToFile(filename);
}

std::string SystemConfig::GetDefaultFilename() const
{
  return File::GetUserPath(D_CONFIG_IDX) + "DefaultPostProcessingPreset.ini";
}*/

}  // namespace VideoCommon::PostProcessing
