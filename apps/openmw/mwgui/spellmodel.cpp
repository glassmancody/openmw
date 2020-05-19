#include "spellmodel.hpp"

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

namespace
{

    bool sortSpells(const MWGui::Spell& left, const MWGui::Spell& right)
    {
        if (left.mType != right.mType)
            return left.mType < right.mType;

        std::string leftName = Misc::StringUtils::lowerCase(left.mName);
        std::string rightName = Misc::StringUtils::lowerCase(right.mName);

        return leftName.compare(rightName) < 0;
    }

    bool sortSchool(const MWGui::Spell& left, const MWGui::Spell& right)
    {
        if (left.mType != right.mType)
            return left.mType < right.mType;

        if (left.mType == MWGui::Spell::Type_EnchantedItem || right.mType == MWGui::Spell::Type_EnchantedItem)
            return left.mType - right.mType > 0;

        const ESM::Spell* leftPtr = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(left.mId);
        int leftId = MWMechanics::getSpellSchool(leftPtr, MWMechanics::getPlayer());

        const ESM::Spell* rightPtr = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(right.mId);
        int rightId = MWMechanics::getSpellSchool(rightPtr, MWMechanics::getPlayer());

        return leftId < rightId;
    }
    /*
    double getCostChance(const MWGui::Spell& spell)
    {
        // TODO 
    }

    bool sortCostChance(const MWGui::Spell& left, const MWGui::Spell& right)
    {
        // TODO
    }
    */
}

namespace MWGui
{

    SpellModel::SpellModel(const MWWorld::Ptr &actor, const std::string& filter)
        : mActor(actor), mFilter(filter), mIncreasing(true)
    {
    }

    SpellModel::SpellModel(const MWWorld::Ptr &actor)
        : mActor(actor), mIncreasing(true)
    {
    }

    void SpellModel::update()
    {
        mSpells.clear();

        MWMechanics::CreatureStats& stats = mActor.getClass().getCreatureStats(mActor);
        const MWMechanics::Spells& spells = stats.getSpells();

        const MWWorld::ESMStore &esmStore = MWBase::Environment::get().getWorld()->getStore();
        const auto& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        std::string filter = Misc::StringUtils::lowerCaseUtf8(mFilter);

        if (mCategory != Category_Items)
        {
            for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
            {
                const ESM::Spell* spell = it->first;

                if (mCategory < Category_Items &&
                    mCategory != MWMechanics::getSpellSchool(spell, mActor))
                    continue;

                if (spell->mData.mType != ESM::Spell::ST_Power && spell->mData.mType != ESM::Spell::ST_Spell)
                    continue;

                if (spell->mData.mType == ESM::Spell::ST_Power && mCategory < Category_Powers)
                    continue;

                if (spell->mData.mType == ESM::Spell::ST_Spell 
                    && ( mCategory > Category_Restoration && mCategory < Category_All ))
                    continue;

                if (!mEffectFilter.empty())
                {
                    auto eList = spell->mEffects.mList;
                    bool found = false;
                    for (const auto& effect : eList)
                    {
                        const std::string effectStr = gmst.find(ESM::MagicEffect::effectIdToString(effect.mEffectID))->mValue.getString();
                        const auto ciEffect = Misc::StringUtils::lowerCase(effectStr);

                        if (ciEffect.find(mEffectFilter) != std::string::npos)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) 
                        continue;
                }

                std::string name = Misc::StringUtils::lowerCaseUtf8(spell->mName);

                if (name.find(filter) == std::string::npos)
                    continue;

                Spell newSpell;
                newSpell.mName = spell->mName;
                if (spell->mData.mType == ESM::Spell::ST_Spell)
                {
                    newSpell.mType = Spell::Type_Spell;
                    std::string cost = std::to_string(spell->mData.mCost);
                    std::string chance = std::to_string(int(MWMechanics::getSpellSuccessChance(spell, mActor)));
                    newSpell.mCostColumn = cost + "/" + chance;
                }
                else
                    newSpell.mType = Spell::Type_Power;

                newSpell.mId = spell->mId;

                newSpell.mSelected = (MWBase::Environment::get().getWindowManager()->getSelectedSpell() == spell->mId);
                newSpell.mActive = true;
                newSpell.mCount = 1;
                mSpells.push_back(newSpell);
            }
        }

        if (mCategory >= Category_All || mCategory == Category_Items)
        {
            MWWorld::InventoryStore& invStore = mActor.getClass().getInventoryStore(mActor);
            for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
            {
                MWWorld::Ptr item = *it;
                const std::string enchantId = item.getClass().getEnchantment(item);
                if (enchantId.empty())
                    continue;
                const ESM::Enchantment* enchant = esmStore.get<ESM::Enchantment>().search(enchantId);
                if (!enchant)
                {
                    Log(Debug::Warning) << "Warning: Can't find enchantment '" << enchantId << "' on item " << item.getCellRef().getRefId();
                    continue;
                }

                if (!mEffectFilter.empty())
                {
                    auto eList = enchant->mEffects.mList;
                    bool found = false;
                    for (const auto& effect : eList)
                    {
                        const std::string effectStr = gmst.find(ESM::MagicEffect::effectIdToString(effect.mEffectID))->mValue.getString();
                        const auto ciEffect = Misc::StringUtils::lowerCase(effectStr);

                        if (ciEffect.find(mEffectFilter) != std::string::npos)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found) 
                        continue;
                }

                if (enchant->mData.mType != ESM::Enchantment::WhenUsed && enchant->mData.mType != ESM::Enchantment::CastOnce)
                    continue;

                std::string name = Misc::StringUtils::lowerCaseUtf8(item.getClass().getName(item));

                if (name.find(filter) == std::string::npos)
                    continue;

                Spell newSpell;
                newSpell.mItem = item;
                newSpell.mId = item.getCellRef().getRefId();
                newSpell.mName = item.getClass().getName(item);
                newSpell.mCount = item.getRefData().getCount();
                newSpell.mType = Spell::Type_EnchantedItem;
                newSpell.mSelected = invStore.getSelectedEnchantItem() == it;

                // FIXME: move to mwmechanics
                if (enchant->mData.mType == ESM::Enchantment::CastOnce)
                {
                    newSpell.mCostColumn = "100/100";
                    newSpell.mActive = false;
                }
                else
                {
                    if (!item.getClass().getEquipmentSlots(item).first.empty()
                            && item.getClass().canBeEquipped(item, mActor).first == 0)
                        continue;

                    int castCost = MWMechanics::getEffectiveEnchantmentCastCost(static_cast<float>(enchant->mData.mCost), mActor);

                    std::string cost = std::to_string(castCost);
                    int currentCharge = int(item.getCellRef().getEnchantmentCharge());
                    if (currentCharge ==  -1)
                        currentCharge = enchant->mData.mCharge;
                    std::string charge = std::to_string(currentCharge);
                    newSpell.mCostColumn = cost + "/" + charge;

                    newSpell.mActive = invStore.isEquipped(item);
                }
                mSpells.push_back(newSpell);
            }
        }

        auto sorter = sortSpells;
        
        switch (mSort)
        {
        case Sort_School:
            sorter = sortSchool;
            break;
        //case Sort_CostChance:
        //    sorter = sortCostChance;
        //    break;
        }
        
        if (mIncreasing)
            std::stable_sort(mSpells.begin(), mSpells.end(), sorter);
        else 
            std::stable_sort(mSpells.rbegin(), mSpells.rend(), sorter);
    }

    size_t SpellModel::getItemCount() const
    {
        return mSpells.size();
    }

    SpellModel::ModelIndex SpellModel::getSelectedIndex() const
    {
        ModelIndex selected = -1;
        for (SpellModel::ModelIndex i = 0; i<int(getItemCount()); ++i)
        {
            if (getItem(i).mSelected) {
                selected = i;
                break;
            }
        }
        return selected;
    }

    Spell SpellModel::getItem(ModelIndex index) const
    {
        if (index < 0 || index >= int(mSpells.size()))
            throw std::runtime_error("invalid spell index supplied");
        return mSpells[index];
    }

    void SpellModel::setEffectFilter(const std::string filter)
    {
        mEffectFilter = Misc::StringUtils::lowerCase(filter);
    }

}
