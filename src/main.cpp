#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <queue>
#include <span>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cgtub/camera.hpp>
#include <cgtub/camera_controller_turntable.hpp>
#include <cgtub/canvas.hpp>
#include <cgtub/event_dispatcher.hpp>
#include <cgtub/geometry.hpp>
#include <cgtub/gl_wrap.hpp>
#include <cgtub/ndc_renderer.hpp>
#include <cgtub/primitives.hpp>
#include <cgtub/simple_renderer.hpp>

#include "helper.hpp"

int main(int argc, char** argv)
{
    GLFWwindow*             window     = nullptr;
    cgtub::EventDispatcher* dispatcher = nullptr;
    if (!cgtub::init(1600, 400, "CG1", &window, &dispatcher))
        return EXIT_FAILURE;

    cgtub::Canvas canvas_left(window, {0.00f, 0.f, 0.248f, 1.f});
    cgtub::Canvas canvas_middle_view(window, {0.25f, 0.f, 0.248f, 1.f});
    cgtub::Canvas canvas_middle_clip(window, {0.50f, 0.f, 0.248f, 1.f});
    cgtub::Canvas canvas_right(window, {0.75f, 0.f, 0.25f, 1.f});

    cgtub::SimpleRenderer renderer_left(canvas_left);
    cgtub::SimpleRenderer renderer_middle_view(canvas_middle_view);
    cgtub::SimpleRenderer renderer_middle_clip(canvas_middle_clip);
    cgtub::NDCRenderer    renderer_right(canvas_right);

    glm::vec3 view_box[] = {
        {-1, -1, -1}, {1, -1, -1}, {-1, 1, -1}, {1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 1}};

    glm::vec3 box_lines[] = {
        // Bottom
        view_box[0],
        view_box[1],
        view_box[1],
        view_box[3],
        view_box[3],
        view_box[2],
        view_box[2],
        view_box[0],
        // Top
        view_box[4],
        view_box[5],
        view_box[5],
        view_box[7],
        view_box[7],
        view_box[6],
        view_box[6],
        view_box[4],
        // Vertical
        view_box[0],
        view_box[4],
        view_box[1],
        view_box[5],
        view_box[2],
        view_box[6],
        view_box[3],
        view_box[7],
    };

    std::vector<glm::vec3> box_colors(12, glm::vec3(0, 0, 0));

    glm::vec3 coordinate_axes_start_end[] = {
        {0, 0, 0},
        {1, 0, 0}, // x
        {0, 0, 0},
        {0, 1, 0}, // y
        {0, 0, 0},
        {0, 0, 1}, // z
    };
    glm::vec3 coordinate_axes_color[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    std::vector<glm::vec3>    bunny_vertices;
    std::vector<glm::u32vec3> bunny_indices;
    ex2::create_bunny_geometry(&bunny_vertices, &bunny_indices);
    glm::vec3 bunny_color(0.75);

    std::vector<glm::vec3>    sphere_vertices;
    std::vector<glm::u32vec3> sphere_indices;
    cgtub::create_sphere_geometry(0.03f, &sphere_vertices, &sphere_indices);

    // State
    float                   azimuth             = 0.f;
    float                   fov                 = 60.f;
    float                   size                = 0.8f;
    float                   znear               = 0.35f;
    float                   zfar                = 1.60f;
    ex2::TransformationType transformation_type = ex2::TransformationType::Orthographic;

    float time = static_cast<float>(glfwGetTime());

    // --- MAIN LOOP ---
    while (!glfwWindowShouldClose(window))
    {
        cgtub::begin_frame(window);

        float now = static_cast<float>(glfwGetTime());
        float dt  = now - time;
        time      = now;

        dispatcher->poll_window_events();
        canvas_left.update(dt, dispatcher);
        canvas_middle_view.update(dt, dispatcher);
        canvas_middle_clip.update(dt, dispatcher);
        canvas_right.update(dt, dispatcher);
        renderer_left.update(dt, dispatcher);
        renderer_middle_view.update(dt, dispatcher);
        renderer_middle_clip.update(dt, dispatcher);

        ex2::gui(&azimuth, &fov, &size, &znear, &zfar, &transformation_type);

        //  view Matrix Calculation
        glm::vec3 camera_origin = glm::vec3(
            std::sin(azimuth) * std::sin(glm::quarter_pi<float>()),
            std::cos(glm::quarter_pi<float>()),
            std::cos(azimuth) * std::sin(glm::quarter_pi<float>()));

        glm::vec3 target   = glm::vec3(0.0f);
        glm::vec3 v_dir    = glm::normalize(target - camera_origin);
        glm::vec3 world_up = glm::vec3(0, 1.0f, 0);
        glm::vec3 v_right  = glm::normalize(glm::cross(v_dir, world_up));
        glm::vec3 v_up     = glm::cross(v_right, v_dir);

        glm::mat4 R(1.0f);
        R[0] = glm::vec4(v_right.x, v_up.x, -v_dir.x, 0.0f);
        R[1] = glm::vec4(v_right.y, v_up.y, -v_dir.y, 0.0f);
        R[2] = glm::vec4(v_right.z, v_up.z, -v_dir.z, 0.0f);

        glm::mat4 T(1.0f);
        T[3] = glm::vec4(-camera_origin, 1.0f);

        glm::mat4 LookAt_Matrix = R * T;

        // Projection Matrix Calculation
        float r = size / 2.0f;
        float l = -size / 2.0f;
        float t = size / 2.0f;
        float b = -size / 2.0f;
        float n = znear;
        float f = zfar;

        // Perspective Variables
        float fov_rad      = glm::radians(fov);
        float tan_half_fov = std::tan(fov_rad / 2.0f);
        float S            = 1.0f / tan_half_fov;
        float A            = -(f + n) / (f - n);
        float B            = -(2.0f * f * n) / (f - n);

        glm::mat4 Projection_Matrix(1.0f);

        if (transformation_type == ex2::TransformationType::Orthographic)
        {
            Projection_Matrix    = glm::mat4(1.0f);
            Projection_Matrix[0] = glm::vec4(2 / (r - l), 0.0f, 0.0f, 0.0f);
            Projection_Matrix[1] = glm::vec4(0.0f, 2 / (t - b), 0.0f, 0.0f);
            Projection_Matrix[2] = glm::vec4(0.0f, 0.0f, -2 / (f - n), 0.0f);
            Projection_Matrix[3] = glm::vec4(-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1.0f);
        }
        else // Perspective
        {
            Projection_Matrix[0][0] = S;     // Scale X
            Projection_Matrix[1][1] = S;     // Scale Y
            Projection_Matrix[2][2] = A;     // Remap Z
            Projection_Matrix[2][3] = -1.0f; // Perspective divide preparation (w = -z)
            Projection_Matrix[3][2] = B;     // Z Offset
            Projection_Matrix[3][3] = 0.0f;  // w = 0
        }

        cgtub::clear(window, 0.f, 0.f, 0.f, 1.f);

        canvas_left.clear(glm::vec3(1.f));
        renderer_left.render_lines(coordinate_axes_start_end, coordinate_axes_color);
        renderer_left.render_mesh(bunny_vertices, bunny_indices, bunny_color);

        std::vector<glm::vec3> cam_sphere = sphere_vertices;
        for (auto& v : cam_sphere)
            v += camera_origin;
        renderer_left.render_mesh(cam_sphere, sphere_indices, glm::vec3(1, 0, 1));

        ex2::render_camera(renderer_left, LookAt_Matrix, Projection_Matrix);

        canvas_middle_view.clear(glm::vec3(1.f));

        std::vector<glm::vec3> bunny_view;
        bunny_view.reserve(bunny_vertices.size());
        for (const auto& v : bunny_vertices)
        {
            bunny_view.push_back(glm::vec3(LookAt_Matrix * glm::vec4(v, 1.0f)));
        }

        std::vector<glm::vec3> axes_view;
        for (const auto& v : coordinate_axes_start_end)
        {
            axes_view.push_back(glm::vec3(LookAt_Matrix * glm::vec4(v, 1.0f)));
        }

        renderer_middle_view.render_mesh(bunny_view, bunny_indices, bunny_color);
        renderer_middle_view.render_lines(axes_view, coordinate_axes_color);
        ex2::render_camera(renderer_middle_view, glm::mat4(1.0f), Projection_Matrix);

        canvas_middle_clip.clear(glm::vec3(1.0f));

        std::vector<glm::vec3> bunny_ndc_visual_vec3;
        std::vector<glm::vec4> bunny_ndc_real_vec4;

        bunny_ndc_visual_vec3.reserve(bunny_vertices.size());
        bunny_ndc_real_vec4.reserve(bunny_vertices.size());

        for (const auto& v : bunny_view)
        {
            glm::vec4 p = Projection_Matrix * glm::vec4(v, 1.0f);

            bunny_ndc_real_vec4.push_back(p);

            if (p.w != 0.0f)
                bunny_ndc_visual_vec3.push_back(glm::vec3(p) / p.w);
            else
                bunny_ndc_visual_vec3.push_back(glm::vec3(p));
        }

        renderer_middle_clip.render_mesh(bunny_ndc_visual_vec3, bunny_indices, bunny_color);
        renderer_middle_clip.render_lines(box_lines, box_colors);

        std::vector<glm::vec3> ndc_axes_visual;
        std::vector<glm::vec4> ndc_axes_real;

        for (const auto& v : axes_view)
        {
            glm::vec4 p = Projection_Matrix * glm::vec4(v, 1.0f);
            ndc_axes_real.push_back(p);

            if (p.w != 0.0f)
                ndc_axes_visual.push_back(glm::vec3(p) / p.w);
            else
                ndc_axes_visual.push_back(glm::vec3(p));
        }
        renderer_middle_clip.render_lines(ndc_axes_visual, coordinate_axes_color);

        canvas_right.clear(glm::vec3(1.0f));

        renderer_right.render_mesh(bunny_ndc_real_vec4, bunny_indices, bunny_color);
        renderer_right.render_lines(ndc_axes_real, coordinate_axes_color);

        cgtub::end_frame(window);
    }

    cgtub::uninit(window, dispatcher);
    return EXIT_SUCCESS;
}