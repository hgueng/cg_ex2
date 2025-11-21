#include "helper.hpp"

#include <imgui.h>

#include "bunny.hpp"

namespace ex2
{

void render_camera(cgtub::SimpleRenderer& renderer, glm::mat4 const& view_matrix, glm::mat4 const& projection_matrix)
{
    // Define geometry for the camera coordinate system
    glm::vec3 lines[] = {
        glm::vec3(0, 0, 0),
        glm::vec3(1, 0, 0),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 1),
    };

    static glm::vec3 colors[] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
    };

    glm::mat4 inv_view_matrix       = glm::inverse(view_matrix);
    glm::mat4 inv_projection_matrix = glm::inverse(projection_matrix);

    // Render the camera coordinate system
    for (glm::vec3& point : lines)
    {
        point = inv_view_matrix * glm::vec4(0.25f * point, 1.f);
    }

    renderer.render_lines(lines, colors);

    bool is_identity = projection_matrix == glm::mat4(1);
    if (!is_identity)
    {
        // Render the view volume
        glm::vec3 frustum[8] = {
            glm::vec3(-1, -1, -1),
            glm::vec3(1, -1, -1),
            glm::vec3(1, 1, -1),
            glm::vec3(-1, 1, -1),
            glm::vec3(-1, -1, 1),
            glm::vec3(1, -1, 1),
            glm::vec3(1, 1, 1),
            glm::vec3(-1, 1, 1)};

        for (glm::vec3& point : frustum)
        {
            glm::vec4 hpoint = inv_view_matrix * inv_projection_matrix * glm::vec4(point, 1.f);
            point            = hpoint / hpoint.w;
        }

        glm::vec3 frustum_line_points[24];
        size_t    idx = 0;
        for (size_t i = 0; i < 4; i++)
        {
            frustum_line_points[idx++] = frustum[i];
            frustum_line_points[idx++] = frustum[(i + 1) % 4];

            frustum_line_points[idx++] = frustum[4 + i];
            frustum_line_points[idx++] = frustum[4 + ((i + 1) % 4)];

            frustum_line_points[idx++] = frustum[i];
            frustum_line_points[idx++] = frustum[i + 4];
        }

        glm::vec3 frustum_line_colors[12];
        for (glm::vec3& c : frustum_line_colors)
        {
            c = glm::vec3(0.f);
        }

        renderer.render_lines(frustum_line_points, frustum_line_colors);

        // Render the lines connecting to the near plane only for a perspective transformation
        bool is_perspective_transformation = (projection_matrix[0][3] != 0) ||
                                             (projection_matrix[1][3] != 0) ||
                                             (projection_matrix[2][3] != 0);
        if (is_perspective_transformation)
        {
            glm::vec3 cameraLocation    = inv_view_matrix * glm::vec4(0, 0, 0, 1);
            glm::vec3 near_line_points[8] = {
                cameraLocation,
                frustum[0],
                cameraLocation,
                frustum[1],
                cameraLocation,
                frustum[2],
                cameraLocation,
                frustum[3],
            };

            glm::vec3 near_line_colors[4] = {
                glm::vec3(0.75f),
                glm::vec3(0.75f),
                glm::vec3(0.75f),
                glm::vec3(0.75f),
            };

            renderer.render_lines(near_line_points, near_line_colors);
        }
    }
}

GuiChanges gui(float* azimuth, float* fov, float* size, float* znear, float* zfar, TransformationType* transformation_type)
{
    GuiChanges changes{0};

    ImGui::Begin("Exercise 2");

    if (ImGui::SliderFloat("Azimuth", azimuth, -glm::two_pi<float>(), glm::two_pi<float>()))
        changes |= 0b000001;

    if (ImGui::Combo("Transformation", reinterpret_cast<int*>(transformation_type), "Orthographic\0Perspective\0"))
        changes |= 0b000010;

    if (*transformation_type == TransformationType::Orthographic)
    {
        if (ImGui::SliderFloat("Size", size, .1f, 2.f))
            changes |= 0b000100;
    }
    else if (ImGui::SliderFloat("FOV", fov, 5.f, 80.f))
        changes |= 0b001000;

    if (ImGui::SliderFloat("Near", znear, 0.1f, *zfar))
        changes |= 0b010000;
    if (ImGui::SliderFloat("Far", zfar, *znear, 20.f))
        changes |= 0b100000;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();

    return changes;
}

bool has_gui_changed_parameter(GuiChanges gui_changes, unsigned int parameter_index)
{
    // TODO: parameter_index must be < 32.

    return static_cast<bool>(gui_changes & (1 << parameter_index));
}

void create_bunny_geometry(std::vector<glm::vec3>* positions, std::vector<glm::u32vec3>* indices)
{
    positions->resize(bunny_positions.size());
    std::copy(bunny_positions.begin(), bunny_positions.end(), positions->begin());

    indices->resize(bunny_indices.size());
    std::copy(bunny_indices.begin(), bunny_indices.end(), indices->begin());
}

} // namespace ex2