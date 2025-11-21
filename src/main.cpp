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
    ////////////////////////////////////////////////////////////////
    // Initialization of display structures (window, canvas, renderer, ...)
    ////////////////////////////////////////////////////////////////

    // Create a GLFW window and an OpenGL context
    // The event dispatcher records incoming events for a window
    // (e.g. change of size or mouse cursor movement)
    GLFWwindow*             window     = nullptr;
    cgtub::EventDispatcher* dispatcher = nullptr;
    if (!cgtub::init(1600, 400, "CG1", &window, &dispatcher))
    {
        std::cerr << "Failed to initialize OpenGL window" << std::endl;
        return EXIT_FAILURE;
    }

    // A canvas is a (logical) subregion of a window, defined by a (normalized) extent. 
    // We create four canvas that cover one fourth of the window width.

    cgtub::Canvas canvas_left        = cgtub::Canvas(window, cgtub::Extent{.x = 0.00f, .y = 0.f, .width = 0.248f, .height = 1.f});
    cgtub::Canvas canvas_middle_view = cgtub::Canvas(window, cgtub::Extent{.x = 0.25f, .y = 0.f, .width = 0.248f, .height = 1.f});
    cgtub::Canvas canvas_middle_clip = cgtub::Canvas(window, cgtub::Extent{.x = 0.50f, .y = 0.f, .width = 0.248f, .height = 1.f});
    cgtub::Canvas canvas_right       = cgtub::Canvas(window, cgtub::Extent{.x = 0.75f, .y = 0.f, .width = 0.25f, .height = 1.f});

    // A renderer can display primitives to a canvas (and a window respectively)
    // Internally, it manages a camera (the way we look at the virtual scene)
    // Each renderer corresponds to a single canvas.

    cgtub::SimpleRenderer    renderer_left(canvas_left);
    cgtub::SimpleRenderer    renderer_middle_view(canvas_middle_view);
    cgtub::SimpleRenderer    renderer_middle_clip(canvas_middle_clip);

    // This renderer takes normalized device coordinates and renders them accordingly.
    cgtub::NDCRenderer       renderer_right(canvas_right);

    ////////////////////////////////////////////////////////////////
    // Initialization of data (geometry, application state, ...)
    ////////////////////////////////////////////////////////////////

    // Define the axis of the world coordinate system using
    // start and end points of the lines (all axis start in the origin)
    glm::vec3 coordinate_axes_start_end[] = {
        glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), // x-axis
        glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), // y-axis
        glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), // z-axis
    };

    glm::vec3 coordinate_axes_color[] = {
        glm::vec3(1, 0, 0), // (r,g,b) color for the x-axis (red)
        glm::vec3(0, 1, 0), // (r,g,b) color for the y-axis (green)
        glm::vec3(0, 0, 1), // (r,g,b) color for the z-axis (blue)
    };

    // Create the geometry of the stanford bunny
    std::vector<glm::vec3> bunny_vertices;
    std::vector<glm::u32vec3> bunny_indices;
    ex2::create_bunny_geometry(&bunny_vertices, &bunny_indices);
    glm::vec3 bunny_color(0.75); // color used for rendering the mesh

    // Create a simple Sphere 
    std::vector<glm::vec3> sphere_vertices;
    std::vector<glm::u32vec3> sphere_indices;
    cgtub::create_sphere_geometry(0.03f, &sphere_vertices, &sphere_indices);

    // Camera position
    float azimuth = 0.f;

    // Internal camera parameters
    float fov   = 60.f;
    float size  = 0.8f;
    float znear = 0.35f;
    float zfar  = 1.60f;

    // You need this to switch between orthographic and perspective transformation
    ex2::TransformationType transformation_type = ex2::TransformationType::Orthographic;

    ////////////////////////////////////////////////////////////////
    // Main loop: one iteration is one frame
    ////////////////////////////////////////////////////////////////
    float time = static_cast<float>(glfwGetTime());
    while (!glfwWindowShouldClose(window))
    {
        cgtub::begin_frame(window);

        // Track current time and elapsed time between frames (dt)
        float now = static_cast<float>(glfwGetTime());
        float dt  = now - time;
        time      = now;

        // Poll and record window events (resizing, key inputs, etc.)
        // The dispatcher is implicitly connected to the window and receives these events.
        dispatcher->poll_window_events();

        // The canvases and renderers must react to incoming events (resizing, user inputs, ...)
        canvas_left.update(dt, dispatcher);
        canvas_middle_view.update(dt, dispatcher);
        canvas_middle_clip.update(dt, dispatcher);
        canvas_right.update(dt, dispatcher);
        renderer_left.update(dt, dispatcher);
        renderer_middle_view.update(dt, dispatcher);
        renderer_middle_clip.update(dt, dispatcher);

        // Update the state of these variables through the GUI
        ex2::gui(&azimuth, &fov, &size, &znear, &zfar, &transformation_type);

        // Compute the camera origin from azimuth
        glm::vec3 camera_origin = glm::vec3(
            std::sin(azimuth) * std::sin(glm::quarter_pi<float>()),
            std::cos(glm::quarter_pi<float>()),
            std::cos(azimuth) * std::sin(glm::quarter_pi<float>()));

        /////////////////////////////////////////
        // Render the geometry using the current application state
        /////////////////////////////////////////
        
        // Clear the full window (clears the gaps between canvases)
        cgtub::clear(window, 0.f, 0.f, 0.f, 1.f); 

        // Clear the state of the left-most canvas
        canvas_left.clear(glm::vec3(1.f, 1.f, 1.f));

        // Render the world coordinate system
        renderer_left.render_lines(coordinate_axes_start_end, coordinate_axes_color);

        // Render the bunny geometry in world space
        renderer_left.render_mesh(bunny_vertices, bunny_indices, bunny_color);

        // Render the camera position as a sphere in world space
        std::vector<glm::vec3> camera_origin_vertices = sphere_vertices;
        for (glm::vec3& v : camera_origin_vertices)
        {
            v += camera_origin;
        }
        renderer_left.render_mesh(camera_origin_vertices, sphere_indices, glm::vec3(1, 0, 1));

        cgtub::end_frame(window);
    }

    cgtub::uninit(window, dispatcher);

    return EXIT_SUCCESS;
}