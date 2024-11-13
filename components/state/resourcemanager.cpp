#include "resourcemanager.hpp"

#include <components/debug/debuglog.hpp>

namespace State
{
    std::size_t ResourceManager::registerMaterial(osg::ref_ptr<State::Material> material)
    {
        for (std::size_t i = 0; i < mMaterials.size(); ++i)
        {
            if (*mMaterials[i].get() == *material.get())
            {
                return i;
            }
        }

        mMaterials.push_back(std::move(material));
        std::size_t index = mMaterials.size() - 1;
        mQueuedMaterials[index] = mMaterials[index]->compile();

        return index;
    }

    std::size_t ResourceManager::registerTexture(osg::ref_ptr<osg::Texture2D> texture)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!texture->getImage())
            throw std::runtime_error("Failed retrieving the image associated with a texture");
        std::string filename = texture->getImage()->getFileName();
        if (filename.empty())
            throw std::runtime_error("Empty filename, this texture doesn't look right");

        for (std::size_t i = 0; i < mTextures.size(); ++i)
        {
            if (mTextures[i].filename == filename)
            {
                return i;
            }
        }

        // Log(Debug::Error) << "REGISTER: " << mTextures.size() << " : " << filename;

        mTextures.emplace_back(std::nullopt, std::move(texture), false, mTextures.size(), filename);
        std::size_t index = mTextures.size() - 1;
        mQueuedTextures[index] = mTextures[index];

        return index;
    }
}