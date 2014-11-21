#include "viswidget.h"
#include "singleton.h"
#include "tinygl.h"
#include "datasetmanager.h"
#include "tfmanager.h"
#include "topanalyzer.h"
#include "alphamanager.h"
#include "renderermanager.h"
#include "glslrenderer.h"
#include "config.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWheelEvent>


VisWidget::VisWidget(const QGLFormat& format, QWidget *parent) :
  QGLWidget(format, parent),
  m_numSamples(512),
  m_flowRate(450.f)
{
  m_timer = new QTimer(this);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
  initialized(false);
  data_loaded(false);
  metafile_loaded(false);
}

VisWidget::~VisWidget()
{
  m_timer->stop();
  delete m_timer;
  initialized(false);
  data_loaded(false);
  metafile_loaded(false);
}

void VisWidget::startTimer(int msec)
{
  m_timer->start(msec);
}

void VisWidget::stopTimer()
{
  m_timer->stop();
}

bool VisWidget::loadMetafile(std::string path, DatasetType tp)
{
  std::string new_path = path + std::string("/");

  bool ret = DatasetManager::instance()->init(new_path);
  ret &= TFManager::instance()->init(new_path + "transfer-functions/");

  metafile_loaded(ret);

  return isMetafileLoaded();
}

bool VisWidget::loadColorTF(std::string key)
{
  return false;
}

bool VisWidget::loadAlphaTF(std::string key)
{
return false;
}

bool VisWidget::saveAlphaTF(std::string key)
{
return false;
}

bool VisWidget::loadDataset(std::string key)
{
  data_loaded(DatasetManager::instance()->setActive(key));
  return isDatasetLoaded();
}

void VisWidget::update()
{
  if(isInitialized() && isDatasetLoaded()) {
    paintGL();
    updateGL();
  }
}

void VisWidget::initializeGL()
{
  makeCurrent();

  glClearColor(0.3f, 0.3f, 0.3f, 1.f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glClearDepth(1.0f);

  glEnable(GL_CULL_FACE);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  GLSLRenderer* rend = new GLSLRenderer(width(), height());
  rend->init();
  rend->updating(true);

  RendererManager::instance()->add("glslrenderer", rend);
  RendererManager::instance()->setActive("glslrenderer");
  initialized(true);
}

void VisWidget::paintGL()
{
  if(isInitialized() && isDatasetLoaded()) {
    RendererManager::instance()->getCurrent()->update();
    RendererManager::instance()->getCurrent()->draw();
  }
}

void VisWidget::resizeEvent(QResizeEvent*)
{
  if(isInitialized())
    RendererManager::instance()->getCurrent()->resize(width(), height());
}

void VisWidget::closeEvent(QCloseEvent*)
{}

void VisWidget::moveEvent(QMoveEvent*)
{}

void VisWidget::keyPressEvent(QKeyEvent*)
{}

void VisWidget::keyReleaseEvent(QKeyEvent*)
{}

void VisWidget::mousePressEvent(QMouseEvent* e)
{
  if(e->button() == Qt::LeftButton) {
    GLSLRenderer* rend = (GLSLRenderer*) RendererManager::instance()->getCurrent();
    rend->setNumSamples(128);
    rend->mousebutton(e->button(), 1, e->x(), e->y());
  }
}

void VisWidget::mouseReleaseEvent(QMouseEvent* e)
{
  GLSLRenderer* rend = (GLSLRenderer*) RendererManager::instance()->getCurrent();
  rend->setNumSamples(256);
  rend->mousebutton(e->button(), 0, e->x(), e->y());
}

void VisWidget::mouseMoveEvent(QMouseEvent* e)
{
  RendererManager::instance()->getCurrent()->mousemove(e->x(), e->y());
}

void VisWidget::wheelEvent(QWheelEvent*)
{}

//void VisWidget::InitResources()
//{
//  using namespace knl;

//#ifdef WIN32
//  DatasetManager::instance()->init("C:/Users/schardong/Pictures/datasets/");
//#else
//  DatasetManager::instance()->init("/home/guilherme/Pictures/datasets/");
//#endif
//  DatasetManager::instance()->setActive("nucleon", GL_TEXTURE1);

//#ifdef WIN32
//  TFManager::instance()->init("C:/Users/schardong/Pictures/datasets/transfer-functions/");
//#else
//  TFManager::instance()->init("/home/guilherme/Pictures/datasets/transfer-functions/");
//#endif
//  TFManager::instance()->setActive("tff1", GL_TEXTURE3);

//  TopAnalyzer::instance()->AnalyzeCurrDataset(m_flowRate,
//    DatasetManager::instance()->getCurrentKey());

//  AlphaManager::instance()->SetActive(DatasetManager::instance()->getCurrentKey(),
//    GL_TEXTURE2);
//}
