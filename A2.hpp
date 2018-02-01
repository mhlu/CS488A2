#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"

#include <glm/glm.hpp>

#include <vector>

// Set a global maximum number of vertices in order to pre-allocate VBO data
// in one shot, rather than reallocating each frame.
const GLsizei kMaxVertices = 1000;


// Convenience class for storing vertex data in CPU memory.
// Data should be copied over to GPU memory via VBO storage before rendering.
class VertexData {
public:
    VertexData();

    std::vector<glm::vec2> positions;
    std::vector<glm::vec3> colours;
    GLuint index;
    GLsizei numVertices;
};


class A2 : public CS488Window {
public:
    A2();
    virtual ~A2();

protected:
    virtual void init() override;
    virtual void appLogic() override;
    virtual void guiLogic() override;
    virtual void draw() override;
    virtual void cleanup() override;

    virtual bool cursorEnterWindowEvent(int entered) override;
    virtual bool mouseMoveEvent(double xPos, double yPos) override;
    virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
    virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
    virtual bool windowResizeEvent(int width, int height) override;
    virtual bool keyInputEvent(int key, int action, int mods) override;

    void createShaderProgram();
    void enableVertexAttribIndices();
    void generateVertexBuffers();
    void mapVboDataToVertexAttributeLocation();
    void uploadVertexDataToVbos();



    void initLineData();

    void setLineColour(const glm::vec3 & colour);

    void drawLine (
            const glm::vec2 & v0,
            const glm::vec2 & v1
    );

    void draw3DLine (
            const glm::vec3 & v0,
            const glm::vec3 & v1,
            const glm::vec3 & colour,
            const glm::mat4 P,
            const glm::mat4 V,
            const glm::mat4 M
    );
    glm::vec3 project_normalize( glm::vec3 v );

    void reset();

    ShaderProgram m_shader;

    GLuint m_vao;            // Vertex Array Object
    GLuint m_vbo_positions;  // Vertex Buffer Object
    GLuint m_vbo_colours;    // Vertex Buffer Object

    VertexData m_vertexData;

    glm::vec3 m_currentLineColour;


    enum INTERACTION_MODE { ROTATE_VIEW, TRANSLATE_VIEW, PERSPECTIVE,
                            ROTATE_MODEL, TRANSLATE_MODEL, SCALE_MODEL,
                            VIEWPORT };
    INTERACTION_MODE m_interaction_mode;
    bool m_left_mouse_key_down;
    bool m_middle_mouse_key_down;
    bool m_right_mouse_key_down;


    double m_fov;
    double m_n;
    double m_f;

    glm::mat4 m_P;
    glm::vec3 m_view_pos;
    glm::mat4 m_view_V;
    glm::mat4 m_model_TR;
    glm::mat4 m_model_S;

    double m_mouse_x;
};
