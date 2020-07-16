// Copyright 2014 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "Common/CommonTypes.h"
#include "Common/Matrix.h"
#include "Common/Timer.h"
#include "VideoCommon/PostProcessingConfig.h"
#include "VideoCommon/RenderState.h"
#include "VideoCommon/TextureConfig.h"

class AbstractFramebuffer;
class AbstractTexture;
class AbstractPipeline;
class AbstractShader;
class AbstractTexture;

namespace VideoCommon::PostProcessing
{
class Instance final
{
public:
  explicit Instance(Config* config);
  ~Instance();

  bool RequiresDepthBuffer() const { return m_config->RequiresDepthBuffer(); }
  bool IsValid() const;

  bool Apply(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
             const AbstractTexture* source_color_tex, const AbstractTexture* source_depth_tex,
             const MathUtil::Rectangle<int>& source_rect, int source_layer);
  bool ShadersCompiled();

  void SetDepthNearFar(float depth_near, float depth_far);

protected:
  struct InputBinding final
  {
    InputType type;
    u32 texture_unit;
    SamplerState sampler_state;
    const AbstractTexture* texture_ptr;
    std::unique_ptr<AbstractTexture> owned_texture_ptr;
    u32 source_pass_index;
  };

  struct RenderPass final
  {
    std::shared_ptr<AbstractShader> vertex_shader;
    std::unique_ptr<AbstractShader> pixel_shader;
    std::unique_ptr<AbstractPipeline> pipeline;
    std::vector<InputBinding> inputs;

    std::unique_ptr<AbstractTexture> output_texture;
    std::unique_ptr<AbstractFramebuffer> output_framebuffer;
    float output_scale;

    u32 shader_index;
    u32 shader_pass_index;
  };

  std::string GetUniformBufferHeader(const Shader& shader) const;
  std::string GetPixelShaderHeader(const Shader& shader, const Pass& pass) const;
  std::string GetPixelShaderFooter(const Shader& shader, const Pass& pass) const;

  std::shared_ptr<AbstractShader> CompileVertexShader(const Shader& shader) const;
  std::unique_ptr<AbstractShader> CompilePixelShader(const Shader& shader, const Pass& pass) const;
  std::unique_ptr<AbstractShader> CompileGeometryShader() const;

  size_t CalculateUniformsSize(const Shader& shader) const;
  void UploadUniformBuffer(const Shader& shader, const AbstractTexture* prev_texture,
                           const MathUtil::Rectangle<int>& prev_rect,
                           const AbstractTexture* source_color_texture,
                           const MathUtil::Rectangle<int>& source_rect, int source_layer,
                           float last_pass_output_scale);

  bool CompilePasses();
  bool CreateInputBinding(u32 shader_index, const Input& input, InputBinding* binding);
  const RenderPass* GetRenderPass(u32 shader_index, u32 pass_index) const;
  u32 GetRenderPassIndex(u32 shader_index, u32 pass_index) const;

  bool CreateOutputTextures(u32 new_width, u32 new_height, u32 new_layers,
                            AbstractTextureFormat new_format);
  void LinkPassOutputs();
  bool CompilePipelines();

  // Timer for determining our time value
  Common::Timer m_timer;
  Config* m_config;

  std::unique_ptr<AbstractShader> m_vertex_shader;
  std::unique_ptr<AbstractShader> m_geometry_shader;

  std::vector<u8> m_uniform_staging_buffer;

  // intermediate buffer sizes
  u32 m_target_width = 0;
  u32 m_target_height = 0;
  u32 m_target_layers = 0;
  AbstractTextureFormat m_target_format = AbstractTextureFormat::Undefined;

  std::vector<RenderPass> m_render_passes;
  bool m_last_pass_uses_color_buffer = false;
  bool m_last_pass_scaled = false;
  u64 m_last_change_count = 0;

  float m_depth_near = 0.001f;
  float m_depth_far = 1.0f;
};

class System
{
public:
  System();

  bool IsEFBValid() const { return m_efb_instance.IsValid(); }
  bool IsPostValid() const { return m_post_instance.IsValid(); }
  bool IsXFBValid() const { return m_xfb_instance.IsValid(); }
  bool IsTextureValid() const { return m_texture_instance.IsValid(); }
  bool IsDownsampleValid() const { return m_downsample_instance.IsValid(); }

  bool ApplyEFB(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                const AbstractTexture* source_color_tex, const AbstractTexture* source_depth_tex,
                const MathUtil::Rectangle<int>& source_rect, int source_layer);

  bool ApplyXFB(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                const AbstractTexture* source_color_tex, const AbstractTexture* source_depth_tex,
                const MathUtil::Rectangle<int>& source_rect, int source_layer);

  bool ApplyPost(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                 const AbstractTexture* source_color_tex,
                 const MathUtil::Rectangle<int>& source_rect, int source_layer);

  bool ApplyTextures(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                    const AbstractTexture* source_color_tex,
                    const MathUtil::Rectangle<int>& source_rect, int source_layer);

  bool ApplyDownsample(AbstractFramebuffer* dest_fb, const MathUtil::Rectangle<int>& dest_rect,
                       const AbstractTexture* source_color_tex,
                       const MathUtil::Rectangle<int>& source_rect, int source_layer);

  void OnProjectionChanged(u32 type, const Common::Matrix44& projection_mtx);
  void OnFrameEnd(std::optional<MathUtil::Rectangle<int>> xfb_rect);
  void OnEFBWritten(const MathUtil::Rectangle<int>& efb_rect);
  void OnOpcodeDrawingStateChanged(bool is_drawing);
  void OnEFBTriggered();

private:
  void ProcessEFB(std::optional<MathUtil::Rectangle<int>> efb_rect);
  void ProcessEFBRect(const MathUtil::Rectangle<int>& framebuffer_rect);
  enum ProjectionState : u32
  {
    Initial,
    Perspective,
    Final
  };
  ProjectionState m_projection_state = ProjectionState::Initial;
  bool m_saw_2d_element = false;
  bool m_efb_triggered = false;
  bool m_ignore_2d = true;
  u8 m_draw_count = 0;

  Instance m_efb_instance;
  Instance m_xfb_instance;
  Instance m_post_instance;
  Instance m_texture_instance;
  Instance m_downsample_instance;
};

}  // namespace VideoCommon::PostProcessing
