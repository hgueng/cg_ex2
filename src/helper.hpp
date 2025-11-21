#pragma once

// #include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cgtub/simple_renderer.hpp>

namespace ex2
{

enum class TransformationType
{
    Orthographic = 0,
    Perspective
};

using GuiChanges = int;

/**
 * \brief Update the Graphical User Interface (GUI) and retrieve new values for the parameters.
 * 
 * All parameters are input/output, meaning their value will be used to display the GUI and
 * they will be set to the new value, as implied by user interaction with the GUI (in the previous frame).
 * 
 * \return Object that tracks changes to the parameters.
 */
GuiChanges gui(float* azimuth, float* fov, float* size, float* znear, float* zfar, TransformationType* transformation_type);

/**
 * \brief Query if an interaction with the GUI has changed a parameter value.
 * 
 * Example usage:
 * \code{.cpp}
 * int   foo = ...;
 * float bar = ...;
 *
 * GuiChanges gui_changes = gui(&foo, &bar);
 *  
 * if(has_gui_changed_parameter(gui_changes, 0))
 * {
 *     // Parameter `foo` was changed  
 * }
 * 
 * if(has_gui_changed_parameter(gui_changes, 1))
 * {
 *     // Parameter `bar` was changed  
 * }
 * \endcode
 * 
 * \param[in] changes         The \c GuiChanges returned by a call to \c gui(...)
 * \param[in] parameter_index The 0-based index of the parameter to query for changes
 */
bool has_gui_changed_parameter(GuiChanges changes, unsigned int parameter_index);

/**
 * \brief Render a visualization of the camera defined by a view matrix and a projection matrix.
 * 
 * \param[in] renderer          The renderer which will be used
 * \param[in] view_matrix       The view matrix determining the position and orientation of the camera
 * \param[in] projection_matrix The projection matrix of the camera
 */
void render_camera(cgtub::SimpleRenderer& renderer, glm::mat4 const& view_matrix, glm::mat4 const& projection_matrix);

/**
 * \brief Generates the geometry of the stanford bunny scaled to a unit bounding box, filling in vertex positions and indices.
 *
 * \param[in, out] positions A pointer to a vector of glm::vec3 that will be filled with the bunny's vertex positions.
 * \param[in, out] indices   A pointer to a vector of glm::u32vec3 that will be filled with the indices of the bunny's triangular faces.
 */
void create_bunny_geometry(std::vector<glm::vec3>* positions, std::vector<glm::u32vec3>* indices);

} // namespace ex2