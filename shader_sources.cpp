#include <cstddef>
#include <dake/gl/shader.hpp>

#include "shader_sources.hpp"


using dake::gl::shader;


// FIXME (this should be shared with render_output.cpp)
enum program_flag_bits {
    BIT_LIGHTING,
    BIT_NORMALS,
    BIT_COLORED,
    BIT_SMOOTH,

    PROGRAM_FLAG_COUNT
};

enum program_flags {
    LIGHTING = 1 << BIT_LIGHTING,
    NORMALS  = 1 << BIT_NORMALS,
    COLORED  = 1 << BIT_COLORED,
    SMOOTH   = 1 << BIT_SMOOTH,

    PROGRAM_COUNT = 1 << PROGRAM_FLAG_COUNT,

    // Not-really-flags™ (may not be used with any other flags but only given
    // alone)

    // Riemann's Neighborhood Graph
    RNG      = 1 << PROGRAM_FLAG_COUNT,
};


const shader_source shader_sources[] = {
    shader_source(shader::VERTEX,

                  "#version 150 core\n"
                  "in vec3 in_position;\n"
                  "in vec3 in_normal;\n"
                  "out vec3 vg_color;\n"
                  "out vec3 vg_normal;\n"
                  "uniform mat4 mv;\n"
                  "void main(void)\n"
                  "{\n"
                  "    gl_Position = mv * vec4(in_position, 1.0);\n"
                  "    vg_color = vec3(1.0, 1.0, 1.0);\n"
                  "    vg_normal = in_normal;\n"
                  "}",

                  0,
                  "trivial vertex shader"),


    shader_source(shader::VERTEX,

                  "#version 150 core\n"
                  "in vec3 in_position;\n"
                  "in vec3 in_normal;\n"
                  "out vec3 vg_color;\n"
                  "out vec3 vg_normal;\n"
                  "uniform mat4 mv;\n"
                  "uniform mat3 nmat;\n"
                  "uniform vec3 light_dir;\n"
                  "void main(void)\n"
                  "{\n"
                  "    gl_Position = mv * vec4(in_position, 1.0);\n"
                  "    vg_color = max(dot(nmat * in_normal, light_dir), 0.0) * vec3(1.0, 1.0, 1.0);\n"
                  "    vg_normal = in_normal;\n"
                  "}",

                  LIGHTING,
                  "lighting vertex shader"),


    shader_source(shader::VERTEX,

                  "#version 150 core\n"
                  "in vec3 in_position;\n"
                  "in vec3 in_normal;\n"
                  "in vec3 in_color;\n"
                  "out vec3 vg_color;\n"
                  "out vec3 vg_normal;\n"
                  "uniform mat4 mv;\n"
                  "void main(void)\n"
                  "{\n"
                  "    gl_Position = mv * vec4(in_position, 1.0);\n"
                  "    vg_color = in_color;\n"
                  "    vg_normal = in_normal;\n"
                  "}",

                  COLORED,
                  "colored vertex shader"),


    shader_source(shader::VERTEX,

                  "#version 150 core\n"
                  "in vec3 in_position;\n"
                  "in vec3 in_normal;\n"
                  "in vec3 in_color;\n"
                  "out vec3 vg_color;\n"
                  "out vec3 vg_normal;\n"
                  "uniform mat4 mv;\n"
                  "uniform mat3 nmat;\n"
                  "uniform vec3 light_dir;\n"
                  "void main(void)\n"
                  "{\n"
                  "    gl_Position = mv * vec4(in_position, 1.0);\n"
                  "    vg_color = max(dot(nmat * in_normal, light_dir), 0.0) * in_color;\n"
                  "    vg_normal = in_normal;\n"
                  "}",

                  LIGHTING | COLORED,
                  "colored lighting vertex shader"),


    shader_source(shader::VERTEX,

                  "#version 150 core\n"
                  "in vec3 in_position;\n"
                  "uniform mat4 mv, proj;\n"
                  "void main(void)\n"
                  "{\n"
                  "    gl_Position = proj * mv * vec4(in_position, 1.0);\n"
                  "}",

                  RNG,
                  "RNG vertex shader"),


    shader_source(shader::GEOMETRY,

                  "#version 150 core\n"
                  "layout(points) in;\n"
                  "layout(points, max_vertices=1) out;\n"
                  "in vec3 vg_color[];\n"
                  "out vec3 gf_color;\n"
                  "uniform mat4 proj;\n"
                  "void main(void)\n"
                  "{\n"
                  "    gf_color = vg_color[0];\n"
                  "    gl_Position = proj * gl_in[0].gl_Position;\n"
                  "    EmitVertex();\n"
                  "    EndPrimitive();\n"
                  "}",

                  0,
                  "trivial geometry shader"),


    shader_source(shader::GEOMETRY,

                  "#version 150 core\n"
                  "layout(points) in;\n"
                  "layout(line_strip, max_vertices=2) out;\n"
                  "in vec3 vg_normal[];\n"
                  "out vec3 gf_color;\n"
                  "uniform mat4 proj;\n"
                  "uniform mat3 nmat;\n"
                  "uniform float normal_scale;\n"
                  "void main(void)\n"
                  "{\n"
                  "    gf_color = abs(vg_normal[0]);\n"
                  "    gl_Position = proj * gl_in[0].gl_Position;\n"
                  "    EmitVertex();\n"
                  "\n"
                  "    gf_color = abs(vg_normal[0]);\n"
                  "    gl_Position = proj * vec4(gl_in[0].gl_Position.xyz + normal_scale * (nmat * vg_normal[0]), 1.0);\n"
                  "    EmitVertex();\n"
                  "\n"
                  "    EndPrimitive();\n"
                  "}",

                  NORMALS,
                  "normals emitting geometry shader"),


    shader_source(shader::FRAGMENT,

                  "#version 150 core\n"
                  "in vec3 gf_color;\n"
                  "out vec4 out_color;\n"
                  "void main(void)\n"
                  "{\n"
                  "    out_color = vec4(gf_color, 1.0);\n"
                  "}",

                  0,
                  "fragment shader"),


    shader_source(shader::FRAGMENT,

                  "#version 150 core\n"
                  "in vec3 gf_color;\n"
                  "out vec4 out_color;\n"
                  "void main(void)\n"
                  "{\n"
                  "    if (length(gl_PointCoord - vec2(0.5, 0.5)) > 0.25) discard;\n"
                  "    out_color = vec4(gf_color, 1.0);\n"
                  "}",

                  SMOOTH,
                  "smooth fragment shader"),


    shader_source(shader::FRAGMENT,

                  "#version 150 core\n"
                  "out vec4 out_color;\n"
                  "void main(void)\n"
                  "{\n"
                  "    out_color = vec4(1.0, 0.25, 0.0, 1.0);\n"
                  "}",

                  RNG,
                  "RNG fragment shader"),
};


const size_t shader_source_count = sizeof(shader_sources) / sizeof(shader_sources[0]);
