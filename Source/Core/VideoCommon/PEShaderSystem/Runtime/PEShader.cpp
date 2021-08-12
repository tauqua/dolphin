#include "VideoCommon/PEShaderSystem/Runtime/PEShader.h"

#include <algorithm>

#include "Common/FileUtil.h"
#include "Common/MsgHandler.h"
#include "VideoCommon/PEShaderSystem/Runtime/PEShaderOption.h"

#include "VideoCommon/AbstractFramebuffer.h"
#include "VideoCommon/AbstractPipeline.h"
#include "VideoCommon/AbstractShader.h"
#include "VideoCommon/AbstractTexture.h"
#include "VideoCommon/FramebufferShaderGen.h"
#include "VideoCommon/RenderBase.h"
#include "VideoCommon/ShaderGenCommon.h"
#include "VideoCommon/VideoCommon.h"
#include "VideoCommon/VideoConfig.h"

namespace VideoCommon::PE
{
bool Shader::UpdateConfig(const ShaderConfig& config)
{
  const bool update_static_options = config.m_changes != m_last_static_option_change_count;
  const bool update_compile_options =
      config.m_compiletime_changes != m_last_compile_option_change_count;
  if (!update_static_options && !update_compile_options)
    return true;

  m_last_static_option_change_count = config.m_changes;
  m_last_compile_option_change_count = config.m_compiletime_changes;

  if (update_compile_options)
  {
    if (!RecompilePasses(config))
    {
      return false;
    }
  }

  u32 i = 0;
  for (const auto& config_option : config.m_options)
  {
    auto&& option = m_options[i];
    option->Update(config_option);
    i++;
  }

  return true;
}

void Shader::CreateOptions(const ShaderConfig& config)
{
  m_last_static_option_change_count = config.m_changes;
  m_last_compile_option_change_count = config.m_compiletime_changes;
  m_options.clear();
  std::size_t buffer_size = m_builtin_uniforms.Size();
  for (const auto& config_option : config.m_options)
  {
    auto option = ShaderOption::Create(config_option);
    buffer_size += option->Size();
    m_options.push_back(std::move(option));
  }

  m_uniform_staging_buffer.resize(buffer_size);
}

bool Shader::CreatePasses(const ShaderConfig& config)
{
  if (g_ActiveConfig.backend_info.bSupportsGeometryShaders && !m_geometry_shader)
  {
    m_geometry_shader = CompileGeometryShader();
    if (!m_geometry_shader)
      return false;
  }

  std::shared_ptr<AbstractShader> vertex_shader = CompileVertexShader(config);
  if (!vertex_shader)
  {
    config.m_runtime_info->SetError(true);
    return false;
  }

  m_passes.clear();
  for (const auto& config_pass : config.m_passes)
  {
    // if (!config_pass.dependent_option.empty())
    //{
    // TODO:
    //}

    ShaderPass pass;

    pass.m_inputs.reserve(config_pass.m_inputs.size());
    for (const auto& config_input : config_pass.m_inputs)
    {
      auto input = ShaderInput::Create(config_input);
      if (!input)
        return false;

      pass.m_inputs.push_back(std::move(input));
    }

    pass.m_output_scale = config_pass.m_output_scale;
    pass.m_vertex_shader = vertex_shader;
    pass.m_pixel_shader = CompilePixelShader(config, pass, config_pass);
    if (!pass.m_pixel_shader)
    {
      config.m_runtime_info->SetError(true);
      return false;
    }

    m_passes.push_back(std::move(pass));
  }

  if (m_passes.empty())
    return false;

  return true;
}

bool Shader::CreateAllPassOutputTextures(u32 width, u32 height, u32 layers,
                                         AbstractTextureFormat format)
{
  for (ShaderPass& pass : m_passes)
  {
    u32 output_width;
    u32 output_height;
    if (pass.m_output_scale < 0.0f)
    {
      const float native_scale = -pass.m_output_scale;
      const int native_width = width * EFB_WIDTH / g_renderer->GetTargetWidth();
      const int native_height = height * EFB_HEIGHT / g_renderer->GetTargetHeight();
      output_width = std::max(
          static_cast<u32>(std::round((static_cast<float>(native_width) * native_scale))), 1u);
      output_height = std::max(
          static_cast<u32>(std::round((static_cast<float>(native_height) * native_scale))), 1u);
    }
    else
    {
      output_width = std::max(1u, static_cast<u32>(width * pass.m_output_scale));
      output_height = std::max(1u, static_cast<u32>(height * pass.m_output_scale));
    }

    pass.m_output_framebuffer.reset();
    pass.m_output_texture.reset();
    pass.m_output_texture = g_renderer->CreateTexture(TextureConfig(
        output_width, output_height, 1, layers, 1, format, AbstractTextureFlag_RenderTarget));
    if (!pass.m_output_texture)
    {
      PanicAlertFmt("Failed to create texture for PE shader pass");
      return false;
    }
    pass.m_output_framebuffer = g_renderer->CreateFramebuffer(pass.m_output_texture.get(), nullptr);
    if (!pass.m_output_framebuffer)
    {
      PanicAlertFmt("Failed to create framebuffer for PE shader pass");
      return false;
    }
  }

  return true;
}

void Shader::UpdatePassInputs(const AbstractTexture* previous_texture)
{
  const auto update_inputs = [this](const ShaderPass& pass,
                                    const AbstractTexture* previous_pass_texture) {
    for (auto&& input : pass.m_inputs)
    {
      input->Update(previous_pass_texture, m_passes);
    }
  };

  if (m_passes.size() == 1)
  {
    update_inputs(m_passes[0], previous_texture);
  }

  for (std::size_t i = 1; i < m_passes.size(); i++)
  {
    update_inputs(m_passes[i], m_passes[i - 1].m_output_texture.get());
  }
}

bool Shader::RebuildPipeline(AbstractTextureFormat format, u32 layers)
{
  AbstractPipelineConfig config = {};
  config.geometry_shader = layers > 1 ? m_geometry_shader.get() : nullptr;
  config.rasterization_state = RenderState::GetNoCullRasterizationState(PrimitiveType::Triangles);
  config.depth_state = RenderState::GetNoDepthTestingDepthState();
  config.blending_state = RenderState::GetNoBlendingBlendState();
  config.framebuffer_state = RenderState::GetColorFramebufferState(format);
  config.usage = AbstractPipelineUsage::Utility;

  for (auto& pass : m_passes)
  {
    config.vertex_shader = pass.m_vertex_shader.get();
    config.pixel_shader = pass.m_pixel_shader.get();
    pass.m_pipeline = g_renderer->CreatePipeline(config);
    if (!pass.m_pipeline)
    {
      PanicAlertFmt("Failed to compile pipeline for PE shader pass");
      return false;
    }
  }

  return true;
}

void Shader::Apply(bool skip_final_copy, const ShaderApplyOptions& options,
                   const AbstractTexture* prev_pass_texture, float prev_pass_output_scale)
{
  const auto parse_inputs = [&](const ShaderPass& pass) {
    for (auto&& input : pass.m_inputs)
    {
      switch (input->GetType())
      {
      case InputType::ColorBuffer:
        g_renderer->SetTexture(input->GetTextureUnit(), options.m_source_color_tex);
        break;

      case InputType::DepthBuffer:
        g_renderer->SetTexture(input->GetTextureUnit(), options.m_source_depth_tex);
        break;

      case InputType::PreviousPassOutput:
      case InputType::ExternalImage:
      case InputType::PassOutput:
        g_renderer->SetTexture(input->GetTextureUnit(), input->GetTexture());
        break;
      case InputType::PreviousShaderOutput:
        g_renderer->SetTexture(input->GetTextureUnit(), prev_pass_texture);
        break;
      }

      g_renderer->SetSamplerState(input->GetTextureUnit(), input->GetSamplerState());
    }
  };
  const AbstractTexture* last_pass_texture = prev_pass_texture;
  float last_pass_output_scale = prev_pass_output_scale;
  for (std::size_t i = 0; i < m_passes.size() - 1; i++)
  {
    const auto& pass = m_passes[i];
    const auto output_fb = pass.m_output_framebuffer.get();
    const auto output_rect = pass.m_output_framebuffer->GetRect();
    g_renderer->SetAndDiscardFramebuffer(output_fb);

    g_renderer->SetPipeline(pass.m_pipeline.get());
    g_renderer->SetViewportAndScissor(
        g_renderer->ConvertFramebufferRectangle(output_rect, output_fb));

    parse_inputs(pass);

    m_builtin_uniforms.Update(options, last_pass_texture, last_pass_output_scale);
    UploadUniformBuffer();

    g_renderer->Draw(0, 3);
    output_fb->GetColorAttachment()->FinishedRendering();

    last_pass_texture = pass.m_output_texture.get();
    last_pass_output_scale = pass.m_output_scale;
  }

  const auto& pass = m_passes[m_passes.size() - 1];
  if (skip_final_copy)
  {
    g_renderer->SetFramebuffer(options.m_dest_fb);
    g_renderer->SetPipeline(pass.m_pipeline.get());
    g_renderer->SetViewportAndScissor(
        g_renderer->ConvertFramebufferRectangle(options.m_dest_rect, options.m_dest_fb));

    parse_inputs(pass);

    m_builtin_uniforms.Update(options, last_pass_texture, last_pass_output_scale);
    UploadUniformBuffer();

    g_renderer->Draw(0, 3);
  }
  else
  {
    g_renderer->SetAndDiscardFramebuffer(pass.m_output_framebuffer.get());
    g_renderer->SetPipeline(pass.m_pipeline.get());
    g_renderer->SetViewportAndScissor(g_renderer->ConvertFramebufferRectangle(
        pass.m_output_framebuffer->GetRect(), pass.m_output_framebuffer.get()));

    parse_inputs(pass);

    m_builtin_uniforms.Update(options, last_pass_texture, last_pass_output_scale);
    UploadUniformBuffer();

    g_renderer->Draw(0, 3);
    pass.m_output_framebuffer->GetColorAttachment()->FinishedRendering();

    g_renderer->ScaleTexture(options.m_dest_fb, options.m_dest_rect, pass.m_output_texture.get(),
                             pass.m_output_texture->GetRect());
  }
}

const std::vector<ShaderPass>& Shader::GetPasses() const
{
  return m_passes;
}

bool Shader::RecompilePasses(const ShaderConfig& config)
{
  if (g_ActiveConfig.backend_info.bSupportsGeometryShaders && !m_geometry_shader)
  {
    m_geometry_shader = CompileGeometryShader();
    if (!m_geometry_shader)
      return false;
  }

  std::shared_ptr<AbstractShader> vertex_shader = CompileVertexShader(config);
  if (!vertex_shader)
  {
    config.m_runtime_info->SetError(true);
    return false;
  }

  for (std::size_t i = 0; i < m_passes.size(); i++)
  {
    auto& pass = m_passes[i];
    auto& config_pass = config.m_passes[i];

    pass.m_vertex_shader = vertex_shader;
    pass.m_pixel_shader = CompilePixelShader(config, pass, config_pass);
    if (!pass.m_pixel_shader)
    {
      config.m_runtime_info->SetError(true);
      return false;
    }
  }

  return true;
}

void Shader::UploadUniformBuffer()
{
  u8* buffer = m_uniform_staging_buffer.data();
  m_builtin_uniforms.WriteToMemory(buffer);
  for (const auto& option : m_options)
  {
    option->WriteToMemory(buffer);
  }
}

std::unique_ptr<AbstractShader> Shader::CompileGeometryShader() const
{
  std::string source = FramebufferShaderGen::GeneratePassthroughGeometryShader(2, 0);
  return g_renderer->CreateShaderFromSource(ShaderStage::Geometry, std::move(source));
}

void Shader::PrepareUniformHeader(ShaderCode& shader_source) const
{
  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
    shader_source.Write("cbuffer PSBlock : register(b0) {{\n");
  else
    shader_source.Write("UBO_BINDING(std140, 1) uniform PSBlock {{\n");

  m_builtin_uniforms.WriteShaderUniforms(shader_source);

  for (const auto& option : m_options)
  {
    option->WriteShaderUniforms(shader_source);
  }
  shader_source.Write("}}\n");
}

std::shared_ptr<AbstractShader> Shader::CompileVertexShader(const ShaderConfig& config) const
{
  ShaderCode shader_source;
  PrepareUniformHeader(shader_source);
  VertexShaderMain(shader_source);

  std::unique_ptr<AbstractShader> vs =
      g_renderer->CreateShaderFromSource(ShaderStage::Vertex, shader_source.GetBuffer());
  if (!vs)
  {
    return nullptr;
  }

  // convert from unique_ptr to shared_ptr, as many passes share one VS
  return std::shared_ptr<AbstractShader>(vs.release());
}

std::unique_ptr<AbstractShader>
Shader::CompilePixelShader(const ShaderConfig& config, const ShaderPass& pass,
                           const ShaderConfigPass& config_pass) const
{
  ShaderCode shader_source;
  PrepareUniformHeader(shader_source);
  PixelShaderHeader(shader_source, pass);

  std::string shader_body;
  File::ReadFileToString(config.m_shader_path, shader_body);
  shader_body = ReplaceAll(shader_body, "\r\n", "\n");
  shader_body = ReplaceAll(shader_body, "{", "{{");
  shader_body = ReplaceAll(shader_body, "}", "}}");

  shader_source.Write(shader_body);
  PixelShaderFooter(shader_source, config_pass);
  return g_renderer->CreateShaderFromSource(ShaderStage::Pixel, shader_source.GetBuffer());
}

void Shader::PixelShaderHeader(ShaderCode& shader_source, const ShaderPass& pass) const
{
  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
  {
    // Rename main, since we need to set up globals
    shader_source.Write(R"(
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
float4 SampleInput(int value) {{ return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(u_layer))); }}
float4 SampleInputLocation(int value, float2 location) {{ return samp[value].Sample(samp_ss[value], float3(location, float(u_layer))); }}
float4 SampleInputLayer(int value, int layer) {{ return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(layer))); }}
float4 SampleInputLocationLayer(int value, float2 location, int layer) {{ return samp[value].Sample(samp_ss[value], float3(location, float(layer))); }}

float4 SampleInputLod(int value, float lod) {{ return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(u_layer)), lod); }}
float4 SampleInputLodLocation(int value, float lod, float2 location) {{ return samp[value].SampleLevel(samp_ss[value], float3(location, float(u_layer)), lod); }}
float4 SampleInputLodLayer(int value, float lod, int layer) {{ return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(layer)), lod); }}
float4 SampleInputLodLocationLayer(int value, float lod, float2 location, int layer) {{ return samp[value].SampleLevel(samp_ss[value], float3(location, float(layer)), lod); }}

float4 SampleInputOffset(int value, int2 offset) {{ return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(u_layer)), offset); }}
float4 SampleInputLocationOffset(int value, float2 location, int2 offset) {{ return samp[value].Sample(samp_ss[value], float3(location.xy, float(u_layer)), offset); }}
float4 SampleInputLayerOffset(int value, int layer, int2 offset) {{ return samp[value].Sample(samp_ss[value], float3(v_tex0.xy, float(layer)), offset); }}
float4 SampleInputLocationLayerOffset(int value, float2 location, int layer, int2 offset) {{ return samp[value].Sample(samp_ss[value], float3(location.xy, float(layer)), offset); }}

float4 SampleInputLodOffset(int value, float lod, int2 offset) {{ return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(u_layer)), lod, offset); }}
float4 SampleInputLodLocationOffset(int value, float lod, float2 location, int2 offset) {{ return samp[value].SampleLevel(samp_ss[value], float3(location.xy, float(u_layer)), lod, offset); }}
float4 SampleInputLodLayerOffset(int value, float lod, int layer, int2 offset) {{ return samp[value].SampleLevel(samp_ss[value], float3(v_tex0.xy, float(layer)), lod, offset); }}
float4 SampleInputLodLocationLayerOffset(int value, float lod, float2 location, int layer, int2 offset) {{ return samp[value].SampleLevel(samp_ss[value], float3(location.xy, float(layer)), lod, offset); }}

int2 SampleInputSize(int value, int lod)
{{
  uint width;
  uint height;
  uint elements;
  uint miplevels;
  samp[value].GetDimensions(lod, width, height, elements, miplevels);
  return int2(width, height);
}}

)");
  }
  else
  {
    shader_source.Write(R"(
#define GLSL 1

// Type aliases.
#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4
#define float4x3 mat4x3

// Utility functions.
float saturate(float x) {{ return clamp(x, 0.0f, 1.0f); }}
float2 saturate(float2 x) {{ return clamp(x, float2(0.0f, 0.0f), float2(1.0f, 1.0f)); }}
float3 saturate(float3 x) {{ return clamp(x, float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f)); }}
float4 saturate(float4 x) {{ return clamp(x, float4(0.0f, 0.0f, 0.0f, 0.0f), float4(1.0f, 1.0f, 1.0f, 1.0f)); }}

// Flipped multiplication order because GLSL matrices use column vectors.
float2 mul(float2x2 m, float2 v) {{ return (v * m); }}
float3 mul(float3x3 m, float3 v) {{ return (v * m); }}
float4 mul(float4x3 m, float3 v) {{ return (v * m); }}
float4 mul(float4x4 m, float4 v) {{ return (v * m); }}
float2 mul(float2 v, float2x2 m) {{ return (m * v); }}
float3 mul(float3 v, float3x3 m) {{ return (m * v); }}
float3 mul(float4 v, float4x3 m) {{ return (m * v); }}
float4 mul(float4 v, float4x4 m) {{ return (m * v); }}

float4x3 mul(float4x3 m, float3x3 m2) {{ return (m2 * m); }}

SAMPLER_BINDING(0) uniform sampler2DArray samp[4];
VARYING_LOCATION(0) in float3 v_tex0;
VARYING_LOCATION(1) in float3 v_tex1;
FRAGMENT_OUTPUT_LOCATION(0) out float4 ocol0;

// Wrappers for sampling functions.
float4 SampleInput(int value) {{ return texture(samp[value], float3(v_tex0.xy, float(u_layer))); }}
float4 SampleInputLocation(int value, float2 location) {{ return texture(samp[value], float3(location, float(u_layer))); }}
float4 SampleInputLayer(int value, int layer) {{ return texture(samp[value], float3(v_tex0.xy, float(layer))); }}
float4 SampleInputLocationLayer(int value, float2 location, int layer) {{ return texture(samp[value], float3(location, float(layer))); }}

float4 SampleInputLod(int value, float lod) {{ return textureLod(samp[value], float3(v_tex0.xy, float(u_layer)), lod); }}
float4 SampleInputLodLocation(int value, float lod, float2 location) {{ return textureLod(samp[value], float3(location, float(u_layer)), lod); }}
float4 SampleInputLodLayer(int value, float lod, int layer) {{ return textureLod(samp[value], float3(v_tex0.xy, float(layer)), lod); }}
float4 SampleInputLodLocationLayer(int value, float lod, float2 location, int layer) {{ return textureLod(samp[value], float3(location, float(layer)), lod); }}

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

ivec3 SampleInputSize(int value, int lod) {{ return textureSize(samp[value], lod); }}
)");
  }

  if (g_ActiveConfig.backend_info.bSupportsReversedDepthRange)
    shader_source.Write("#define DEPTH_VALUE(val) (val)\n");
  else
    shader_source.Write("#define DEPTH_VALUE(val) (1.0 - (val))\n");

  pass.WriteShaderIndices(shader_source);

  for (const auto& option : m_options)
  {
    option->WriteShaderConstants(shader_source);
  }

  shader_source.Write(R"(

// Convert z/w -> linear depth
// https://gist.github.com/kovrov/a26227aeadde77b78092b8a962bd1a91
// http://dougrogers.blogspot.com/2013/02/how-to-derive-near-and-far-clip-plane.html
float ToLinearDepth(float depth)
{{
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
}}

// For backwards compatibility.
float4 Sample() {{ return SampleInput(PREV_OUTPUT_INPUT_INDEX); }}
float4 SampleLocation(float2 location) {{ return SampleInputLocation(PREV_OUTPUT_INPUT_INDEX, location); }}
float4 SampleLayer(int layer) {{ return SampleInputLayer(PREV_OUTPUT_INPUT_INDEX, layer); }}
float4 SamplePrev() {{ return SampleInput(PREV_OUTPUT_INPUT_INDEX); }}
float4 SamplePrevLocation(float2 location) {{ return SampleInputLocation(PREV_OUTPUT_INPUT_INDEX, location); }}
float SampleRawDepth() {{ return DEPTH_VALUE(SampleInput(DEPTH_BUFFER_INPUT_INDEX).x); }}
float SampleRawDepthLocation(float2 location) {{ return DEPTH_VALUE(SampleInputLocation(DEPTH_BUFFER_INPUT_INDEX, location).x); }}
float SampleDepth() {{ return ToLinearDepth(SampleRawDepth()); }}
float SampleDepthLocation(float2 location) {{ return ToLinearDepth(SampleRawDepthLocation(location)); }}
#define SampleOffset(offset) (SampleInputOffset(COLOR_BUFFER_INPUT_INDEX, offset))
#define SampleLayerOffset(offset, layer) (SampleInputLayerOffset(COLOR_BUFFER_INPUT_INDEX, layer, offset))
#define SamplePrevOffset(offset) (SampleInputOffset(PREV_OUTPUT_INPUT_INDEX, offset))
#define SampleRawDepthOffset(offset) (DEPTH_VALUE(SampleInputOffset(DEPTH_BUFFER_INPUT_INDEX, offset).x))
#define SampleDepthOffset(offset) (ToLinearDepth(SampleRawDepthOffset(offset)))

float2 GetResolution() {{ return prev_resolution.xy; }}
float2 GetInvResolution() {{ return prev_resolution.zw; }}
float2 GetWindowResolution() {{  return window_resolution.xy; }}
float2 GetInvWindowResolution() {{ return window_resolution.zw; }}
float2 GetCoordinates() {{ return v_tex0.xy; }}

float2 GetPrevResolution() {{ return prev_resolution.xy; }}
float2 GetInvPrevResolution() {{ return prev_resolution.zw; }}
float2 GetPrevRectOrigin() {{ return prev_rect.xy; }}
float2 GetPrevRectSize() {{ return prev_rect.zw; }}
float2 GetPrevCoordinates() {{ return v_tex0.xy; }}

float2 GetSrcResolution() {{ return src_resolution.xy; }}
float2 GetInvSrcResolution() {{ return src_resolution.zw; }}
float2 GetSrcRectOrigin() {{ return src_rect.xy; }}
float2 GetSrcRectSize() {{ return src_rect.zw; }}
float2 GetSrcCoordinates() {{ return v_tex1.xy; }}

float4 GetFragmentCoord() {{ return gl_FragCoord; }}

float GetLayer() {{ return u_layer; }}
float GetTime() {{ return u_time; }}

void SetOutput(float4 color) {{ ocol0 = color; }}

#define GetOption(x) (x)
#define OptionEnabled(x) (x)

)");
}

void Shader::PixelShaderFooter(ShaderCode& shader_source, const ShaderConfigPass& config_pass) const
{
  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
  {
    shader_source.Write("#undef main\n");
    shader_source.Write("void main(in float3 v_tex0_ : TEXCOORD0,\n");
    shader_source.Write("\tin float3 v_tex1_ : TEXCOORD1,\n");
    shader_source.Write("\tin float4 pos : SV_Position,\n");
    shader_source.Write("\tout float4 ocol0_ : SV_Target)\n");
    shader_source.Write("{{\n");
    shader_source.Write("\tv_tex0 = v_tex0_;\n");
    shader_source.Write("\tv_tex1 = v_tex1_;\n");
    shader_source.Write("\tv_fragcoord = pos;\n");

    if (config_pass.m_entry_point == "main")
    {
      shader_source.Write("\treal_main();\n");
    }
    else if (config_pass.m_entry_point.empty())
    {
      // No entry point should create a copy
      shader_source.Write("\tocol0 = SampleInput(0);\n");
    }
    else
    {
      shader_source.Write("\t{}(0);\n", config_pass.m_entry_point);
    }
    shader_source.Write("\tocol0_ = ocol0;\n");
    shader_source.Write("}}\n");
  }
  else
  {
    if (config_pass.m_entry_point.empty())
    {
      // No entry point should create a copy
      shader_source.Write("void main() {{ ocol0 = SampleInput(0);\n }}\n");
    }
    else if (config_pass.m_entry_point != "main")
    {
      shader_source.Write("void main()\n");
      shader_source.Write("{{\n");
      shader_source.Write("\t{}();\n", config_pass.m_entry_point);
      shader_source.Write("}}\n");
    }
  }
}
void Shader::VertexShaderMain(ShaderCode& shader_source) const
{
  if (g_ActiveConfig.backend_info.api_type == APIType::D3D)
  {
    shader_source.Write("void main(in uint id : SV_VertexID, out float3 v_tex0 : TEXCOORD0,\n");
    shader_source.Write(
        "          out float3 v_tex1 : TEXCOORD1, out float4 opos : SV_Position) {{\n");
  }
  else
  {
    if (g_ActiveConfig.backend_info.bSupportsGeometryShaders)
    {
      shader_source.Write("VARYING_LOCATION(0) out VertexData {{\n");
      shader_source.Write("  float3 v_tex0;\n");
      shader_source.Write("  float3 v_tex1;\n");
      shader_source.Write("}};\n");
    }
    else
    {
      shader_source.Write("VARYING_LOCATION(0) out float3 v_tex0;\n");
      shader_source.Write("VARYING_LOCATION(0) out float3 v_tex1;\n");
    }

    shader_source.Write("#define id gl_VertexID\n");
    shader_source.Write("#define opos gl_Position\n");
    shader_source.Write("void main() {{\n");
  }
  shader_source.Write("  v_tex0 = float3(float((id << 1) & 2), float(id & 2), 0.0f);\n");
  shader_source.Write(
      "  opos = float4(v_tex0.xy * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n");
  shader_source.Write("  v_tex1 = float3(src_rect.xy + (src_rect.zw * v_tex0.xy), 0.0f);\n");

  if (g_ActiveConfig.backend_info.api_type == APIType::Vulkan)
    shader_source.Write("  opos.y = -opos.y;\n");

  shader_source.Write("}}\n");
}
}  // namespace VideoCommon::PE
