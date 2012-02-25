#include "config.h"

#include <yaml-cpp/yaml.h>

struct ModelData
{
    string modelpath;
    math::vec3 pos;
};

static void operator>> (const YAML::Node &node, math::vec3 &v)
{
    node[0] >> v.x;
    node[1] >> v.y;
    node[2] >> v.z;
}

static void operator>> (const YAML::Node &node, ModelData &data)
{
    node["model"] >> data.modelpath;
    node["position"] >> data.pos;
}

namespace base
{

Config::Config()
{
}

Config::~Config()
{
}

void Config::configure(const OsPath &path, Controller *controller)
{
    std::fstream inconf(path.filePath());
    if (!inconf)
    {
        *syslog << "Can't open configuration file" << logerr;
        return;
    }

    vector<ModelData> modelData;
    try
    {
        YAML::Parser parser(inconf);
        YAML::Node doc;

        parser.GetNextDocument(doc);
        for(unsigned i = 0;i < doc.size(); i++)
        {
            ModelData currdata;
            doc[i] >> currdata;
            modelData.push_back(currdata);
        }
    }
    catch(YAML::Exception &e)
    {
        *syslog << "Error while parsing config file. Setting to defaults."
                << e.what() << logerr;
        modelData.clear();
    }

    for (unsigned i = 0; i < modelData.size(); i++)
    {
        string modelpath = modelData[i].modelpath;
        if (modelpath.empty())
        {
            *syslog << "Empty model name" << logwarn;
            continue;
        }

        // load resource
        SPTR(base::Resource) gettedResource = controller->m_resourceMgr->getResource(modelpath);
        if (!gettedResource)
            continue;

        // check: is it rendering item?
        SPTR(rend::Mesh) renderItem = dynamic_pointer_cast<rend::Mesh>(gettedResource);
        if (!renderItem)
        {
            *syslog << "Can't render" << modelpath << logerr;
            continue;
        }

        // save it
        renderItem->setPosition(modelData[i].pos);
        controller->m_rendmgr->addMesh(renderItem);
    }
}


}