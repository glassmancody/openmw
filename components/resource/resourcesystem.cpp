#include <QString>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>

#include "resourcesystem.hpp"

#include <algorithm>

#include "scenemanager.hpp"
#include "imagemanager.hpp"
#include "niffilemanager.hpp"
#include "keyframemanager.hpp"

#include <components/files/configurationmanager.hpp>
#include <components/config/gamesettings.hpp>

namespace Resource
{

    ResourceSystem::ResourceSystem(const VFS::Manager *vfs)
        : mVFS(vfs)
    {
        mNifFileManager.reset(new NifFileManager(vfs));
        mKeyframeManager.reset(new KeyframeManager(vfs));
        mImageManager.reset(new ImageManager(vfs));
        mSceneManager.reset(new SceneManager(vfs, mImageManager.get(), mNifFileManager.get()));

        mCfgMgr.reset(new Files::ConfigurationManager());
        mGameSettings.reset(new Config::GameSettings(*mCfgMgr.get()));

        QString path =mCfgMgr->getUserConfigPath().string().c_str();
        path.append("openmw.cfg");
        QFile file(path);

        Log(Debug::Warning) << "Trying to open: " << path.toStdString();

        if (file.exists()) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) 
                Log(Debug::Warning) << "Failed reading user config file";
            else {
                QTextStream stream(&file);
                stream.setCodec(QTextCodec::codecForName("UTF-8"));

                mGameSettings->readUserFile(stream);
                file.close();
                Log(Debug::Warning) << "Successfully to open: " << path.toStdString();
            }
        }

        addResourceManager(mNifFileManager.get());
        addResourceManager(mKeyframeManager.get());
        // note, scene references images so add images afterwards for correct implementation of updateCache()
        addResourceManager(mSceneManager.get());
        addResourceManager(mImageManager.get());
    }

    Files::ConfigurationManager* ResourceSystem::getConfigurationManager()
    {
        return mCfgMgr.get();
    }

    Config::GameSettings* ResourceSystem::getGameSettings()
    {
        return mGameSettings.get();
    }

    ResourceSystem::~ResourceSystem()
    {
        // this has to be defined in the .cpp file as we can't delete incomplete types

        mResourceManagers.clear();
    }

    SceneManager* ResourceSystem::getSceneManager()
    {
        return mSceneManager.get();
    }

    ImageManager* ResourceSystem::getImageManager()
    {
        return mImageManager.get();
    }

    NifFileManager* ResourceSystem::getNifFileManager()
    {
        return mNifFileManager.get();
    }

    KeyframeManager* ResourceSystem::getKeyframeManager()
    {
        return mKeyframeManager.get();
    }

    void ResourceSystem::setExpiryDelay(double expiryDelay)
    {
        for (std::vector<BaseResourceManager*>::iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->setExpiryDelay(expiryDelay);

        // NIF files aren't needed any more once the converted objects are cached in SceneManager / BulletShapeManager,
        // so no point in using an expiry delay
        mNifFileManager->setExpiryDelay(0.0);
    }

    void ResourceSystem::updateCache(double referenceTime)
    {
        for (std::vector<BaseResourceManager*>::iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->updateCache(referenceTime);
    }

    void ResourceSystem::clearCache()
    {
        for (std::vector<BaseResourceManager*>::iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->clearCache();
    }

    void ResourceSystem::addResourceManager(BaseResourceManager *resourceMgr)
    {
        mResourceManagers.push_back(resourceMgr);
    }

    void ResourceSystem::removeResourceManager(BaseResourceManager *resourceMgr)
    {
        std::vector<BaseResourceManager*>::iterator found = std::find(mResourceManagers.begin(), mResourceManagers.end(), resourceMgr);
        if (found != mResourceManagers.end())
            mResourceManagers.erase(found);
    }

    const VFS::Manager* ResourceSystem::getVFS() const
    {
        return mVFS;
    }

    void ResourceSystem::reportStats(unsigned int frameNumber, osg::Stats *stats) const
    {
        for (std::vector<BaseResourceManager*>::const_iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->reportStats(frameNumber, stats);
    }

    void ResourceSystem::releaseGLObjects(osg::State *state)
    {
        for (std::vector<BaseResourceManager*>::const_iterator it = mResourceManagers.begin(); it != mResourceManagers.end(); ++it)
            (*it)->releaseGLObjects(state);
    }

}
