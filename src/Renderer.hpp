#pragma once

#include "Game.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

struct SDL_Window;

static constexpr int NO_PICK = -1;

class Renderer {
public:
  explicit Renderer(SDL_Window *window);
  ~Renderer();

  void begin_frame();

  // hovered_cell: flat index x*N*N + y*N + z, or NO_PICK
  // empty_color: base color for unoccupied cells
  void end_frame(const Game &game, float cell_size, int hovered_cell, glm::vec4 empty_color);

  void apply_rotation_delta(float dx, float dy);
  void reset_orientation();

  // Returns x*N*N + y*N + z, or NO_PICK.
  int pick(float mouse_x, float mouse_y, float cell_size) const;

  int fb_width = 0, fb_height = 0;
  int win_width = 0, win_height = 0;

private:
  void build_pipeline();
  void build_geometry();
  glm::mat4 view_proj() const;

  MTL::Device *device_ = nullptr;
  MTL::CommandQueue *queue_ = nullptr;
  MTL::RenderPipelineState *pipeline_ = nullptr;
  MTL::DepthStencilState *depth_ds_ = nullptr;
  MTL::Buffer *vbuf_ = nullptr;
  MTL::Buffer *ibuf_ = nullptr;
  MTL::Texture *depth_tex_ = nullptr;
  CA::MetalLayer *layer_ = nullptr;

  CA::MetalDrawable *drawable_ = nullptr;
  MTL::RenderPassDescriptor *rpa_ = nullptr;

  glm::quat rotation_{1, 0, 0, 0};
  uint32_t index_count_ = 0;
};
