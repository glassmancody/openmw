#ifndef OPENMW_MWGUI_SPELLLISTWIDGET_H
#define OPENMW_MWGUI_SPELLLISTWIDGET_H

#include <cmath>
#include <unordered_map>

#include <MyGUI_Widget.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_ImageBox.h>

#include <components/widgets/box.hpp>
#include <components/widgets/sharedstatebutton.hpp>

#include "spellmodel.hpp"
#include "tooltips.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"

namespace MWGui
{

    /// @brief A widget that shows an icon for an MWWorld::Ptr
    class SpellListWidget : public MyGUI::Widget
    {

        MYGUI_RTTI_DERIVED(SpellListWidget)

    public:
        SpellListWidget();

        /// Register needed components with MyGUI's factory manager
        static void registerComponents ();

        typedef MyGUI::delegates::CMultiDelegate1<SpellListWidget*> EventHandle_Item;
        EventHandle_Item eventItemFocused;
        
        void setStateFocused(bool focus);
        void setStateSelected(bool state)
        {
            //mName->setStateSelected(state);
            //mName->setTextColour(MyGUI::Colour::parse("0.2 0.2 1.0"));
        }

        void setSpell(const Spell& spell, int category)
        {
            static const std::string schoolNames[] = {
                "#{sSchoolAlteration}", "#{sSchoolConjuration}", 
                "#{sSchoolDestruction}", "#{sSchoolIllusion}", 
                "#{sSchoolMysticism}", "#{sSchoolRestoration}"};

            const std::string captionSuffix = MWGui::ToolTips::getCountString(spell.mCount);    

            mName->setCaption(spell.mName + captionSuffix);
            mName->setTextAlign(MyGUI::Align::Left);

            if (spell.mSelected)
            {
                int x = mName->getCoord().left + mName->getTextSize().width+8;
                static constexpr int size = 16;
                MyGUI::ImageBox* t = mItem->getParent()->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(x,10,size,size), MyGUI::Align::Left | MyGUI::Align::VCenter);
                t->setImageTexture("textures\\ui\\enchanted.dds");
                t->setNeedMouseFocus(false);
            }
            
            mSchool->setVisible(false);
            mCostChance->setVisible(false);

            Gui::ButtonGroup group;

            if (category == MWGui::SpellModel::Category_All)
            {
                std::string school = "Power";
                if (spell.mType == Spell::Type_Spell)
                {
                    const ESM::Spell* spellPtr = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spell.mId);
                    int schoolID = MWMechanics::getSpellSchool(spellPtr, MWMechanics::getPlayer());
                    school = schoolNames[schoolID];
                }
                else if (spell.mType == Spell::Type_EnchantedItem)
                    school = "Item";

                group.push_back(mSchool);
                mSchool->setCaptionWithReplacing(school);
                mSchool->setVisible(true);
            }

            if (category != SpellModel::Category_Simple)
            {
                mCostChance->setCaption(spell.mCostColumn);
                mCostChance->setTextAlign(MyGUI::Align::Right);
                mCostChance->setVisible(true);
                group.push_back(mCostChance);
            }

            group.push_back(mName);
            
            Gui::SharedStateButton::createButtonGroup(group);

            mOverlay = mItem->getParent()->createWidget<MyGUI::ImageBox>("ImageBox",
                MyGUI::IntCoord(0,0,mItem->getParent()->getWidth(),mItem->getParent()->getHeight()), MyGUI::Align::Stretch);
            mOverlay->setImageTexture("textures\\ui\\selected.dds");
            mOverlay->setNeedMouseFocus(false);
            mOverlay->setVisible(false);

            // hidden widget still take up space :(
            for (size_t i = 0; i < mItem->getChildCount(); i++)
            {
                MyGUI::Widget* child = mItem->getChildAt(i); 
                if (child == nullptr) continue; 
                if (!child->getVisible())
                    child->setSize(0,0);
            }
        }

    protected:
        void initialiseOverride() final;

        void onMouseSetFocus(MyGUI::Widget *_old) final;
        void onMouseLostFocus(MyGUI::Widget *_new) final;

        Gui::SharedStateButton* mName; 
        Gui::SharedStateButton* mSchool;
        Gui::SharedStateButton* mCostChance; 
        Gui::HBox* mItem;
        MyGUI::ImageBox* mOverlay;

        //static const std::unordered_map<ESM::Weapon::Type, const std::string> mSchoolType; 
        static const std::string mBlank; 
    };
}

#endif