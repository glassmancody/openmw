#ifndef COMPONENTS_STATE_TEXTURE
#define COMPONENTS_STATE_TEXTURE

#include <optional>

#include <osg/Texture2D>

namespace State
{
    struct BindlessTextureHandle
    {
        std::optional<std::uint64_t> handle = std::nullopt;
        osg::ref_ptr<osg::Texture2D> texture;
        bool resident = false;
        int virtualIndex = 0;
        std::string filename = "";
    };
}

#endif
