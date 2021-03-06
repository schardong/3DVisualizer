#include "tfmanager.h"
#include "logger.h"

#define TIXML_USE_STL

#include <tinyxml.h>

bool TFManager::init(std::string path)
{
  m_tfPath = path;

  TiXmlDocument metafile(path + std::string("metafile.xml"));
  bool load_ok = metafile.LoadFile();

  if(!load_ok) return false;

  TiXmlNode* node = NULL;

  node = metafile.FirstChild("transfer-functions");

  for(TiXmlNode* data_child = node->FirstChildElement();
      data_child != NULL;
      data_child = data_child->NextSiblingElement()) {

    TFunction* tf = new TFunction;
    std::string type_str = data_child->ToElement()->Attribute("type");

    if(type_str == "alpha") {
      tf->num_channels = 1;
    } else if(type_str == "rgb") {
      tf->num_channels = 3;
    } else if(type_str == "rgba") {
      tf->num_channels = 4;
    }

    std::istringstream(data_child->FirstChild("rows")->FirstChild()->Value()) >> tf->rows;
    std::istringstream(data_child->FirstChild("bytes_elem")->FirstChild()->Value()) >> tf->bytes_elem;

    add(data_child->FirstChild("name")->FirstChild()->Value(), tf);
  }

  return true;
}

bool TFManager::add(std::string key, TFunction* tf)
{
  if(key.empty() || tf == NULL) {
    Logger::getInstance()->error("TFManager::Add invalid parameters.");
    return false;
  }

  m_funcMap[key] = new TFunction(*tf);
  return true;
}

bool TFManager::setActive(std::string key, GLenum tex_unit)
{
  if(key.empty() || m_activeKey == key) return true;

  std::map<std::string, TFunction*>::iterator it = m_funcMap.find(key);

  if(it == m_funcMap.end()) return false;

  TFunction* tf = it->second;
  if(!tf->isLoaded()) {
    if(!tf->load(m_tfPath + it->first + ".raw")) {
      Logger::getInstance()->error("Failed to load transfer function " + key);
      return false;
    }
  }
  if(!tf->isUploaded()) {
    if(!tf->upload()) {
      Logger::getInstance()->error("Failed to upload transfer function " + key);
      return false;
    }
  }

  tf->bind(tex_unit);
  m_activeKey = key;
  return true;
}

TFunction* TFManager::get(std::string key)
{
  if(key.empty() || m_funcMap.find(key) == m_funcMap.end()) {
    Logger::getInstance()->error("TFManager::get invalid key.");
    return NULL;
  }
  return m_funcMap[key];
}

TFunction* TFManager::getCurrent()
{
  return get(m_activeKey);
}

void TFManager::freeResources()
{
  for(auto it = m_funcMap.begin(); it != m_funcMap.end(); it++) {
    if(it->second->tex_id != 0)
      glDeleteTextures(1, &it->second->tex_id);
    delete it->second;
  }

  m_funcMap.clear();
}
