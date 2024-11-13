#ifndef COMPONENTS_STATE_RESOURCEMANAGER
#define COMPONENTS_STATE_RESOURCEMANAGER

#include <map>
#include <mutex>
#include <optional>
#include <vector>

#include <osg/Texture2D>

#include <components/state/material.hpp>

#include "texture.hpp"

namespace State
{
    class ResourceManager
    {
    public:
        std::size_t registerMaterial(osg::ref_ptr<State::Material> material);
        std::size_t registerTexture(osg::ref_ptr<osg::Texture2D> texture);

        std::map<std::size_t, Material::GPUData> mQueuedMaterials;
        std::map<std::size_t, BindlessTextureHandle> mQueuedTextures;

    private:
        std::mutex mMutex;
        std::vector<osg::ref_ptr<Material>> mMaterials;
        std::vector<BindlessTextureHandle> mTextures;
    };
}

#endif