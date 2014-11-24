#include "alphamanager.h"
#include "logger.h"

#define TIXML_USE_STL

#include <tinyxml.h>

using namespace knl;

bool AlphaManager::init(std::string path)
{
  m_alphaPath = path;

  TiXmlDocument metafile(path + std::string("metafile.xml"));
  bool load_ok = metafile.LoadFile();

  if(!load_ok) return false;

  TiXmlNode* node = NULL;
  TiXmlElement* data_elem = NULL;

  node = metafile.FirstChild("alpha-maps");
  if(node == NULL) return true;

  data_elem = node->ToElement();

  for(TiXmlNode* data_child = node->FirstChild(); data_child != NULL; data_child = data_child->NextSibling()) {
    Dataset* d = new Dataset;

    std::istringstream(data_child->FirstChild("width")->FirstChild()->Value()) >> d->width;
    std::istringstream(data_child->FirstChild("height")->FirstChild()->Value()) >> d->height;
    std::istringstream(data_child->FirstChild("slices")->FirstChild()->Value()) >> d->slices;
    std::istringstream(data_child->FirstChild("bytes_elem")->FirstChild()->Value()) >> d->bytes_elem;

    std::string alpha_key = data_child->FirstChild("name")->FirstChild()->Value();
    std::string data_key = data_child->FirstChild("dataset")->FirstChild()->Value();
    add(alpha_key, data_key, d);
  }

  return true;
}

bool AlphaManager::add(std::string key, std::string data_key, Dataset* alpha_map)
{
  if(key.empty() || data_key.empty() || alpha_map == NULL) {
    Logger::getInstance()->error("AlphaManager::Add - invalid parameters. Returning now.");
    return false;
  }

  m_alphaMap[key] = new Dataset(*alpha_map);
  m_alphaDataMap[key] = data_key;
  return true;
}

bool AlphaManager::setActive(std::string key, GLenum tex_unit)
{
  if(key.empty() || m_activeKey == key) return true;

  std::map<std::string, Dataset*>::iterator it = m_alphaMap.find(key);

  if(it == m_alphaMap.end()) return false;

  Dataset* data = it->second;
  if(!data->isLoaded()) {
    if(!data->load(m_alphaPath + it->first + ".raw")) {
      Logger::getInstance()->error("Failed to load alpha map " + key);
      return false;
    }
  }
  if(!data->isUploaded()) {
    if(!data->upload()) {
      Logger::getInstance()->error("Failed to upload alpha map " + key);
      return false;
    }
  }

  data->bind(tex_unit);
  m_activeKey = key;
  return true;
}

Dataset* AlphaManager::get(std::string key)
{
  assert(!key.empty());
  assert(m_alphaMap.find(key) != m_alphaMap.end());
  return m_alphaMap[key];
}

Dataset* AlphaManager::getCurrent()
{
  return get(m_activeKey);
}

void AlphaManager::freeResources()
{
  for(auto it = m_alphaMap.begin(); it != m_alphaMap.end(); it++) {
    if(it->second->tex_id != 0)
      glDeleteTextures(1, &it->second->tex_id);
    delete it->second;
  }

  m_alphaMap.clear();
}
