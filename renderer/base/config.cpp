/*
 * config.cpp
 *
 *  Created on: Mar 10, 2012
 *      Author: flamingo
 */

#include "config.h"

#include <yaml-cpp/yaml.h>
#include "poly.h"

namespace base
{

struct ModelData
{
    string modelpath;
    math::vec3 pos;
    math::Triangle::WindingOrder wo;
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
    string wo;
    node["order"] >> wo;

    if (wo == "CW")
        data.wo = math::Triangle::WO_CW;
    else if (wo == "CCW")
        data.wo = math::Triangle::WO_CCW;
    else
    {
        *syslog << "Bad winding order in model" << data.modelpath << ". Setting to defaults" << logwarn;
        data.wo = math::Triangle::WO_CW;
    }
}

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
        for (YAML::Iterator i = doc.begin(); i != doc.end(); ++i)
        {
            const YAML::Node &value = i.second();

            for(unsigned i = 0; i < value.size(); i++)
            {
                ModelData currdata;
                value[i] >> currdata;
                modelData.push_back(currdata);
            }
        }

        parser.GetNextDocument(doc);
        math::vec3 vectCam;
        int width, height;
        doc["Campos"] >> vectCam;
        controller->m_mainCam->setPosition(vectCam);

        doc["Width"] >> width;
        doc["Height"] >> height;

        controller->resize(width, height);
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
        sptr(base::Resource) gettedResource = controller->m_resourceMgr->getResource(modelpath);
        if (!gettedResource)
            continue;

        // check: is it rendering item?
        sptr(rend::Mesh) renderItem = dynamic_pointer_cast<rend::Mesh>(gettedResource);
        if (!renderItem)
        {
            *syslog << "Can't render" << modelpath << logerr;
            continue;
        }

        // save it
        renderItem->setPosition(modelData[i].pos);
        //renderItem->setWindingOrder(modelData[i].wo);
        controller->m_rendmgr->addMesh(renderItem);
    }
}


}
