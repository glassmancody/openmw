#ifndef OPENMW_COMPONENTS_SCENEUTIL_CLUSTERED_H
#define OPENMW_COMPONENTS_SCENEUTIL_CLUSTERED_H

#include <cstdint>
#include <osg/Drawable>
#include <osg/Vec4f>

namespace SceneUtil
{

    struct Cluster
    {
        osg::Vec4f minPoint;
        osg::Vec4f maxPoint;
    };

    struct LightGrid
    {
        std::uint32_t offset;
        std::uint32_t count;
    };

    struct PointLight
    {
        osg::Vec4f position;
        osg::Vec4f diffuse;
        osg::Vec4f ambient;
        osg::Vec4f specular;
        float constant;
        float linear;
        float quadratic;
        float radius;
    };

    struct InvokeMemoryBarrier : public osg::Drawable::DrawCallback
    {
        InvokeMemoryBarrier(GLbitfield barriers)
            : _barriers(barriers)
        {
        }

        virtual void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
        {
            drawable->drawImplementation(renderInfo);
            renderInfo.getState()->get<osg::GLExtensions>()->glMemoryBarrier(_barriers);
        }
        GLbitfield _barriers;
    };
}

#endif
