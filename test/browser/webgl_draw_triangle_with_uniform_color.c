/*
 * Copyright 2018 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

GLuint compile_shader(GLenum shaderType, const char *src) {
  GLuint shader = glCreateShader(shaderType);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);

  GLint isCompiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if (!isCompiled)
  {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
    char *buf = (char*)malloc(maxLength+1);
    glGetShaderInfoLog(shader, maxLength, &maxLength, buf);
    printf("%s\n", buf);
    free(buf);
    return 0;
  }

  return shader;
}

GLuint create_program(GLuint vertexShader, GLuint fragmentShader) {
  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glBindAttribLocation(program, 0, "apos");
  glBindAttribLocation(program, 1, "acolor");
  glLinkProgram(program);
  return program;
}

int main() {
  EmscriptenWebGLContextAttributes attr;
  emscripten_webgl_init_context_attributes(&attr);
#ifdef EXPLICIT_SWAP
  attr.explicitSwapControl = 1;
#endif
#ifdef DRAW_FROM_CLIENT_MEMORY
  // This test verifies that drawing from client-side memory when enableExtensionsByDefault==false works.
  attr.enableExtensionsByDefault = 0;
#endif

  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
  emscripten_webgl_make_context_current(ctx);

  static const char vertex_shader[] =
    "attribute vec4 apos;"
    "attribute vec4 acolor;"
    "varying vec4 color;"
    "void main() {"
      "color = acolor;"
      "gl_Position = apos;"
    "}";
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);

  static const char fragment_shader[] =
    "precision lowp float;"
    "varying vec4 color;"
    "uniform vec4 color2;"
    "void main() {"
      "gl_FragColor = color*color2;"
    "}";
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

  GLuint program = create_program(vs, fs);
  glUseProgram(program);

  static const float pos_and_color[] = {
  //     x,     y, r, g, b
     -0.6f, -0.6f, 1, 0, 0,
      0.6f, -0.6f, 0, 1, 0,
      0.f,   0.6f, 0, 0, 1,
  };

#ifdef DRAW_FROM_CLIENT_MEMORY
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, pos_and_color);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 20, (void*)(pos_and_color+2));
#else
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(pos_and_color), pos_and_color, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, 0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 20, (void*)8);
#endif
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // Spot-test for miniTempWebGLFloatBuffers optimization in library_webgl.js.
  // Note there WILL be GL errors from this, but the test doesn't check for them
  // so it won't cause it to fail.
#define GL_POOL_TEMP_BUFFERS_SIZE 288
  {
    GLfloat fdata[GL_POOL_TEMP_BUFFERS_SIZE + 4] = {};
    // Just under the optimization limit (should use unoptimized codepath)
    glUniform4fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 4 - 1, fdata);
    glUniform2fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 2 - 1, fdata);
    glUniform1fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE - 1, fdata);
    // Just at the optimization limit (should use optimized codepath)
    glUniform4fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 4, fdata);
    glUniform2fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 2, fdata);
    glUniform1fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE, fdata);
    // Just over the optimization limit (should use optimized codepath)
    glUniform4fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 4 + 1, fdata);
    glUniform2fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 2 + 1, fdata);
    glUniform1fv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE + 1, fdata);

    GLint idata[GL_POOL_TEMP_BUFFERS_SIZE + 4] = {};
    // Just under the optimization limit (should use unoptimized codepath)
    glUniform4iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 4 - 1, idata);
    glUniform2iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 2 - 1, idata);
    glUniform1iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE - 1, idata);
    // Just at the optimization limit (should use optimized codepath)
    glUniform4iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 4, idata);
    glUniform2iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 2, idata);
    glUniform1iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE, idata);
    // Just over the optimization limit (should use optimized codepath)
    glUniform4iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 4 + 1, idata);
    glUniform2iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE / 2 + 1, idata);
    glUniform1iv(glGetUniformLocation(program, "color2"), GL_POOL_TEMP_BUFFERS_SIZE + 1, idata);
  }

  // Actual upload for the rest of the test (overwrites the previous one).
  float color2[4] = { 0.0f, 1.f, 0.0f, 1.0f };
  glUniform4fv(glGetUniformLocation(program, "color2"), 1, color2);

  // Test that passing zero for the size paramater does not cause error
  // https://github.com/emscripten-core/emscripten/issues/21567
  // (These are zero-sized, so shouldn't overwrite anything.)
  {
    GLfloat fdata[4] = {};
    glUniform4fv(glGetUniformLocation(program, "color2"), 0, fdata);
    glUniform4fv(glGetUniformLocation(program, "color2"), 0, NULL);

    GLint idata[4] = {};
    glUniform4iv(glGetUniformLocation(program, "color2"), 0, idata);
    glUniform4iv(glGetUniformLocation(program, "color2"), 0, NULL);
  }

  glClearColor(0.3f,0.3f,0.3f,1);
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLES, 0, 3);

#ifdef EXPLICIT_SWAP
  emscripten_webgl_commit_frame();
#endif

  return 0;
}
