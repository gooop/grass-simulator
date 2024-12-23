#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <random>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdlib>
#include <cstdio>

#include <deque>
#include <string>
#include <vector>
#include <fstream>

#include <CSCI441/OpenGLUtils.hpp>
#include <CSCI441/ShaderProgram.hpp>

// This class stores an SSBO and binds it to an index.
class SSBO {
  public:

    SSBO(GLuint location) {
      glGenBuffers(1, &buffer);
      index = location; 
    }

    void sendData(std::vector<glm::vec4> &data) {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
      glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * data.size(), data.data(), GL_DYNAMIC_COPY);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer);
    }
  private:
    GLuint buffer;
    GLuint index;
};

// This creates all the grass ssbo's with initial values for things like v1 and v2 initial.
std::vector<SSBO> bindGrassSSBOs(std::vector<glm::vec4> v0s, std::vector<glm::vec4> v2s, std::vector<glm::vec4> fronts, std::vector<glm::vec4> lengths) {
  std::vector<SSBO> result;
  std::vector<glm::vec4> ns;
  for (int i = 0; i < v0s.size(); i++) {
    ns.push_back({0.0f, 0.0f, 0.0f, 1.0f});
  }

  for (GLuint i = 10; i < 17; i++) {
    SSBO ssbo(i);
    result.push_back(ssbo);

    switch (i) {
      case 10:
        ssbo.sendData(v0s);
        break;
      case 11:
        ssbo.sendData(v2s);
        break;
      case 12:
        ssbo.sendData(v2s);
        break;
      case 13:
        ssbo.sendData(v2s);
        break;
      case 14:
        ssbo.sendData(fronts);
        break;
      case 15:
        ssbo.sendData(ns);
        break;
      case 16:
        ssbo.sendData(lengths);
        break;
    }
  }
  return result;
}

// This function generates a large block of grass.
std::vector<SSBO> simpleGrassSSBOs(int num, float max, float min) {
  std::vector<glm::vec4> v0s;
  std::vector<glm::vec4> v2s;
  std::vector<glm::vec4> hs;
  std::vector<glm::vec4> fronts;
  std::default_random_engine generator;
  std::uniform_real_distribution<float> distribution(0.7, 1);
  std::uniform_real_distribution<float> xz(-0.1, 0.1);
  auto height = std::bind ( distribution, generator );
  auto offset = std::bind ( xz, generator );

  for (int i = 0; i < num; i++) {
    for (int j = 0; j < num; j++) {
      float h = height();
      float di = i / (float)num;
      float dj = j / (float)num;
      float mm = max - min;
      auto xoffset = offset();
      auto zoffset = offset();
      glm::vec4 v0 {min + di * mm + xoffset, 0, min + dj * mm + zoffset, 1};
      glm::vec4 v2 {min + di * mm + 0.1 + xoffset, h, min + dj * mm + 0.1 + zoffset, 1};
      glm::vec4 front {0, 0, 1, 0};

      v0s.push_back(v0);
      v2s.push_back(v2);
      hs.push_back({h, 0.0f, 0.0f, 1.0f});
      fronts.push_back(front);
    }
  }

  return bindGrassSSBOs(v0s, v2s, fronts, hs);
}

class GrassShader {
  public:
    GrassShader() {
      tessProgram = new CSCI441::ShaderProgram( "shaders/bezierPatch.v.glsl", "shaders/bezierPatch.tc.glsl", "shaders/bezierPatch.te.glsl", "shaders/bezierPatch.f.glsl" );
      mvpUniform = tessProgram->getUniformLocation("mvpMtx");
      glGenVertexArrays(1, &vao);
      glBindVertexArray(vao);

      ssbos = simpleGrassSSBOs(200, 20, -10);

      shader = CSCI441_INTERNAL::ShaderUtils::compileShader("shaders/physicalGrass.glsl", GL_COMPUTE_SHADER);
      printf("Compute shader had ID: %d\n", shader);
      program = glCreateProgram();
      glAttachShader(program, shader);
      glLinkProgram(program);
      CSCI441_INTERNAL::ShaderUtils::printLog(program);

      time = glGetUniformLocation(program, "t");
      deltat = glGetUniformLocation(program, "dt");
    };
    
    void run(float t, float dt, glm::mat4 mvp) {
      tessProgram->useProgram();
      glUniformMatrix4fv(mvpUniform, 1, false, &mvp[0][0]);
      glBindVertexArray(vao);
      glDrawArrays( GL_PATCHES, 0, 400*400*3);

      glUseProgram(program);

      glUniform1f(time, t);
      glUniform1f(deltat, dt);
      
      glDispatchCompute(400, 400, 1);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
  private:
    CSCI441::ShaderProgram *tessProgram;
    GLuint vao;
    GLuint program;
    GLuint shader;
    GLuint time;
    GLuint deltat;
    GLuint mvpUniform;
    std::vector<SSBO> ssbos;
};
