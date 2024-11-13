#ifndef COMPONENTS_STATE_MATERIAL
#define COMPONENTS_STATE_MATERIAL

#include <osg/StateAttribute>

namespace osg
{
    class Vec4f;
}

namespace State
{
    enum class ColorModes : std::int32_t
    {
        ColorMode_None = 0,
        ColorMode_Emission = 1,
        ColorMode_AmbientAndDiffuse = 2,
        ColorMode_Ambient = 3,
        ColorMode_Diffuse = 4,
        ColorMode_Specular = 5,
    };

    class Material : public osg::StateAttribute
    {
    public:
        struct alignas(16) GPUData
        {
            std::array<float, 4> mDiffuse = { 0, 0, 0, 1 };
            std::array<float, 4> mAmbient = { 1, 1, 1, 1 };
            std::array<float, 4> mSpecular = { 0, 0, 0, 1 };
            std::array<float, 4> mEmission = { 0, 0, 0, 1 };
            float mShininess = 0.0;
            float mEmissiveMultiplier = 1.0;
            float mSpecularStrength = 1.0;
            std::int32_t mColorMode = static_cast<std::int32_t>(ColorModes::ColorMode_None);
        };

        Material();

        Material(const Material& other, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_StateAttribute(State, Material, osg::StateAttribute::MATERIAL)

        void apply(osg::State& state) const override;

        int compare(const StateAttribute& sa) const override;

        void releaseGLObjects(osg::State* state = nullptr) const override;

        GPUData compile()
        {
            GPUData data;
            data.mDiffuse = { mDiffuse.x(), mDiffuse.y(), mDiffuse.z(), mAlpha };
            data.mAmbient = { mAmbient.x(), mAmbient.y(), mAmbient.z(), mAlpha };
            data.mSpecular = { mSpecular.x(), mSpecular.y(), mSpecular.z(), mAlpha };
            data.mEmission = { mEmission.x(), mEmission.y(), mEmission.z(), mAlpha };
            data.mShininess = std::clamp(mShininess, 0.f, 128.f);
            data.mEmissiveMultiplier = mEmissiveMultiplier;
            data.mSpecularStrength = mSpecularStrength;
            data.mColorMode = static_cast<std::int32_t>(mColorMode);

            mDirty = false;

            return data;
        }

        bool operator==(const Material& other) const;

        void setDiffuse(const osg::Vec4f& diffuse)
        {
            if (diffuse == mDiffuse)
                return;

            mDiffuse = diffuse;
            mDirty = true;
        }

        osg::Vec4f getDiffuse() const { return mDiffuse; }

        void setAmbient(const osg::Vec4f& ambient)
        {
            if (ambient == mAmbient)
                return;

            mAmbient = ambient;
            mDirty = true;
        }

        osg::Vec4f getAmbient() const { return mAmbient; }

        void setSpecular(const osg::Vec4f& specular)
        {
            if (specular == mSpecular)
                return;

            mSpecular = specular;
            mDirty = true;
        }

        osg::Vec4f getSpecular() const { return mSpecular; }

        void setEmission(const osg::Vec4f& emission)
        {
            if (emission == mEmission)
                return;

            mEmission = emission;
            mDirty = true;
        }

        osg::Vec4f getEmission() const { return mEmission; }

        void setShininess(float shininess)
        {
            if (shininess == mShininess)
                return;

            mShininess = shininess;
            mDirty = true;
        }

        float getShininess() const { return mShininess; }

        void setEmissiveMultiplier(float mult)
        {
            if (mult == mEmissiveMultiplier)
                return;

            mEmissiveMultiplier = mult;
            mDirty = true;
        }

        float getEmissiveMultiplier() const { return mEmissiveMultiplier; }

        void setSpecularStrength(float strength)
        {
            if (strength == mSpecularStrength)
                return;

            mSpecularStrength = strength;
            mDirty = true;
        }

        float getSpecularStrength() const { return mSpecularStrength; }

        void setAlpha(float alpha)
        {
            if (alpha == mAlpha)
                return;

            mAlpha = alpha;
            mDirty = true;
        }

        float getAlpha() const { return mAlpha; }

        void setColorMode(ColorModes mode)
        {
            if (mode == mColorMode)
                return;

            mColorMode = mode;
            mDirty = true;
        }

        ColorModes getColorMode() const { return mColorMode; }

    private:
        bool mDirty = false;

        osg::Vec4f mDiffuse;
        osg::Vec4f mAmbient;
        osg::Vec4f mSpecular;
        osg::Vec4f mEmission;
        float mShininess = 0.0;
        float mEmissiveMultiplier = 1.0;
        float mSpecularStrength = 1.0;
        float mAlpha = 1.0;
        ColorModes mColorMode = ColorModes::ColorMode_None;
    };
}

#endif