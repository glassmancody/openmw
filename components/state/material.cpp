#include "material.hpp"

#include <algorithm>

#include <osg/Material>

namespace State
{

    Material::Material()
        : osg::StateAttribute()
    {
    }

    Material::Material(const Material& other, const osg::CopyOp& copyop)
        : osg::StateAttribute(other, copyop)
        , mDiffuse(other.mDiffuse)
        , mAmbient(other.mAmbient)
        , mSpecular(other.mSpecular)
        , mEmission(other.mEmission)
        , mShininess(other.mShininess)
        , mEmissiveMultiplier(other.mEmissiveMultiplier)
        , mSpecularStrength(other.mSpecularStrength)
        , mAlpha(other.mAlpha)
        , mColorMode(other.mColorMode)
    {
    }

    void Material::apply(osg::State& state) const
    {
        // TODO: change based on legacy bit set
        if (true)
        {
            return;
        }

        // osg::ref_ptr<osg::Material> m = new osg::Material;
        // m->setAmbient(osg::Material::FRONT_AND_BACK, mAmbient);
        // m->setDiffuse(osg::Material::FRONT_AND_BACK, mDiffuse);
        // m->setEmission(osg::Material::FRONT_AND_BACK, mEmission);
        // m->setShininess(osg::Material::FRONT_AND_BACK, mShininess);
        // m->setAlpha(osg::Material::FRONT_AND_BACK, mAlpha);

        // osg::Material::ColorMode mode = osg::Material::OFF;

        // switch (mColorMode)
        // {
        //     case 0:
        //         mode = osg::Material::OFF;
        //         break;
        //     case 1:
        //         mode = osg::Material::EMISSION;
        //         break;
        //     default:
        //     case 2:
        //         mode = osg::Material::AMBIENT_AND_DIFFUSE;
        //         break;
        //     case 3:
        //         mode = osg::Material::AMBIENT;
        //         break;
        //     case 4:
        //         mode = osg::Material::DIFFUSE;
        //         break;
        //     case 5:
        //         mode = osg::Material::SPECULAR;
        //         break;
        // }

        // m->setColorMode(mode);
        // m->apply(state);
    }

    int Material::compare(const StateAttribute& sa) const
    {
        COMPARE_StateAttribute_Types(Material, sa);
        COMPARE_StateAttribute_Parameter(mDiffuse);
        COMPARE_StateAttribute_Parameter(mAmbient);
        COMPARE_StateAttribute_Parameter(mSpecular);
        COMPARE_StateAttribute_Parameter(mEmission);
        COMPARE_StateAttribute_Parameter(mShininess);
        COMPARE_StateAttribute_Parameter(mEmissiveMultiplier);
        COMPARE_StateAttribute_Parameter(mSpecularStrength);
        COMPARE_StateAttribute_Parameter(mAlpha);
        COMPARE_StateAttribute_Parameter(mColorMode);

        return 0;
    }

    bool Material::operator==(const Material& other) const
    {
        return (mDiffuse == other.mDiffuse && mAmbient == other.mAmbient && mSpecular == other.mSpecular
            && mEmission == other.mEmission && mShininess == other.mShininess
            && mEmissiveMultiplier == other.mEmissiveMultiplier && mSpecularStrength == other.mSpecularStrength
            && mAlpha == other.mAlpha && mColorMode == other.mColorMode);
    }

    void Material::releaseGLObjects(osg::State* state) const {}

}