#include <array>
#include <cstdint>
#include <iostream>
#include <queue>
#include <span>

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

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
    {
        std::cerr << "Failed to initialize OpenGL window" << std::endl;
        return EXIT_FAILURE;
    }

    cgtub::Canvas canvas_left        = cgtub::Canvas(window, cgtub::Extent{.x = 0.00f, .y = 0.f, .width = 0.248f, .height = 1.f});
    cgtub::Canvas canvas_middle_view = cgtub::Canvas(window, cgtub::Extent{.x = 0.25f, .y = 0.f, .width = 0.248f, .height = 1.f});
    cgtub::Canvas canvas_middle_clip = cgtub::Canvas(window, cgtub::Extent{.x = 0.50f, .y = 0.f, .width = 0.248f, .height = 1.f});
    cgtub::Canvas canvas_right       = cgtub::Canvas(window, cgtub::Extent{.x = 0.75f, .y = 0.f, .width = 0.25f, .height = 1.f});

    cgtub::SimpleRenderer renderer_left(canvas_left);
    cgtub::SimpleRenderer renderer_middle_view(canvas_middle_view);
    cgtub::SimpleRenderer renderer_middle_clip(canvas_middle_clip);
    cgtub::NDCRenderer    renderer_right(canvas_right);

    // --- WORLD AXES ---
    glm::vec3 coordinate_axes_start_end[] = {
        glm::vec3(0, 0, 0), glm::vec3(1, 0, 0),
        glm::vec3(0, 0, 0), glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 0), glm::vec3(0, 0, 1)};
    glm::vec3 coordinate_axes_color[] = {
        glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)};

    std::vector<glm::vec3>    bunny_vertices;
    std::vector<glm::u32vec3> bunny_indices;
    ex2::create_bunny_geometry(&bunny_vertices, &bunny_indices);

    glm::vec3 bunny_color(0.75f);

    // Sphere for camera visualization
    std::vector<glm::vec3>    sphere_vertices;
    std::vector<glm::u32vec3> sphere_indices;
    cgtub::create_sphere_geometry(0.03f, &sphere_vertices, &sphere_indices);

    float azimuth = 0.f;
    float fov     = 60.f;
    float size    = 0.8f;
    float znear   = 0.35f;
    float zfar    = 1.60f;

    ex2::TransformationType transformation_type = ex2::TransformationType::Orthographic;

    float time = static_cast<float>(glfwGetTime());

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

        // --- CAMERA POSITION ---
        glm::vec3 camera_origin = glm::vec3(
            std::sin(azimuth) * std::sin(glm::quarter_pi<float>()),
            std::cos(glm::quarter_pi<float>()),
            std::cos(azimuth) * std::sin(glm::quarter_pi<float>()));

        cgtub::clear(window, 0.f, 0.f, 0.f, 1.f);
        canvas_left.clear(glm::vec3(1.f));

        // WORLD AXES
        renderer_left.render_lines(coordinate_axes_start_end, coordinate_axes_color);

        // WORLD MESH
        renderer_left.render_mesh(bunny_vertices, bunny_indices, bunny_color);

        // --- BUILD VIEW MATRIX FIRST ---
        glm::vec3 world_camera_position = camera_origin;
        glm::vec3 world_target          = glm::vec3(0.f);
        glm::vec3 world_up(0.f, 1.f, 0.f);

        glm::vec3 forward  = glm::normalize(world_target - world_camera_position);
        glm::vec3 backward = -forward;
        glm::vec3 right    = glm::normalize(glm::cross(world_up, backward));
        glm::vec3 up       = glm::cross(backward, right);

        glm::mat4 view(1.f);

        // Die Matrix-Zuweisung war KORREKT!
        // view[0][0] -> Spalte 0, Zeile 0 (r.x)
        // view[0][1] -> Spalte 0, Zeile 1 (u.x)
        // Wir bauen die Matrix Zeile für Zeile auf (was in memory den Spalten entspricht)
        view[0][0] = right.x;
        view[0][1] = up.x;
        view[0][2] = backward.x;

        view[1][0] = right.y;
        view[1][1] = up.y;
        view[1][2] = backward.y;

        view[2][0] = right.z;
        view[2][1] = up.z;
        view[2][2] = backward.z;

        view[3][0] = -glm::dot(right, world_camera_position);
        view[3][1] = -glm::dot(up, world_camera_position);
        view[3][2] = -glm::dot(backward, world_camera_position);
        view[3][3] = 1.f;

        // --- RENDER CAMERA AXES ---
        ex2::render_camera(renderer_left, view, glm::mat4(1.f));

        // --- RENDER CAMERA SPHERE ---
        std::vector<glm::vec3> cam_vertices = sphere_vertices;
        for (auto& v : cam_vertices)
            v += camera_origin;

        renderer_left.render_mesh(cam_vertices, sphere_indices, glm::vec3(1, 0, 1));

        // END FRAME
        cgtub::end_frame(window);
    }

    cgtub::uninit(window, dispatcher);
    return EXIT_SUCCESS;
}