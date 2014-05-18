#ifndef SHADER_SOURCES_HPP
#define SHADER_SOURCES_HPP

#include <dake/gl/shader.hpp>


struct shader_source {
    shader_source(dake::gl::shader::type t, const char *s, int f, const char *d): type(t), source(s), flags(f), description(d)
    {}

    operator const char *(void) const
    { return source; }

    operator dake::gl::shader::type(void) const
    { return type; }

    dake::gl::shader::type type;
    const char *source;
    int flags;
    const char *description;
};


extern const shader_source shader_sources[];
extern const size_t shader_source_count;

#endif
