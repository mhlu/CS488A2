#include "A2.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
#include <cassert>
using namespace std;

#include <imgui/imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include <cmath>
using namespace glm;

double clip_float( double v, double min, double max ) {
    assert( min <= max );
    if ( v < min ) v = min;
    if ( v > max ) v = max;
}

bool clipAgainstPlane( vec3 &A, vec3 &B, vec3 P, vec3 n) {

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

bool clip( vec4 &A, vec4 &B) {

  float AA[6] = {A[0], -A[0], A[1], -A[1], A[2], -A[2]};
  float BB[6] = {B[0], -B[0], B[1], -B[1], B[2], -B[2]};


  // the algo only works for w>0
  if(A[3] < 1e-5 && B[3] < 1e-5 ) {

    return true;

  } else if(A[3] < 1e-5) {

    float t = (B[3] - 1e-5) / (B[3] - A[3]);
    A = B + t*(A-B);

  } else if(B[3] < 1e-5) {

    float t = (A[3] - 1e-5) / (A[3] - B[3]);
    B = A + t*(B-A);

  }

  for(uint33_t i = 0; i < 3; i++) {

    float BL1 = A[3] + A[i];
    float BL2 = B[3] + B[i];
    if( BL1 >= 0 && BL2 >= 0) {
        continue;
    }

    if( BL1 < 0 && BL2 < 0) {
        return true;
    }

    float a =  BL1 / ( BL1 - BL2);

    if( BL1  < 0) {
      A = A + a*(B-A);

    } else {
      B = A + a*(B-A);

    }

  }

  for(int i = 0; i < 3; i++) {

    float BL1 = A[3] - A[i];
    float BL2 = B[3] - B[i];
    if( BL1 >= 0 && BL2 >= 0) {
        continue;
    }

    if( BL1 < 0 && BL2 < 0) {
        return true;
    }

    float a =  BL1 / ( BL1 - BL2);

    if( BL1  < 0) {
      A = A + a*(B-A);

    } else {
      B = A + a*(B-A);

    }

  }

  return false;
}


void printMatrix( mat4 M ) {
    for ( int i=0; i<4; i++ ) {
        for ( int j=0; j<4; j++ )
            cout<<M[i][j]<<" ";
        cout<<endl;
    }
    cout<<'\n'*5<<endl;
}

void screen_to_NDC( int x, int y, int W, int H, double &a, double &b ) {
    if ( x < 0 ) x = 0;
    if ( x > W ) x = W;
    if ( y < 0 ) y = 0;
    if ( y > H ) y = H;
    y = H - y;

    a = double(x)/W*2 - 1;
    b = double(y)/H*2 - 1;
}

mat4 perspective_matrix( double fov, double n, double f ) {
    float c = 1.0f / tan( (fov*(2*M_PI/360.0f)) / 2.0f );
    mat4 P(
        c, 0.0f, 0.0f, 0.0f,
        0.0f, c, 0.0f, 0.0f,
        0.0f, 0.0f, -(n+f)/(f-n), -1.0f,
        0.0f, 0.0f, -2*f*n/(f-n), 0.0f
    );
    printMatrix(P);
    return P;
}

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


    // draw port axis
    glClearColor(0.0, 0.5, 0.5, 1.0);
    draw3DLine( vec3(0, 0, 0), vec3(0.3, 0, 0), vec3(1, 0, 0), m_view_V );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0.3, 0), vec3(0, 1, 0), m_view_V );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0, 0.3), vec3(0, 0, 1), m_view_V );


    // the cube and its axis
    for ( int i = 0; i < 12; i++ ) {
        draw3DLine( cube_coords[i][0], cube_coords[i][1], cube_colour, m_view_V * m_model_TR * m_model_S );
    }
    draw3DLine( vec3(0, 0, 0), vec3(0.3, 0, 0), vec3(1, 0, 0), m_view_V * m_model_TR );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0.3, 0), vec3(0, 1, 0), m_view_V * m_model_TR );
    draw3DLine( vec3(0, 0, 0), vec3(0, 0, 0.3), vec3(0, 0, 1), m_view_V * m_model_TR );

    drawLine( vec2(m_port_x, m_port_y),           vec2(m_port_x+m_port_w, m_port_y) );
    drawLine( vec2(m_port_x, m_port_y),           vec2(m_port_x, m_port_y+m_port_h) );
    drawLine( vec2(m_port_x+m_port_w, m_port_y),  vec2(m_port_x+m_port_w, m_port_y+m_port_h) );
    drawLine( vec2(m_port_x, m_port_y+m_port_h),  vec2(m_port_x+m_port_w, m_port_y+m_port_h) );

}

/* draw the 3D line onto screen after entire pipeline */

void A2::draw3DLine ( const vec3 & v0, const vec3 & v1, const vec3 & colour, mat4 transform) {

    vec4 P = m_P * transform * vec4( v0, 1.0f );
    vec4 Q = m_P * transform * vec4( v1, 1.0f );

    //P =  clipPlane( vec3 &A, vec3 &B, vec3 P, vec3 n);
    //Q =  clipPlane( vec3 &A, vec3 &B, vec3 P, vec3 n);


    bool reject = clip( P, Q );

    if ( reject )
        return;

    P = P / P[3];
    Q = Q / Q[3];

    to_view( P, Q );

    setLineColour( colour );
    drawLine( vec2( P[0], P[1] ), vec2( Q[0], Q[1] ) );


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
            if ( m_left_mouse_key_down ) {
                m_fov += delta_x / 10.0f;
                m_fov = clip_float( m_fov, 10, 50 );
                m_P = perspective_matrix( m_fov, m_n, m_f );
            }
            if ( m_middle_mouse_key_down ) {
                m_n += delta_x / 100.0f;
                m_n = clip_float( m_n, 1e-5, m_f );
                m_P = perspective_matrix( m_fov, m_n, m_f );
            }
            if ( m_right_mouse_key_down ) {
                m_f += delta_x / 100.0f;
                m_f = clip_float( m_f, m_n+1e-5, 100 );
                m_P = perspective_matrix( m_fov, m_n, m_f );
            }
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


        if ( m_interaction_mode == INTERACTION_MODE::VIEWPORT_CLICKED ) {

            int width, height;
            glfwGetWindowSize(m_window, &width, &height);

            double nx, ny;
            screen_to_NDC( xPos, yPos, width, height, nx, ny );

            m_port_x = nx < m_port_init_x ? nx : m_port_init_x;
            m_port_y = ny < m_port_init_y ? ny : m_port_init_y;

            m_port_w  = nx - m_port_init_x;
            m_port_h = ny - m_port_init_y;

            m_port_w = m_port_w > 0 ? m_port_w : -m_port_w;
            m_port_h = m_port_h > 0 ? m_port_h : -m_port_h;

        }

        m_mouse_x = xPos;
        m_mouse_y = yPos;
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

    if ( button == GLFW_MOUSE_BUTTON_LEFT && actions == GLFW_PRESS ) {
        m_left_mouse_key_down = true;

        if ( m_interaction_mode == INTERACTION_MODE::VIEWPORT ) {
            int width, height;
            glfwGetWindowSize(m_window, &width, &height);
            screen_to_NDC( m_mouse_x, m_mouse_y, width, height, m_port_init_x, m_port_init_y );

            m_interaction_mode = INTERACTION_MODE::VIEWPORT_CLICKED;
        }

        else if ( m_interaction_mode == INTERACTION_MODE::VIEWPORT_CLICKED ) {
            m_interaction_mode = INTERACTION_MODE::VIEWPORT;
        }

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
             reset();
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

void A2::to_view( vec4 &P, vec4 &Q ) {

    float port_center_x = m_port_x+m_port_w/2;
    float port_center_y = m_port_y+m_port_h/2;

    mat4 T = my_translate( vec3(port_center_x, port_center_y, 0) );
    mat4 S = my_scale( vec3( m_port_w/2.0f, m_port_h/2.0f, 1) );

    P = T * S * P;
    Q = T * S * Q;
}

void A2::reset() {

    m_port_x = -0.9;
    m_port_y = -0.9;
    m_port_w = 1.8;
    m_port_h = 1.8;

    interaction_radio = 3;
    m_interaction_mode =  INTERACTION_MODE::ROTATE_MODEL;

    m_model_TR = mat4();
    m_model_S = mat4();

    m_view_pos = vec3(0.0, 0.0, 10.0);
    m_view_V = mat4();
    m_view_V *= my_translate(-m_view_pos);

    m_fov = 30;
    m_n = 2;
    m_f = 20;
    m_P = perspective_matrix( m_fov, m_n, m_f );

}

vec3 A2::project_normalize( vec3 v ) {
    vec4 res( m_P * vec4(v, 1) );
    printMatrix( m_P );
    return vec3(
        res[ 0 ] /= res[3],
        res[ 1 ] /= res[3],
        res[ 2 ] /= res[3]
        );
}


