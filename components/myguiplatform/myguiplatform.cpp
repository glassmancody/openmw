#include "myguiplatform.hpp"

#include <components/resource/resourcesystem.hpp>

#include "myguidatamanager.hpp"
#include "myguiloglistener.hpp"
#include "myguirendermanager.hpp"

namespace MyGUIPlatform
{

    Platform::Platform(osgViewer::Viewer* viewer, osg::Group* guiRoot, Resource::ResourceSystem* resourceSystem,
        float uiScalingFactor, VFS::Path::NormalizedView resourcePath, const std::filesystem::path& logName)
        : mLogFacility(logName.empty() ? nullptr : std::make_unique<LogFacility>(logName, false))
        , mLogManager(std::make_unique<MyGUI::LogManager>())
        , mDataManager(std::make_unique<DataManager>(resourcePath, resourceSystem->getVFS()))
        , mRenderManager(std::make_unique<RenderManager>(viewer, guiRoot, resourceSystem, uiScalingFactor))
    {
        if (mLogFacility != nullptr)
            mLogManager->addLogSource(mLogFacility->getSource());

        mRenderManager->initialise();
    }

    Platform::~Platform() = default;

    void Platform::shutdown()
    {
        mRenderManager->shutdown();
    }

    RenderManager* Platform::getRenderManagerPtr()
    {
        return mRenderManager.get();
    }

    DataManager* Platform::getDataManagerPtr()
    {
        return mDataManager.get();
    }

}
