#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "la.h"
#include "glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include "defs.h"

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

int main(int argc, char** argv) {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "textedit", nullptr, nullptr);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader text_shader("/Users/liz3/Projects/glfw/simple.vs", "/Users/liz3/Projects/glfw/simple.fs", {});
    text_shader.use();

    // glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WIDTH), 0.0f, static_cast<float>(HEIGHT));
    // glUniformMatrix4fv(glGetUniformLocation(text_shader.pid, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        std::cout << text_shader.pid << "\n";
    FontAtlas atlas("/Users/liz3/Downloads/Fira_Code_v5.2/ttf/FiraCode-Regular.ttf", 64);

    State state;
    while (!glfwWindowShouldClose(window))
    {
      if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
      }

      // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      // glClear(GL_COLOR_BUFFER_BIT);
      text_shader.use();
      glActiveTexture(GL_TEXTURE0);
      glBindVertexArray(state.vao);
      glBindTexture(GL_TEXTURE_2D, atlas.texture_id);
      glBindBuffer(GL_ARRAY_BUFFER, state.vbo);


      float xpos = 15;
      float ypos = HEIGHT -atlas.atlas_height-15;
      std::string text = "Hello Yoshi";
      std::vector<RenderChar> entries;
      std::string::const_iterator c;
       for (c = text.begin(); c != text.end(); c++) {
         entries.push_back(atlas.render(*c, xpos, -1));
         xpos += atlas.getAdvance(*c);
       }

//      glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(RenderChar) *entries.size(), &entries[0]); // be sure to use glBufferSubData and not glBufferData

      glBindBuffer(GL_ARRAY_BUFFER, 0);

      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 8, (GLsizei) entries.size());
glBindTexture(GL_TEXTURE_2D, 0);
       glBindVertexArray(0);



      glfwSwapBuffers(window);
      glfwPollEvents();

    }
    glfwTerminate();
  return 0;
};
