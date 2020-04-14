#include "sortfilteritemmodel.hpp"

#include <components/misc/stringops.hpp>
#include <components/debug/debuglog.hpp>
#include <components/esm/loadalch.hpp>
#include <components/esm/loadappa.hpp>
#include <components/esm/loadarmo.hpp>
#include <components/esm/loadbook.hpp>
#include <components/esm/loadclot.hpp>
#include <components/esm/loadingr.hpp>
#include <components/esm/loadlock.hpp>
#include <components/esm/loadligh.hpp>
#include <components/esm/loadmisc.hpp>
#include <components/esm/loadprob.hpp>
#include <components/esm/loadrepa.hpp>
#include <components/esm/loadweap.hpp>
#include <components/esm/loadench.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/alchemy.hpp"
#include "../mwmechanics/actorutil.hpp"

namespace
{
    bool compareType(const std::string& type1, const std::string& type2)
    {
        // this defines the sorting order of types. types that are first in the vector appear before other types.
        std::vector<std::string> mapping;
        mapping.push_back( typeid(ESM::Weapon).name() );
        mapping.push_back( typeid(ESM::Armor).name() );
        mapping.push_back( typeid(ESM::Clothing).name() );
        mapping.push_back( typeid(ESM::Potion).name() );
        mapping.push_back( typeid(ESM::Ingredient).name() );
        mapping.push_back( typeid(ESM::Apparatus).name() );
        mapping.push_back( typeid(ESM::Book).name() );
        mapping.push_back( typeid(ESM::Light).name() );
        mapping.push_back( typeid(ESM::Miscellaneous).name() );
        mapping.push_back( typeid(ESM::Lockpick).name() );
        mapping.push_back( typeid(ESM::Repair).name() );
        mapping.push_back( typeid(ESM::Probe).name() );

        assert( std::find(mapping.begin(), mapping.end(), type1) != mapping.end() );
        assert( std::find(mapping.begin(), mapping.end(), type2) != mapping.end() );

        return std::find(mapping.begin(), mapping.end(), type1) < std::find(mapping.begin(), mapping.end(), type2);
    }

    bool CompareName(const MWGui::ItemStack& left, const MWGui::ItemStack& right)
    {
        if (left.mBase.isEmpty() || right.mBase.isEmpty()) return false;

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        // show barter items, then equipped items when sorting by name in ascending order
        auto rightBartered = right.mType == MWGui::ItemStack::Type_Barter;
        auto leftBartered = left.mType == MWGui::ItemStack::Type_Barter;
        if (rightBartered != leftBartered) {
            return rightBartered;
        }

        auto rightEquipped = store.isEquipped(right.mBase);
        auto leftEquipped = store.isEquipped(left.mBase);
        if (rightEquipped != leftEquipped) {
            return rightEquipped;
        }

        std::string leftName = Misc::StringUtils::lowerCase(left.mBase.getClass().getName(left.mBase));
        std::string rightName = Misc::StringUtils::lowerCase(right.mBase.getClass().getName(right.mBase));
        return leftName.compare(rightName) < 0; 
    }

    bool CompareValue(const MWGui::ItemStack& left, const MWGui::ItemStack& right)
    {
        if (left.mBase.isEmpty() || right.mBase.isEmpty()) return false;
        return left.mBase.getClass().getValue(left.mBase) > right.mBase.getClass().getValue(right.mBase);
    }

    bool CompareWeight(const MWGui::ItemStack& left, const MWGui::ItemStack& right)
    {
        if (left.mBase.isEmpty() || right.mBase.isEmpty()) return false;
        return (left.mBase.getClass().getWeight(left.mBase) * left.mCount) > (right.mBase.getClass().getWeight(right.mBase) * right.mCount);
    }

    bool CompareRatio(const MWGui::ItemStack& left, const MWGui::ItemStack& right)
    {
        if (left.mBase.isEmpty() || right.mBase.isEmpty()) return false;

        float lw = left.mBase.getClass().getWeight(left.mBase); 
        float lv =  left.mBase.getClass().getValue(left.mBase);
        float rw = right.mBase.getClass().getWeight(right.mBase);
        float rv = right.mBase.getClass().getValue(right.mBase);
        
        if (lw == 0)
            return true; 
        if (rw == 0)
            return false; 
        return (lv/lw) > (rv/rw);
    }

    bool CompareWeaponType(const MWGui::ItemStack& left, const MWGui::ItemStack& right)
    {
        if (left.mBase.isEmpty() || right.mBase.isEmpty()) return false;

        int result = left.mBase.get<ESM::Weapon>()->mBase->mData.mType - right.mBase.get<ESM::Weapon>()->mBase->mData.mType;  
        if (result != 0)
            return result < 0;
        else 
            return false;  
    }

    bool CompareArmorType(const MWGui::ItemStack& left, const MWGui::ItemStack& right)
    {
        if (left.mBase.isEmpty() || right.mBase.isEmpty()) return false;

        int result = left.mBase.getClass().getEquipmentSkill(left.mBase) - right.mBase.getClass().getEquipmentSkill(right.mBase);
        if (result != 0)
            return result > 0;
        else 
            return false;  
    }

    struct Compare
    {
        bool mSortByType;
        Compare(bool sortByType) : mSortByType(sortByType) {}
        bool operator() (const MWGui::ItemStack& left, const MWGui::ItemStack& right)
        {
            if (mSortByType && left.mType != right.mType)
                return left.mType < right.mType;

            float result = 0;

            // compare items by type
            std::string leftName = left.mBase.getTypeName();
            std::string rightName = right.mBase.getTypeName();

            if (leftName != rightName)
                return compareType(leftName, rightName);

            // compare items by name
            leftName = Misc::StringUtils::lowerCase(left.mBase.getClass().getName(left.mBase));
            rightName = Misc::StringUtils::lowerCase(right.mBase.getClass().getName(right.mBase));

            result = leftName.compare(rightName);
            if (result != 0)
                return result < 0;

            // compare items by enchantment:
            // 1. enchanted items showed before non-enchanted
            // 2. item with lesser charge percent comes after items with more charge percent
            // 3. item with constant effect comes before items with non-constant effects
            int leftChargePercent = -1;
            int rightChargePercent = -1;
            leftName = left.mBase.getClass().getEnchantment(left.mBase);
            rightName = right.mBase.getClass().getEnchantment(right.mBase);

            if (!leftName.empty())
            {
                const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(leftName);
                if (ench)
                {
                    if (ench->mData.mType == ESM::Enchantment::ConstantEffect)
                        leftChargePercent = 101;
                    else
                        leftChargePercent = static_cast<int>(left.mBase.getCellRef().getNormalizedEnchantmentCharge(ench->mData.mCharge) * 100);
                }
            }

            if (!rightName.empty())
            {
                const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(rightName);
                if (ench)
                {
                    if (ench->mData.mType == ESM::Enchantment::ConstantEffect)
                        rightChargePercent = 101;
                    else
                        rightChargePercent = static_cast<int>(right.mBase.getCellRef().getNormalizedEnchantmentCharge(ench->mData.mCharge) * 100);
                }
            }

            result = leftChargePercent - rightChargePercent;
            if (result != 0)
                return result > 0;

            // compare items by condition
            if (left.mBase.getClass().hasItemHealth(left.mBase) && right.mBase.getClass().hasItemHealth(right.mBase))
            {
                result = left.mBase.getClass().getItemHealth(left.mBase) - right.mBase.getClass().getItemHealth(right.mBase);
                if (result != 0)
                    return result > 0;
            }

            // compare items by remaining usage time
            result = left.mBase.getClass().getRemainingUsageTime(left.mBase) - right.mBase.getClass().getRemainingUsageTime(right.mBase);
            if (result != 0)
                return result > 0;

            // compare items by value
            result = left.mBase.getClass().getValue(left.mBase) - right.mBase.getClass().getValue(right.mBase);
            if (result != 0)
                return result > 0;

            // compare items by weight
            result = left.mBase.getClass().getWeight(left.mBase) - right.mBase.getClass().getWeight(right.mBase);
            if (result != 0)
                return result > 0;

            // compare items by Id
            leftName = left.mBase.getCellRef().getRefId();
            rightName = right.mBase.getCellRef().getRefId();

            result = leftName.compare(rightName);
            return result < 0;
        }
    };
}

namespace MWGui
{

    SortFilterItemModel::SortFilterItemModel(ItemModel *sourceModel)
        : mCategory(Category_All)
        , mSort(Sort_Default)
        , mFilter(0)
        , mSortByType(true)
        , mNameFilter("")
        , mEffectFilter("")
        , mIncreasing(true)
    {
        mSourceModel = sourceModel;
    }

    bool SortFilterItemModel::allowedToUseItems() const
    {
        return mSourceModel->allowedToUseItems();
    }

    void SortFilterItemModel::addDragItem (const MWWorld::Ptr& dragItem, size_t count)
    {
        mDragItems.push_back(std::make_pair(dragItem, count));
    }

    void SortFilterItemModel::clearDragItems()
    {
        mDragItems.clear();
    }

    void SortFilterItemModel::setSort(int sort)
    {
        if (mSort == sort)
             mIncreasing = !mIncreasing; 
         else 
             mIncreasing = true; 
         mSort = sort; 
    }
    
    int SortFilterItemModel::getSort() const 
    {
        return mSort; 
    }

    bool SortFilterItemModel::filterAccepts (const ItemStack& item)
    {
        MWWorld::Ptr base = item.mBase;
        int category = 0;

        if (base.getTypeName() == typeid(ESM::Armor).name()
                || base.getTypeName() == typeid(ESM::Clothing).name())
            category = Category_Apparel;
        else if (base.getTypeName() == typeid(ESM::Ingredient).name()
                     || base.getTypeName() == typeid(ESM::Potion).name())
            category = Category_Magic;
        if (base.getTypeName() == typeid(ESM::Armor).name())   
            category = Category_Armor;
        else if (base.getTypeName() == typeid(ESM::Weapon).name())
            category = Category_Weapon;
        else if (base.getTypeName() == typeid(ESM::Clothing).name())   
            category = Category_Cloth;
        else if (base.getTypeName() == typeid(ESM::Potion).name())   
            category = Category_Potion;
        else if (base.getTypeName() == typeid(ESM::Ingredient).name())   
            category = Category_Ingredient;
        else if (base.getTypeName() == typeid(ESM::Lockpick).name()
            || base.getTypeName() == typeid(ESM::Probe).name()
            || base.getTypeName() == typeid(ESM::Repair).name()
            || base.getTypeName() == typeid(ESM::Apparatus).name())   
            category = Category_Tool;
        else if (base.getTypeName() == typeid(ESM::Book).name())   
        {
            if (base.get<ESM::Book>()->mBase->mData.mIsScroll && (item.mFlags & ItemStack::Flag_Enchanted))
                category = Category_Magic;
            else
                category = Category_Book;
        }
        else if ( base.getTypeName() == typeid(ESM::Light).name())
            category = Category_Misc;
        else if (base.getTypeName() == typeid(ESM::Miscellaneous).name())   
        {
            if (base.getCellRef().getSoul() != "")
                category = Category_Magic;
            else 
                category = Category_Misc;
        }

        if (item.mFlags & ItemStack::Flag_Enchanted)
            category |= Category_Magic;

        if (!(category & mCategory) && (mCategory != Category_Simple))
            return false;

        if (mFilter & Filter_OnlyIngredients)
        {
            if (base.getTypeName() != typeid(ESM::Ingredient).name())
                return false;

            if (!mNameFilter.empty() && !mEffectFilter.empty())
                throw std::logic_error("name and magic effect filter are mutually exclusive");

            if (!mNameFilter.empty())
            {
                const auto itemName = Misc::StringUtils::lowerCase(base.getClass().getName(base));
                return itemName.find(mNameFilter) != std::string::npos;
            }

            if (!mEffectFilter.empty())
            {
                MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
                const auto alchemySkill = player.getClass().getSkill(player, ESM::Skill::Alchemy);

                const auto effects = MWMechanics::Alchemy::effectsDescription(base, alchemySkill);

                for (const auto& effect : effects)
                {
                    const auto ciEffect = Misc::StringUtils::lowerCase(effect);

                    if (ciEffect.find(mEffectFilter) != std::string::npos)
                        return true;
                }
                return false;
            }
            return true;
        }

        if ((mFilter & Filter_OnlyEnchanted) && !(item.mFlags & ItemStack::Flag_Enchanted))
            return false;
        if ((mFilter & Filter_OnlyChargedSoulstones) && (base.getTypeName() != typeid(ESM::Miscellaneous).name()
                                                     || base.getCellRef().getSoul() == "" || !MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().search(base.getCellRef().getSoul())))
            return false;
        if ((mFilter & Filter_OnlyRepairTools) && (base.getTypeName() != typeid(ESM::Repair).name()))
            return false;
        if ((mFilter & Filter_OnlyEnchantable) && (item.mFlags & ItemStack::Flag_Enchanted
                                               || (base.getTypeName() != typeid(ESM::Armor).name()
                                                   && base.getTypeName() != typeid(ESM::Clothing).name()
                                                   && base.getTypeName() != typeid(ESM::Weapon).name()
                                                   && base.getTypeName() != typeid(ESM::Book).name())))
            return false;
        if ((mFilter & Filter_OnlyEnchantable) && base.getTypeName() == typeid(ESM::Book).name()
                && !base.get<ESM::Book>()->mBase->mData.mIsScroll)
            return false;

        if ((mFilter & Filter_OnlyUsableItems) && base.getClass().getScript(base).empty())
        {
            std::shared_ptr<MWWorld::Action> actionOnUse = base.getClass().use(base);
            if (!actionOnUse || actionOnUse->isNullAction())
                return false;
        }

        if ((mFilter & Filter_OnlyRepairable) && (
                    !base.getClass().hasItemHealth(base)
                    || (base.getClass().getItemHealth(base) == base.getClass().getItemMaxHealth(base))
                    || (base.getTypeName() != typeid(ESM::Weapon).name()
                        && base.getTypeName() != typeid(ESM::Armor).name())))
            return false;

        if (mFilter & Filter_OnlyRechargable)
        {
            if (!(item.mFlags & ItemStack::Flag_Enchanted))
                return false;

            std::string enchId = base.getClass().getEnchantment(base);
            const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(enchId);
            if (!ench)
            {
                Log(Debug::Warning) << "Warning: Can't find enchantment '" << enchId << "' on item " << base.getCellRef().getRefId();
                return false;
            }

            if (base.getCellRef().getEnchantmentCharge() >= ench->mData.mCharge
                    || base.getCellRef().getEnchantmentCharge() == -1)
                return false;
        }

        std::string compare = Misc::StringUtils::lowerCase(item.mBase.getClass().getName(item.mBase));
        if(compare.find(mNameFilter) == std::string::npos)
            return false;

        return true;
    }

    ItemStack SortFilterItemModel::getItem (ModelIndex index)
    {
        if (index < 0)
            throw std::runtime_error("Invalid index supplied");
        if (mItems.size() <= static_cast<size_t>(index))
            throw std::runtime_error("Item index out of range");
        return mItems[index];
    }

    size_t SortFilterItemModel::getItemCount()
    {
        return mItems.size();
    }

    void SortFilterItemModel::setCategory (int category)
    {
        mCategory = category;
    }

    int SortFilterItemModel::getCategory () const 
    {
        return mCategory;
    }

    void SortFilterItemModel::setFilter (int filter)
    {
        mFilter = filter;
    }

    void SortFilterItemModel::setNameFilter (const std::string& filter)
    {
        mNameFilter = Misc::StringUtils::lowerCase(filter);
    }

    void SortFilterItemModel::setEffectFilter (const std::string& filter)
    {
        mEffectFilter = Misc::StringUtils::lowerCase(filter);
    }

    void SortFilterItemModel::update()
    {
        mSourceModel->update();

        size_t count = mSourceModel->getItemCount();

        mItems.clear();
        for (size_t i=0; i<count; ++i)
        {
            ItemStack item = mSourceModel->getItem(i);

            for (std::vector<std::pair<MWWorld::Ptr, size_t> >::iterator it = mDragItems.begin(); it != mDragItems.end(); ++it)
            {
                if (item.mBase == it->first)
                {
                    if (item.mCount < it->second)
                        throw std::runtime_error("Dragging more than present in the model");
                    item.mCount -= it->second;
                }
            }

            if (item.mCount > 0 && filterAccepts(item))
                mItems.push_back(item);
        }

        updateSort();
    }

    void SortFilterItemModel::updateSort()
    {
        switch(mSort)
        {
            case Sort_Name:
            {
                if (mIncreasing)
                    std::sort(mItems.begin(), mItems.end(), CompareName);
                else 
                    std::sort(mItems.rbegin(), mItems.rend(), CompareName);
                break;
            }
            case Sort_Value:
            {
                if (mIncreasing)
                    std::sort(mItems.begin(), mItems.end(), CompareValue);
                else 
                    std::sort(mItems.rbegin(), mItems.rend(), CompareValue);
                break;
            }   
            case Sort_Weight:
            {
                if (mIncreasing)
                    std::sort(mItems.begin(), mItems.end(), CompareWeight);
                else 
                    std::sort(mItems.rbegin(), mItems.rend(), CompareWeight);
                break;
            }
            case Sort_Ratio:
            {
                if (mIncreasing)
                    std::sort(mItems.begin(), mItems.end(), CompareRatio);
                else 
                    std::sort(mItems.rbegin(), mItems.rend(), CompareRatio);
                break;
            }
            case Sort_WeaponType:
            {
                if (mIncreasing)
                    std::sort(mItems.begin(), mItems.end(), CompareWeaponType);
                else 
                    std::sort(mItems.rbegin(), mItems.rend(), CompareWeaponType);
                break;
            }
            case Sort_ArmorType:
            {
                if (mIncreasing)
                    std::sort(mItems.begin(), mItems.end(), CompareArmorType);
                else 
                    std::sort(mItems.rbegin(), mItems.rend(), CompareArmorType);
                break;
            }
            default: 
            {
                if (mIncreasing)
                    std::sort(mItems.begin(), mItems.end(), Compare(mSortByType));
                else 
                    std::sort(mItems.rbegin(), mItems.rend(), Compare(mSortByType));
            }
        }
    }

    void SortFilterItemModel::onClose()
    {
        mSourceModel->onClose();
    }

    bool SortFilterItemModel::onDropItem(const MWWorld::Ptr &item, int count)
    {
        return mSourceModel->onDropItem(item, count);
    }

    bool SortFilterItemModel::onTakeItem(const MWWorld::Ptr &item, int count)
    {
        return mSourceModel->onTakeItem(item, count);
    }
}
