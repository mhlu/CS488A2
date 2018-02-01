#include "A2.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
#include <cassert>
using namespace std;

#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
using namespace glm;



bool clipPlane( vec3 &A, vec3 &B, vec3 P, vec3 n) {
  float wecA = dot( (A - P), n );
  float wecB = dot( (B - P), n );

  if ( wecA < 0 && wecB < 0) {
    return false;
  }

  if ( !(wecA >= 0 && wecB >= 0) ) {
    float t = wecA / (wecA - wecB);
    if ( wecA < 0 ) {
      A = A + (B - A) * t;
    } else {
      B = A + (B - A) * t;
    }
  }

  return true;
}


//mat4 perspective_matrix(


//----------------------------------------------------------------------------------------
// Constructor
VertexData::VertexData()
    : numVertices(0),
      index(0)
{
    positions.resize(kMaxVertices);
    colours.resize(kMaxVertices);
}

//TODO fill in my implementation
mat4 my_translate( vec3 v ) {
    mat4 I;
    return glm::translate( I, v );
}

mat4 my_rotate( float angle, char axis ) {
    assert( axis == 'x' or axis == 'y' or axis == 'z' );
    mat4 I;

    if ( axis == 'x' )
        return glm::rotate( I, angle, vec3(1.0, 0.0, 0.0) );

    if ( axis == 'y' )
        return glm::rotate( I, angle, vec3(0.0, 1.0, 0.0) );

    if ( axis == 'z' )
        return glm::rotate( I, angle, vec3(0.0, 0.0, 1.0) );
}

mat4 my_scale( vec3 scale ) {
    mat4 I;
    return glm::scale( I, scale );

}

//----------------------------------------------------------------------------------------
// Constructor
//
static int interaction_radio = 0;

A2::A2()
    : m_currentLineColour(vec3(0.0f)),
    m_left_mouse_key_down( false ),
    m_middle_mouse_key_down( false ),
    m_right_mouse_key_down( false )
{
    reset();
}

//----------------------------------------------------------------------------------------
// Destructor
A2::~A2()
{

}


//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A2::init()
{
    // Set the background colour.
    glClearColor(0.3, 0.5, 0.7, 1.0);

    createShaderProgram();

    glGenVertexArrays(1, &m_vao);

    enableVertexAttribIndices();

    generateVertexBuffers();

    mapVboDataToVertexAttributeLocation();
}

//----------------------------------------------------------------------------------------
void A2::createShaderProgram()
{
    m_shader.generateProgramObject();
    m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
    m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
    m_shader.link();
}

//----------------------------------------------------------------------------------------
void A2::enableVertexAttribIndices()
{
    glBindVertexArray(m_vao);

    // Enable the attribute index location for "position" when rendering.
    GLint positionAttribLocation = m_shader.getAttribLocation( "position" );
    glEnableVertexAttribArray(positionAttribLocation);

    // Enable the attribute index location for "colour" when rendering.
    GLint colourAttribLocation = m_shader.getAttribLocation( "colour" );
    glEnableVertexAttribArray(colourAttribLocation);

    // Restore defaults
    glBindVertexArray(0);

    CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void A2::generateVertexBuffers()
{
    // Generate a vertex buffer to store line vertex positions
    {
        glGenBuffers(1, &m_vbo_positions);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);

        // Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * kMaxVertices, nullptr,
                GL_DYNAMIC_DRAW);


        // Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        CHECK_GL_ERRORS;
    }

    // Generate a vertex buffer to store line colors
    {
        glGenBuffers(1, &m_vbo_colours);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);

        // Set to GL_DYNAMIC_DRAW because the data store will be modified frequently.
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * kMaxVertices, nullptr,
                GL_DYNAMIC_DRAW);


        // Unbind the target GL_ARRAY_BUFFER, now that we are finished using it.
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        CHECK_GL_ERRORS;
    }
}

//----------------------------------------------------------------------------------------
void A2::mapVboDataToVertexAttributeLocation()
{
    // Bind VAO in order to record the data mapping.
    glBindVertexArray(m_vao);

    // Tell GL how to map data from the vertex buffer "m_vbo_positions" into the
    // "position" vertex attribute index for any bound shader program.
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
    GLint positionAttribLocation = m_shader.getAttribLocation( "position" );
    glVertexAttribPointer(positionAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Tell GL how to map data from the vertex buffer "m_vbo_colours" into the
    // "colour" vertex attribute index for any bound shader program.
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
    GLint colorAttribLocation = m_shader.getAttribLocation( "colour" );
    glVertexAttribPointer(colorAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    //-- Unbind target, and restore default values:
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    CHECK_GL_ERRORS;
}

//---------------------------------------------------------------------------------------
void A2::initLineData()
{
    m_vertexData.numVertices = 0;
    m_vertexData.index = 0;
}

//---------------------------------------------------------------------------------------
void A2::setLineColour (
        const glm::vec3 & colour
) {
    m_currentLineColour = colour;
}

//---------------------------------------------------------------------------------------
void A2::drawLine(
        const glm::vec2 & v0,   // Line Start (NDC coordinate)
        const glm::vec2 & v1    // Line End (NDC coordinate)
) {

    m_vertexData.positions[m_vertexData.index] = v0;
    m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
    ++m_vertexData.index;
    m_vertexData.positions[m_vertexData.index] = v1;
    m_vertexData.colours[m_vertexData.index] = m_currentLineColour;
    ++m_vertexData.index;

    m_vertexData.numVertices += 2;
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A2::appLogic()
{
    // Place per frame, application logic here ...

    // Call at the beginning of frame, before drawing lines:
    initLineData();


    // Draw Cube
    static vec3 cube_coords[][2] = {
        { vec3(-1, -1, -1), vec3(1, -1, -1) },
        { vec3(-1, -1, -1), vec3(-1, 1, -1) },
        { vec3(-1, -1, -1), vec3(-1, -1, 1) },

        { vec3(1, 1, 1), vec3(-1, 1, 1) },
        { vec3(1, 1, 1), vec3(1, -1, 1) },
        { vec3(1, 1, 1), vec3(1, 1, -1) },

        { vec3(1, -1, -1), vec3(1, 1, -1) },
        { vec3(1, -1, -1), vec3(1, -1, 1) },

        { vec3(-1, 1, -1), vec3(1, 1, -1) },
        { vec3(-1, 1, -1), vec3(-1, 1, 1) },

        { vec3(-1, -1, 1), vec3(1, -1, 1) },
        { vec3(-1, -1, 1), vec3(-1, 1, 1) }
    };
    vec3 cube_colour(1.0f, 0.7f, 0.8f);


    // draw world axis
    draw3DLine( vec3(0, 0, 0), vec3(0.3, 0, 0), vec3(1, 0, 0), P, m_view_V, mat4() );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0.3, 0), vec3(0, 1, 0), P, m_view_V, mat4() );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0, 0.3), vec3(0, 0, 1), P, m_view_V, mat4() );


    // the cube and its axis
    for ( int i = 0; i < 12; i++ ) {
        draw3DLine( cube_coords[i][0], cube_coords[i][1], cube_colour, P, m_view_V, m_model_TR*m_model_S );
    }
    draw3DLine( vec3(0, 0, 0), vec3(0.3, 0, 0), vec3(1, 0, 0), P, m_view_V, m_model_TR );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0.3, 0), vec3(0, 1, 0), P, m_view_V, m_model_TR );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0, 0.3), vec3(0, 0, 1), P, m_view_V, m_model_TR );


    //// Draw outer square:
    //setLineColour(vec3(1.0f, 0.7f, 0.8f));
    //drawLine(vec2(-0.5f, -0.5f), vec2(0.5f, -0.5f));
    //drawLine(vec2(0.5f, -0.5f), vec2(0.5f, 0.5f))1
    //drawLine(vec2(0.5f, 0.5f), vec2(-0.5f, 0.5f));
    //drawLine(vec2(-0.5f, 0.5f), vec2(-0.5f, -0.5f));


    //// Draw inner square:
    //setLineColour(vec3(0.2f, 1.0f, 1.0f));
    //drawLine(vec2(-0.25f, -0.25f), vec2(0.25f, -0.25f));
    //drawLine(vec2(0.25f, -0.25f), vec2(0.25f, 0.25f));
    //drawLine(vec2(0.25f, 0.25f), vec2(-0.25f, 0.25f));
    //drawLine(vec2(-0.25f, 0.25f), vec2(-0.25f, -0.25f));
}

/* draw the 3D line onto screen after entire pipeline */

void A2::draw3DLine (
        const vec3 & v0, const vec3 & v1, const vec3 & colour,
        mat4 P, mat4 V, mat4 M) {

    vec4 v00 = V * M * vec4(v0, 1);
    vec4 v11 = V * M * vec4(v1, 1);
    vec3 a = vec3( v00 );
    vec3 b = vec3( v11 );

    bool not_clipped = true;
    not_clipped &= clipPlane( a, b, vec3(0.0f, 0.0f, m_n), vec3(0.0, 0.0f, 1.0f) );
    not_clipped &= clipPlane( a, b, vec3(0.0f, 0.0f, m_f), vec3(0.0, 0.0f, -1.0f) );




    vec2 start = vec2( a[0], a[1] );
    vec2 end = vec2( b[0], b[1] );

    start[0] = start[0]/2;
    start[1] = start[1]/2;
    end[0] = end[0]/2;
    end[1] = end[1]/2;


    setLineColour( colour );
    drawLine( start, end );

}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A2::guiLogic()
{
    static bool firstRun(true);
    if (firstRun) {
        ImGui::SetNextWindowPos(ImVec2(50, 50));
        firstRun = false;
    }

    static bool showDebugWindow(true);
    ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
    float opacity(0.5f);

    ImGui::Begin("Properties", &showDebugWindow, ImVec2(100,100), opacity,
            windowFlags);


        if( ImGui::Button( "Quit Application" ) ) { glfwSetWindowShouldClose(m_window, GL_TRUE); }
        if( ImGui::Button( "Reset" ) ) { reset(); }

        if ( ImGui::RadioButton( "Rotate View",      &interaction_radio, 0 ) ) { m_interaction_mode = INTERACTION_MODE::ROTATE_VIEW; }
        if ( ImGui::RadioButton( "Translate View",   &interaction_radio, 1 ) ) { m_interaction_mode = INTERACTION_MODE::TRANSLATE_VIEW; }
        if ( ImGui::RadioButton( "Perspective",      &interaction_radio, 2 ) ) { m_interaction_mode = INTERACTION_MODE::PERSPECTIVE; }
        if ( ImGui::RadioButton( "Rotate Model",     &interaction_radio, 3 ) ) { m_interaction_mode = INTERACTION_MODE::ROTATE_MODEL; }
        if ( ImGui::RadioButton( "Translate Model",  &interaction_radio, 4 ) ) { m_interaction_mode = INTERACTION_MODE::TRANSLATE_MODEL; }
        if ( ImGui::RadioButton( "Scale Model",      &interaction_radio, 5 ) ) { m_interaction_mode = INTERACTION_MODE::SCALE_MODEL; }
        if ( ImGui::RadioButton( "Viewport",         &interaction_radio, 6 ) ) { m_interaction_mode = INTERACTION_MODE::VIEWPORT; }

        ImGui::Text( "near: %.1f, far: %.1f, fov: %.1f", m_n, m_f, m_fov );
        ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

    ImGui::End();
}

//----------------------------------------------------------------------------------------
void A2::uploadVertexDataToVbos() {

    //-- Copy vertex position data into VBO, m_vbo_positions:
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_positions);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec2) * m_vertexData.numVertices,
                m_vertexData.positions.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        CHECK_GL_ERRORS;
    }

    //-- Copy vertex colour data into VBO, m_vbo_colours:
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo_colours);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * m_vertexData.numVertices,
                m_vertexData.colours.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        CHECK_GL_ERRORS;
    }
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A2::draw()
{
    uploadVertexDataToVbos();

    glBindVertexArray(m_vao);

    m_shader.enable();
        glDrawArrays(GL_LINES, 0, m_vertexData.numVertices);
    m_shader.disable();

    // Restore defaults
    glBindVertexArray(0);

    CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A2::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A2::cursorEnterWindowEvent (
        int entered
) {
    bool eventHandled(false);

    // Fill in with event handling code...

    return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A2::mouseMoveEvent (
        double xPos,
        double yPos
) {
    bool eventHandled(false);

    int delta_x = xPos - m_mouse_x;
    if (!ImGui::IsMouseHoveringAnyWindow()) {

        if ( m_interaction_mode == INTERACTION_MODE::ROTATE_VIEW ) {
            float rot = delta_x / 100.0f;
            char axis = '\0';
            if ( m_left_mouse_key_down ) { axis = 'x'; }
            if ( m_middle_mouse_key_down ) { axis = 'y'; }
            if ( m_right_mouse_key_down ) { axis = 'z'; }

            if ( axis == 'x' || axis == 'y' || axis == 'z' ) {
                m_view_V *= my_translate( m_view_pos );
                m_view_V = my_rotate( -rot, axis ) * m_view_V;
                m_view_V *= my_translate( -m_view_pos );
            }
        }

        if ( m_interaction_mode == INTERACTION_MODE::TRANSLATE_VIEW ) {
            float delta = delta_x / 100.0f;
            vec3 trans(0,0,0);

            if ( m_left_mouse_key_down ) {      trans[0] = delta; }
            if ( m_middle_mouse_key_down ) {    trans[1] = delta; }
            if ( m_right_mouse_key_down ) {     trans[2] = delta; }

            m_view_pos += trans;
            m_view_V *= my_translate( -trans );
        }

        if ( m_interaction_mode == INTERACTION_MODE::PERSPECTIVE ) {
            if ( m_left_mouse_key_down )    m_fov += delta_x / 100.0f;
            if ( m_middle_mouse_key_down )  m_n += delta_x / 100.0f;
            if ( m_right_mouse_key_down )   m_f += delta_x / 100.0f;
        }

        if ( m_interaction_mode == INTERACTION_MODE::ROTATE_MODEL ) {
            float rot = delta_x / 100.0f;
            if ( m_left_mouse_key_down ) {      m_model_TR *= my_rotate( rot, 'x' ); }
            if ( m_middle_mouse_key_down ) {    m_model_TR *= my_rotate( rot, 'y' ); }
            if ( m_right_mouse_key_down ) {     m_model_TR *= my_rotate( rot, 'z' ); }
        }

        if ( m_interaction_mode == INTERACTION_MODE::TRANSLATE_MODEL ) {
            float trans = delta_x / 100.0f;
            if ( m_left_mouse_key_down ) {      m_model_TR *= my_translate( vec3( trans, 0, 0 ) ); }
            if ( m_middle_mouse_key_down ) {    m_model_TR *= my_translate( vec3( 0, trans, 0 ) ); }
            if ( m_right_mouse_key_down ) {     m_model_TR *= my_translate( vec3( 0, 0, trans ) ); }
        }

        if ( m_interaction_mode == INTERACTION_MODE::SCALE_MODEL ) {
            float scale = 1 + delta_x / 100.0f;
            if ( m_left_mouse_key_down ) {      m_model_S *= my_scale( vec3( scale, 1, 1 ) ); }
            if ( m_middle_mouse_key_down ) {    m_model_S *= my_scale( vec3( 1, scale, 1 ) ); }
            if ( m_right_mouse_key_down ) {     m_model_S *= my_scale( vec3( 1, 1, scale ) ); }
        }

        m_mouse_x = xPos;
        eventHandled = true;
    }

    return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A2::mouseButtonInputEvent (
        int button,
        int actions,
        int mods
) {
    bool eventHandled(false);

    if (!ImGui::IsMouseHoveringAnyWindow()) {

        if ( button == GLFW_MOUSE_BUTTON_LEFT && actions == GLFW_PRESS ) {
            m_left_mouse_key_down = true;
            eventHandled = true;
        }

        if ( button == GLFW_MOUSE_BUTTON_LEFT && actions == GLFW_RELEASE ) {
            m_left_mouse_key_down = false;
            eventHandled = true;
        }

        if ( button == GLFW_MOUSE_BUTTON_MIDDLE && actions == GLFW_PRESS ) {
            m_middle_mouse_key_down = true;
            eventHandled = true;
        }

        if ( button == GLFW_MOUSE_BUTTON_MIDDLE && actions == GLFW_RELEASE ) {
            m_middle_mouse_key_down = false;
            eventHandled = true;
        }

        if ( button == GLFW_MOUSE_BUTTON_RIGHT && actions == GLFW_PRESS ) {
            m_right_mouse_key_down = true;
            eventHandled = true;
        }

        if ( button == GLFW_MOUSE_BUTTON_RIGHT && actions == GLFW_RELEASE ) {
            m_right_mouse_key_down = false;
            eventHandled = true;
        }
    }

    return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A2::mouseScrollEvent (
        double xOffSet,
        double yOffSet
) {
    bool eventHandled(false);

    // Fill in with event handling code...

    return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A2::windowResizeEvent (
        int width,
        int height
) {
    bool eventHandled(false);

    // Fill in with event handling code...

    return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A2::keyInputEvent (
        int key,
        int action,
        int mods
) {
    bool eventHandled(false);

    if( action == GLFW_PRESS ) {

        if ( key == GLFW_KEY_Q ) {
            glfwSetWindowShouldClose(m_window, GL_TRUE);
            eventHandled = true;
        }

        if ( key == GLFW_KEY_A ) {
            // reset();
            eventHandled = true;
        }

        if ( key == GLFW_KEY_O ) {
            m_interaction_mode = INTERACTION_MODE::ROTATE_VIEW;
            interaction_radio = 0;
            eventHandled = true;
        }
        if ( key == GLFW_KEY_N ) {
            m_interaction_mode = INTERACTION_MODE::TRANSLATE_VIEW;
            interaction_radio = 1;
            eventHandled = true;
        }
        if ( key == GLFW_KEY_P ) {
            m_interaction_mode = INTERACTION_MODE::PERSPECTIVE;
            interaction_radio = 2;
            eventHandled = true;
        }
        if ( key == GLFW_KEY_R ) {
            m_interaction_mode = INTERACTION_MODE::ROTATE_MODEL;
            interaction_radio = 3;
            eventHandled = true;
        }
        if ( key == GLFW_KEY_T ) {
            m_interaction_mode = INTERACTION_MODE::TRANSLATE_MODEL;
            interaction_radio = 4;
            eventHandled = true;
        }
        if ( key == GLFW_KEY_S ) {
            m_interaction_mode = INTERACTION_MODE::SCALE_MODEL;
            interaction_radio = 5;
            eventHandled = true;
        }
        if ( key == GLFW_KEY_V ) {
            m_interaction_mode = INTERACTION_MODE::VIEWPORT;
            interaction_radio = 6;
            eventHandled = true;
        }

    }

    return eventHandled;
}

void A2::reset() {

    interaction_radio = 3;
    m_interaction_mode =  INTERACTION_MODE::ROTATE_MODEL;

    m_model_TR = mat4();
    m_model_S = mat4();

    m_view_pos = vec3(0.0, 0.0, 6.0);
    m_view_V = mat4();
    m_view_V *= my_translate(-m_view_pos);

    m_fov = 30;
    m_n = 2;
    m_f = 20;

}
