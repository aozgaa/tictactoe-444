#include <SDL.h>
#include <SDL_metal.h>

#include "Renderer.hpp"
#include "Game.hpp"

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <imgui.h>
#include <imgui_impl_metal.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

struct Vertex { float pos[3]; float nor[3]; };

static std::array<Vertex, 24> cube_vertices() {
    std::array<Vertex, 24> v{};
    v[ 0]={{ 0.5f,-0.5f,-0.5f},{ 1, 0, 0}}; v[ 1]={{ 0.5f, 0.5f,-0.5f},{ 1, 0, 0}};
    v[ 2]={{ 0.5f, 0.5f, 0.5f},{ 1, 0, 0}}; v[ 3]={{ 0.5f,-0.5f, 0.5f},{ 1, 0, 0}};
    v[ 4]={{-0.5f,-0.5f, 0.5f},{-1, 0, 0}}; v[ 5]={{-0.5f, 0.5f, 0.5f},{-1, 0, 0}};
    v[ 6]={{-0.5f, 0.5f,-0.5f},{-1, 0, 0}}; v[ 7]={{-0.5f,-0.5f,-0.5f},{-1, 0, 0}};
    v[ 8]={{-0.5f, 0.5f,-0.5f},{ 0, 1, 0}}; v[ 9]={{-0.5f, 0.5f, 0.5f},{ 0, 1, 0}};
    v[10]={{ 0.5f, 0.5f, 0.5f},{ 0, 1, 0}}; v[11]={{ 0.5f, 0.5f,-0.5f},{ 0, 1, 0}};
    v[12]={{-0.5f,-0.5f, 0.5f},{ 0,-1, 0}}; v[13]={{-0.5f,-0.5f,-0.5f},{ 0,-1, 0}};
    v[14]={{ 0.5f,-0.5f,-0.5f},{ 0,-1, 0}}; v[15]={{ 0.5f,-0.5f, 0.5f},{ 0,-1, 0}};
    v[16]={{-0.5f,-0.5f, 0.5f},{ 0, 0, 1}}; v[17]={{ 0.5f,-0.5f, 0.5f},{ 0, 0, 1}};
    v[18]={{ 0.5f, 0.5f, 0.5f},{ 0, 0, 1}}; v[19]={{-0.5f, 0.5f, 0.5f},{ 0, 0, 1}};
    v[20]={{ 0.5f,-0.5f,-0.5f},{ 0, 0,-1}}; v[21]={{-0.5f,-0.5f,-0.5f},{ 0, 0,-1}};
    v[22]={{-0.5f, 0.5f,-0.5f},{ 0, 0,-1}}; v[23]={{ 0.5f, 0.5f,-0.5f},{ 0, 0,-1}};
    return v;
}

static std::array<uint16_t, 36> cube_indices() {
    std::array<uint16_t, 36> idx{};
    for (int f = 0; f < 6; ++f) {
        uint16_t b = (uint16_t)(f * 4);
        idx[f*6+0]=b+0; idx[f*6+1]=b+1; idx[f*6+2]=b+2;
        idx[f*6+3]=b+0; idx[f*6+4]=b+2; idx[f*6+5]=b+3;
    }
    return idx;
}

Renderer::Renderer(SDL_Window* window) {
    SDL_Metal_GetDrawableSize(window, &fb_width, &fb_height);
    SDL_GetWindowSize(window, &win_width, &win_height);

    SDL_MetalView sdl_view = SDL_Metal_CreateView(window);
    layer_ = (CA::MetalLayer*)SDL_Metal_GetLayer(sdl_view);

    device_ = MTL::CreateSystemDefaultDevice();
    layer_->setDevice(device_);
    layer_->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    layer_->setDrawableSize(CGSizeMake(fb_width, fb_height));

    queue_ = device_->newCommandQueue();
    build_pipeline();
    build_geometry();

    auto* td = MTL::TextureDescriptor::alloc()->init();
    td->setTextureType(MTL::TextureType2D);
    td->setPixelFormat(MTL::PixelFormatDepth32Float);
    td->setWidth(fb_width); td->setHeight(fb_height);
    td->setStorageMode(MTL::StorageModePrivate);
    td->setUsage(MTL::TextureUsageRenderTarget);
    depth_tex_ = device_->newTexture(td);
    td->release();

    ImGui_ImplMetal_Init((__bridge id<MTLDevice>)device_);
}

Renderer::~Renderer() {
    ImGui_ImplMetal_Shutdown();
    if (pipeline_)  pipeline_->release();
    if (depth_ds_)  depth_ds_->release();
    if (vbuf_)      vbuf_->release();
    if (ibuf_)      ibuf_->release();
    if (depth_tex_) depth_tex_->release();
    if (queue_)     queue_->release();
    if (device_)    device_->release();
}

void Renderer::build_pipeline() {
    NS::Error* err = nullptr;
    auto* lib = device_->newDefaultLibrary();
    if (!lib) throw std::runtime_error("Failed to load default.metallib");

    auto* vert = lib->newFunction(NS::String::string("vert_main", NS::UTF8StringEncoding));
    auto* frag = lib->newFunction(NS::String::string("frag_main", NS::UTF8StringEncoding));

    auto* vd = MTL::VertexDescriptor::alloc()->init();
    vd->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    vd->attributes()->object(0)->setOffset(0);  vd->attributes()->object(0)->setBufferIndex(0);
    vd->attributes()->object(1)->setFormat(MTL::VertexFormatFloat3);
    vd->attributes()->object(1)->setOffset(12); vd->attributes()->object(1)->setBufferIndex(0);
    vd->layouts()->object(0)->setStride(sizeof(Vertex));

    auto* pd = MTL::RenderPipelineDescriptor::alloc()->init();
    pd->setVertexFunction(vert); pd->setFragmentFunction(frag);
    pd->setVertexDescriptor(vd);
    pd->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
    pd->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    auto* ca = pd->colorAttachments()->object(0);
    ca->setBlendingEnabled(true);
    ca->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    ca->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    ca->setRgbBlendOperation(MTL::BlendOperationAdd);
    ca->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
    ca->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    ca->setAlphaBlendOperation(MTL::BlendOperationAdd);

    pipeline_ = device_->newRenderPipelineState(pd, &err);
    if (!pipeline_) throw std::runtime_error("Pipeline creation failed");
    pd->release(); vd->release(); vert->release(); frag->release(); lib->release();

    auto* dsd = MTL::DepthStencilDescriptor::alloc()->init();
    dsd->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    dsd->setDepthWriteEnabled(false);
    depth_ds_ = device_->newDepthStencilState(dsd);
    dsd->release();
}

void Renderer::build_geometry() {
    auto verts = cube_vertices();
    auto idxs  = cube_indices();
    index_count_ = (uint32_t)idxs.size();
    vbuf_ = device_->newBuffer(verts.data(), sizeof(verts), MTL::ResourceStorageModeShared);
    ibuf_ = device_->newBuffer(idxs.data(),  sizeof(idxs),  MTL::ResourceStorageModeShared);
}

glm::mat4 Renderer::view_proj() const {
    float aspect = (float)fb_width / (float)fb_height;
    const glm::mat4 z_remap(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f
    );
    // Camera pulled back to z=9 to fit the larger 4x4x4 grid
    glm::mat4 proj = z_remap * glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 0, 9.0f), glm::vec3(0), glm::vec3(0, 1, 0));
    return proj * view;
}

void Renderer::begin_frame() {
    drawable_ = layer_->nextDrawable();
    rpa_ = MTL::RenderPassDescriptor::alloc()->init();
    rpa_->colorAttachments()->object(0)->setTexture(drawable_->texture());
    rpa_->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    rpa_->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
    rpa_->colorAttachments()->object(0)->setClearColor({0.13, 0.13, 0.13, 1.0});
    rpa_->depthAttachment()->setTexture(depth_tex_);
    rpa_->depthAttachment()->setLoadAction(MTL::LoadActionClear);
    rpa_->depthAttachment()->setClearDepth(1.0);
    rpa_->depthAttachment()->setStoreAction(MTL::StoreActionDontCare);
    ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor*)rpa_);
}

struct Uniforms { float mvp[16]; float color[4]; };

void Renderer::end_frame(const Game& game, float cell_size,
                         int hovered_cell, glm::vec4 empty_color) {
    if (!drawable_) return;

    auto* cmd = queue_->commandBuffer();
    auto* enc = cmd->renderCommandEncoder(rpa_);
    rpa_->release(); rpa_ = nullptr;

    enc->setRenderPipelineState(pipeline_);
    enc->setDepthStencilState(depth_ds_);
    enc->setVertexBuffer(vbuf_, 0, 0);
    enc->setFrontFacingWinding(MTL::WindingCounterClockwise);
    enc->setCullMode(MTL::CullModeBack);

    glm::mat4 vp  = view_proj();
    glm::mat4 rot = glm::toMat4(rotation_);
    float scale   = cell_size * 0.95f;

    // Sort cells back-to-front (camera at (0,0,9))
    struct Entry { int x, y, z; float cam_z; };
    std::array<Entry, N*N*N> order;
    int oi = 0;
    for (int x = 0; x < N; ++x)
    for (int y = 0; y < N; ++y)
    for (int z = 0; z < N; ++z) {
        // Cell center: (x - (N-1)/2, ...) so the grid is centred at origin
        float cx = x - (N-1)*0.5f, cy = y - (N-1)*0.5f, cz = z - (N-1)*0.5f;
        glm::vec4 wp = rot * glm::vec4(cx, cy, cz, 1);
        order[oi++] = {x, y, z, wp.z - 9.0f};
    }
    std::sort(order.begin(), order.end(),
              [](const Entry& a, const Entry& b){ return a.cam_z < b.cam_z; });

    for (auto& e : order) {
        int x = e.x, y = e.y, z = e.z;
        int idx = x*N*N + y*N + z;
        Cell c = game.board[x][y][z];

        const bool highlighted = game.highlighted[x][y][z];

        glm::vec4 col;
        if (c == Cell::X) {
            col = highlighted ? glm::vec4{1.0f, 0.92f, 0.35f, 1.0f}
                              : glm::vec4{0.95f, 0.25f, 0.25f, 0.85f};
        } else if (c == Cell::O) {
            col = highlighted ? glm::vec4{1.0f, 0.92f, 0.35f, 1.0f}
                              : glm::vec4{0.25f, 0.50f, 0.95f, 0.85f};
        } else if (idx == hovered_cell) {
            col = {empty_color.r, empty_color.g, empty_color.b, 0.60f};
        } else {
            col = empty_color;
        }

        float cx = x - (N-1)*0.5f, cy = y - (N-1)*0.5f, cz = z - (N-1)*0.5f;
        glm::mat4 model = rot
            * glm::translate(glm::mat4(1), glm::vec3(cx, cy, cz))
            * glm::scale(glm::mat4(1), glm::vec3(scale));
        glm::mat4 mvp = vp * model;

        Uniforms u{};
        memcpy(u.mvp, &mvp[0][0], 64);
        u.color[0]=col.r; u.color[1]=col.g; u.color[2]=col.b; u.color[3]=col.a;
        enc->setVertexBytes(&u, sizeof(u), 1);
        enc->setFragmentBytes(&u, sizeof(u), 1);
        enc->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, index_count_,
                                   MTL::IndexTypeUInt16, ibuf_, 0);
    }

    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(),
        (__bridge id<MTLCommandBuffer>)cmd,
        (__bridge id<MTLRenderCommandEncoder>)enc);

    enc->endEncoding();
    cmd->presentDrawable(reinterpret_cast<MTL::Drawable*>(drawable_));
    cmd->commit();
    drawable_ = nullptr;
}

void Renderer::apply_rotation_delta(float dx, float dy) {
    const float k = 0.005f;
    glm::quat qy = glm::angleAxis(dx * k, glm::vec3(0, 1, 0));
    glm::quat qx = glm::angleAxis(dy * k, glm::vec3(1, 0, 0));
    rotation_ = glm::normalize(qy * qx * rotation_);
}

void Renderer::reset_orientation() { rotation_ = glm::quat(1, 0, 0, 0); }

int Renderer::pick(float mouse_x, float mouse_y, float cell_size) const {
    float ndcx =  2.0f * mouse_x / win_width  - 1.0f;
    float ndcy =  1.0f - 2.0f * mouse_y / win_height;

    glm::mat4 inv_vp = glm::inverse(view_proj());
    auto unproject = [&](float ndc_z) -> glm::vec3 {
        glm::vec4 p = inv_vp * glm::vec4(ndcx, ndcy, ndc_z, 1.0f);
        return glm::vec3(p) / p.w;
    };

    glm::vec3 near_ws = unproject(0.0f);
    glm::vec3 far_ws  = unproject(1.0f);
    glm::vec3 ray_dir = glm::normalize(far_ws - near_ws);

    glm::mat4 inv_rot = glm::toMat4(glm::inverse(rotation_));
    glm::vec3 ro = glm::vec3(inv_rot * glm::vec4(near_ws, 1));
    glm::vec3 rd = glm::vec3(inv_rot * glm::vec4(ray_dir, 0));

    float half  = cell_size * 0.95f * 0.5f;
    int   best  = NO_PICK;
    float best_t = 1e30f;

    for (int x = 0; x < N; ++x)
    for (int y = 0; y < N; ++y)
    for (int z = 0; z < N; ++z) {
        glm::vec3 center(x-(N-1)*0.5f, y-(N-1)*0.5f, z-(N-1)*0.5f);
        float tmin = 1e-3f, tmax = 1e30f;
        for (int a = 0; a < 3; ++a) {
            float inv_d = (rd[a] != 0.0f) ? 1.0f / rd[a] : 1e30f;
            float t1 = (center[a] - half - ro[a]) * inv_d;
            float t2 = (center[a] + half - ro[a]) * inv_d;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            tmin = (t1 > tmin) ? t1 : tmin;
            tmax = (t2 < tmax) ? t2 : tmax;
        }
        if (tmax > tmin && tmin < best_t) {
            best_t = tmin;
            best   = x*N*N + y*N + z;
        }
    }
    return best;
}
