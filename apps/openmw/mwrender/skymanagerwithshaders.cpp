#include "skymanagerwithshaders.hpp"

#include <osg/PositionAttitudeTransform>
#include <osg/ValueObject>

#include <components/resource/scenemanager.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/shader/shadermanager.hpp>

#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>

using namespace MWRender;

namespace
{


osg::ref_ptr<osg::Program> getProgram(std::string shader, Shader::ShaderManager& shaderManager)
{
    std::map<std::string, std::string> defineMap;
    osg::ref_ptr<osg::Shader> vertexShader = shaderManager.getShader(shader + "_vertex.glsl", defineMap, osg::Shader::VERTEX);
    osg::ref_ptr<osg::Shader> fragmentShader = shaderManager.getShader(shader + "_fragment.glsl", defineMap, osg::Shader::FRAGMENT);
    if (!vertexShader || !fragmentShader)
        throw std::runtime_error("Unable to create shader");

    return shaderManager.getProgram(vertexShader, fragmentShader);
}


class ShaderVisitor : public osg::NodeVisitor
{
public:
    ShaderVisitor(Shader::ShaderManager& shaderManager) :
        osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mShaderManager(shaderManager)
    {
    }

    void apply(osg::Node& node) override
    {
        using namespace MWRender::SkyObjectType;

        int objectType = 0;
        osg::ref_ptr<osg::Program> program;

        auto stateSet = node.getOrCreateStateSet();

        if (node.getUserValue<int>("skyObjectType", objectType))
        {
            switch (objectType)
            {
                case SUN:
                    program = getProgram("sun", mShaderManager);
                    break;
                case SUN_FLASH:
                    program = getProgram("sunflash", mShaderManager);
                    break;
                case MOON:
                    program = getProgram("moon", mShaderManager);
                    stateSet->addUniform(new osg::Uniform("isAtmosphere", true),
                                            osg::StateAttribute::OVERRIDE);
                    break;
                case CLOUDS:
                    program = getProgram("clouds", mShaderManager);
                    stateSet->addUniform(new osg::Uniform("isClouds", true),
                                            osg::StateAttribute::OVERRIDE);
                    stateSet->addUniform(new osg::Uniform("phaseMap", 1));
                    break;
                case ATMOSPHERE:
                    program = getProgram("atmosphere", mShaderManager);
                    stateSet->addUniform(new osg::Uniform("isAtmosphere", true),
                                            osg::StateAttribute::OVERRIDE);
                    break;
                case ATMOSPHERE_NIGHT:
                    program = getProgram("atmosphere_night", mShaderManager);
                    stateSet->addUniform(new osg::Uniform("isAtmosphere", true),
                                            osg::StateAttribute::OVERRIDE);
                    break;
            }
        }

        if (program)
        {
            stateSet->setAttributeAndModes(program,
                osg::StateAttribute::OVERRIDE|osg::StateAttribute::PROTECTED|osg::StateAttribute::ON);
        }

        stateSet->addUniform(new osg::Uniform("diffuseMap", 0));

        traverse(node);
    }

private:
    Shader::ShaderManager& mShaderManager;
};


}


namespace MWRender
{


struct SkyManagerWithShaders::StateUpdater : public SceneUtil::StateSetUpdater
{
    osg::Vec3f mCameraPos;
    osg::Vec3f mSunDirection;
    float mSunHeight = 0;
    int mWeather0 = 0;
    int mWeather1 = 0;
    float mWeatherBlend = 0;

    void setDefaults(osg::StateSet *stateset) override
    {
        stateset->addUniform(new osg::Uniform("isSunFlash", false));
        stateset->addUniform(new osg::Uniform("isVisibleQuery", false));
        stateset->addUniform(new osg::Uniform("isWaterReflection", false));
        stateset->addUniform(new osg::Uniform("isWaterRefraction", false));
        stateset->addUniform(new osg::Uniform("isAtmosphere", false));
        stateset->addUniform(new osg::Uniform("isClouds", false));
        stateset->addUniform(new osg::Uniform("world.sunDirection", mSunDirection));
        stateset->addUniform(new osg::Uniform("world.cameraPos", mCameraPos));
        stateset->addUniform(new osg::Uniform("world.sunHeight", mSunHeight));
        stateset->addUniform(new osg::Uniform("world.weather0", mWeather0));
        stateset->addUniform(new osg::Uniform("world.weather1", mWeather1));
        stateset->addUniform(new osg::Uniform("world.weatherBlend", mWeatherBlend));
    }

    void apply(osg::StateSet* stateset, osg::NodeVisitor*) override
    {
        stateset->getUniform("world.sunDirection")->set(mSunDirection);
        stateset->getUniform("world.cameraPos")->set(mCameraPos);
        stateset->getUniform("world.sunHeight")->set(mSunHeight);
        stateset->getUniform("world.weather0")->set(mWeather0);
        stateset->getUniform("world.weather1")->set(mWeather1);
        stateset->getUniform("world.weatherBlend")->set(mWeatherBlend);
    }
};


SkyManagerWithShaders::SkyManagerWithShaders(osg::Group* parentNode,
                                             Resource::SceneManager* sceneManager)
    : SkyManager(parentNode, sceneManager)
{
    mStateUpdater = new StateUpdater;
    parentNode->addUpdateCallback(mStateUpdater);
}

void SkyManagerWithShaders::create()
{
    SkyManager::create();

    getAtmosphereNode()->setPosition(osg::Vec3(0,0,-300)); // HACK cover the horizon
    getCloudNode()->setPosition(osg::Vec3(0,0,-250));

    ShaderVisitor shaderVisitor(getSceneManager()->getShaderManager());
    getRootNode()->accept(shaderVisitor);
}

void SkyManagerWithShaders::setCameraPos(const osg::Vec3f& pos)
{
    mStateUpdater->mCameraPos = pos;
}

void SkyManagerWithShaders::setSunDirection(const osg::Vec3f& direction)
{
    SkyManager::setSunDirection(direction);
    mStateUpdater->mSunDirection = direction;
}

void SkyManagerWithShaders::setSunHeight(float height)
{
    mStateUpdater->mSunHeight = height;
}

void SkyManagerWithShaders::setWeather(int weatherID0, int weatherID1, float transitionFactor)
{
    mStateUpdater->mWeather0 = weatherID0;
    mStateUpdater->mWeather1 = weatherID1;
    mStateUpdater->mWeatherBlend = transitionFactor;
}


}
