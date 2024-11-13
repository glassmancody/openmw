#ifndef COMPONENTS_STATE_GPUSTATE
#define COMPONENTS_STATE_GPUSTATE

#include <array>
#include <map>
#include <unordered_set>

#include <osg/Drawable>

#include <components/state/material.hpp>

namespace State
{
    struct BindlessTextureHandle;
    class ResourceManager;

    class GPUState : public osg::Drawable
    {
    public:
        GPUState(std::shared_ptr<ResourceManager> resourceManager = nullptr);

        void drawImplementation(osg::RenderInfo& renderInfo) const override;

        void releaseGLObjects(osg::State* state = nullptr) const override;

        void traverse(osg::NodeVisitor& nv) override;

        mutable std::array<std::map<std::size_t, Material::GPUData>, 2> mQueuedGPUMaterials;
        mutable std::array<std::map<std::size_t, BindlessTextureHandle>, 2> mQueuedGPUTextures;

    private:
        int open(osg::State& state, BindlessTextureHandle& handle);
        void close(osg::State& state, BindlessTextureHandle& handle);
        void makeGPUHandle(osg::State& state, BindlessTextureHandle& handle);

        std::shared_ptr<ResourceManager> mResourceManager = nullptr;

        mutable std::vector<Material::GPUData> mMaterials;
        mutable std::vector<std::uint64_t> mTextures;
        mutable unsigned int mSSBOMaterials = 0;
        mutable unsigned int mSSBOTextures = 0;
        mutable size_t mBufferSize = 0;
        mutable size_t mTextureBufferSize = 0;
    };
}

#endif