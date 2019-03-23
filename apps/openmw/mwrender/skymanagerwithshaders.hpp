#ifndef OPENMW_MWRENDER_SKYMANAGERWITHSHADERS_H
#define OPENMW_MWRENDER_SKYMANAGERWITHSHADERS_H

#include "sky.hpp"

namespace MWRender
{
    class SkyManagerWithShaders : public SkyManager
    {
    public:
        SkyManagerWithShaders(osg::Group* parentNode, Resource::SceneManager* sceneManager);
        ~SkyManagerWithShaders();

        void setSunHeight(float) override;
        void setWeather(int weatherID0, int weatherID1, float transitionFactor) override;
        void setCameraPos(const osg::Vec3f& pos) override;
        void setSunDirection(const osg::Vec3f& direction) override;

    protected:
        void create() override;
        float getCloudSpeed() override { return SkyManager::getCloudSpeed() * 0.1; }

    private:
        class StateUpdater;

        osg::ref_ptr<StateUpdater> mStateUpdater;
    };
}

#endif
