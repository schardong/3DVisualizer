#include "glslrenderer.h"
#include "singleton.h"
#include "tinygl.h"
#include "topanalyzer.h"
#include "alphamanager.h"
#include "datasetmanager.h"
#include "tfmanager.h"
#include "config.h"

GLSLRenderer::GLSLRenderer(size_t w, size_t h) :
  Renderer(w, h)
{
  m_type = RendererType::GLSL;
  Initialized(false);
  Updating(false);

#ifdef WIN32
  m_arcball = new Arcball(GetWidth(), GetHeight(), 0.8);
#else
  m_arcball = new Arcball(GetWidth(), GetHeight(), 0.05);
#endif
}

GLSLRenderer::~GLSLRenderer()
{
  Updating(false);
  Initialized(false);

  delete m_arcball;
  m_arcball = nullptr;
}

int GLSLRenderer::init()
{
  m_eye = glm::vec3(0, 0, 3.5);
  m_viewMatrix = glm::lookAt(m_eye, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
  m_projMatrix = glm::ortho(-0.8f, 0.8f, -0.8f, 0.8f, 1.f, 5.f);
  Initialized(true);
  return 0;
}

void GLSLRenderer::update()
{
  if(!IsUpdating() || !IsInitialized())
    return;
}

void GLSLRenderer::resize(int w, int h)
{}

void GLSLRenderer::keypress(int key, int press)
{}

void GLSLRenderer::mousebutton(int button, int pressed, int, int)
{
  m_arcball->mouseButtonCallback(button, pressed);
}

void GLSLRenderer::mousemove(int x, int y)
{
  m_arcball->cursorCallback(static_cast<double>(x), static_cast<double>(y));
}

void GLSLRenderer::mousewheel(float, int, int)
{}

void GLSLRenderer::draw()
{
  if(!IsUpdating() || !IsInitialized())
    return;

  TinyGL* gl_ptr = TinyGL::GetInstance();
  Mesh* m = gl_ptr->getMesh("proxy_cube");
  Shader* fpass = gl_ptr->getShader(FPASS_KEY);
  Shader* spass = gl_ptr->getShader(SPASS_KEY);
  FramebufferObject* fbo = gl_ptr->getFBO(FBO_KEY);

  if(!fpass || !spass || !fbo || !m) return;

  fpass->bind();
  glm::mat4 rot = m_viewMatrix * m_arcball->createViewRotationMatrix();
  fpass->setUniformMatrix("u_mView", rot);

  //First pass
  fbo->bind(GL_FRAMEBUFFER);
  glViewport(0, 0, GetWidth(), GetHeight());
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glCullFace(GL_FRONT);
  m->draw();

  FramebufferObject::unbind();
  glViewport(0, 0, GetWidth(), GetHeight());

  //Second pass
  spass->bind();
  spass->setUniformMatrix("u_mView", rot);

  DatasetManager::GetInstance()->GetCurrent()->bind(GL_TEXTURE1);
  AlphaManager::GetInstance()->GetCurrent()->bind(GL_TEXTURE2);
  TFManager::GetInstance()->GetCurrent()->bind(GL_TEXTURE3);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, fbo->getAttachmentId(GL_COLOR_ATTACHMENT0));

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glCullFace(GL_BACK);
  m->draw();
  Shader::Unbind();
}
