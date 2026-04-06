#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>

#include "Game.hpp"
#include "Renderer.hpp"

#include <cmath>

int main(int /*argc*/, char* /*argv*/[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "4x4x4 Tic-Tac-Toe",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 800,
        SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) { SDL_Log("SDL_CreateWindow: %s", SDL_GetError()); return 1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForMetal(window);

    Renderer renderer(window);

    Game  game;
    float cell_size = 0.72f;  // slightly smaller default — 64 cells is denser
    float empty_color[4] = {0.50f, 0.50f, 0.55f, 0.18f};

    bool  mouse_down   = false;
    bool  is_drag      = false;
    float drag_start_x = 0, drag_start_y = 0;
    float cur_x = 0, cur_y = 0;
    float last_x = 0, last_y = 0;
    const float DRAG_THRESH = 4.0f;

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            switch (ev.type) {
            case SDL_QUIT:   running = false; break;
            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = false; break;
            case SDL_MOUSEMOTION:
                cur_x = (float)ev.motion.x; cur_y = (float)ev.motion.y;
                if (mouse_down && !io.WantCaptureMouse) {
                    float tdx = cur_x - drag_start_x, tdy = cur_y - drag_start_y;
                    if (!is_drag && sqrtf(tdx*tdx + tdy*tdy) > DRAG_THRESH) is_drag = true;
                    if (is_drag) renderer.apply_rotation_delta(cur_x - last_x, cur_y - last_y);
                    last_x = cur_x; last_y = cur_y;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT && !io.WantCaptureMouse) {
                    mouse_down = true; is_drag = false;
                    drag_start_x = last_x = cur_x = (float)ev.button.x;
                    drag_start_y = last_y = cur_y = (float)ev.button.y;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    if (!is_drag && mouse_down && !io.WantCaptureMouse && !game.game_over) {
                        int idx = renderer.pick(cur_x, cur_y, cell_size);
                        if (idx != NO_PICK)
                            game.make_move(idx/(N*N), (idx/N)%N, idx%N);
                    }
                    mouse_down = is_drag = false;
                }
                break;
            }
        }

        int hovered = NO_PICK;
        if (!mouse_down && !io.WantCaptureMouse)
            hovered = renderer.pick(cur_x, cur_y, cell_size);

        renderer.begin_frame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({290, 0}, ImGuiCond_Always);
        ImGui::Begin("Controls", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::SliderFloat("Cell Size", &cell_size, 0.10f, 1.00f);
        if (ImGui::Button("Reset Orientation")) renderer.reset_orientation();
        ImGui::SameLine();
        if (ImGui::Button("New Game")) game.reset();

        ImGui::Separator();
        ImGui::Text("Empty cell color:");
        ImGui::ColorEdit4("##face_color", empty_color,
            ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_AlphaBar);

        ImGui::Separator();
        if (game.winner != Cell::Empty) {
            const char* who = (game.winner == Cell::X) ? "X wins!" : "O wins!";
            ImGui::TextColored({1.0f, 1.0f, 0.2f, 1.0f}, "%s", who);
        } else if (game.game_over) {
            ImGui::TextColored({0.6f, 0.6f, 0.6f, 1.0f}, "Draw!");
        } else {
            ImGui::Text("Turn: %s", game.current_player == Cell::X ? "X" : "O");
        }
        ImGui::Text("Drag=rotate  Click=place");
        ImGui::Text("4x4x4 - get 4 in a row to win");

        ImGui::End();
        ImGui::Render();

        renderer.end_frame(game, cell_size, hovered,
            {empty_color[0], empty_color[1], empty_color[2], empty_color[3]});
    }

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
