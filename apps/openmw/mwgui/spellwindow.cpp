#include "spellwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Window.h>

#include <components/misc/stringops.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/spellutil.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "spellicons.hpp"
#include "confirmationdialog.hpp"
#include "spellview.hpp"

namespace MWGui
{

    SpellWindow::SpellWindow(DragAndDrop* drag)
        : WindowPinnableBase("openmw_spell_window.layout")
        , NoDrop(drag, mMainWidget)
        , mSpellView(nullptr)
        , mSortModel(nullptr)
        , mUpdateTimer(0.0f)
    {
        mSpellIcons = new SpellIcons();

        MyGUI::Widget* deleteButton;
        getWidget(deleteButton, "DeleteSpellButton");

        getWidget(mSpellView, "SpellView");
        getWidget(mEffectBox, "EffectsBox");
        getWidget(mFilterEdit, "FilterEdit");
        getWidget(mFilterValue, "FilterValue");

        getWidget(mAllButton, "AllButton");
        getWidget(mPowersButton, "PowersButton");
        getWidget(mItemsButton, "ItemsButton");
        getWidget(mAlterationButton, "AlterationButton");
        getWidget(mConjurationButton, "ConjurationButton");
        getWidget(mDestructionButton, "DestructionButton");
        getWidget(mIllusionButton, "IllusionButton");
        getWidget(mMysticismButton, "MysticismButton");
        getWidget(mRestorationButton, "RestorationButton");
        getWidget(mCategories, "Categories");

        mSpellView->eventSpellClicked += MyGUI::newDelegate(this, &SpellWindow::onModelIndexSelected);
        mFilterEdit->eventEditTextChange += MyGUI::newDelegate(this, &SpellWindow::onNameFilterChanged);
        deleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onDeleteClicked);

        mFilterValue->eventComboChangePosition += MyGUI::newDelegate(this, &SpellWindow::onFilterChanged);
        mFilterValue->eventEditTextChange += MyGUI::newDelegate(this, &SpellWindow::onFilterEdited);

        mAllButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mPowersButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mItemsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mAlterationButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mConjurationButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mDestructionButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mIllusionButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mMysticismButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);
        mRestorationButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onCategoryFilterChanged);

        mSpellView->getHeader()->eventItemClicked += MyGUI::newDelegate(this, &SpellWindow::onHeaderClicked);

        setCoord(498, 300, 302, 300);

        // Adjust the spell filtering widget size because of MyGUI limitations.
        int filterWidth = mSpellView->getSize().width - deleteButton->getSize().width - 3;
        mFilterEdit->setSize(filterWidth, mFilterEdit->getSize().height);
    }

    void SpellWindow::onHeaderClicked(int sort)
    {
        mSortModel->toggleSort(sort);
        mSpellView->update();
    }

    void SpellWindow::adjustCategoryHeader()
    {
        mCategories->setCoord(8,36,mCategories->getWidth(), mCategories->getHeight());
        static int maxPadding = MyGUI::utility::parseInt(mCategories->getUserString("MaxPadding"));
        static int maxSize = MyGUI::utility::parseInt(mCategories->getUserString("MaxSize"));
        static int minMargin = MyGUI::utility::parseInt(mCategories->getUserString("MinMargin"));
        static int minSize = MyGUI::utility::parseInt(mCategories->getUserString("MinSize"));
        static int padding = MyGUI::utility::parseInt(mCategories->getUserString("Padding"));
        
        int count = mCategories->getChildCount();

        int width = std::min(maxSize,std::max(static_cast<int>(((mCategories->getWidth()-(2*minMargin)-(padding*count)) / static_cast<float>(count))), minSize));
        int sidemargin = ((mCategories->getWidth() - ((width+padding) * count))/2) + 8; 
        
        if (sidemargin < 0)
            sidemargin = minMargin;

        MyGUI::Widget* widget = mCategories->getChildAt(0);
        widget->setCoord(MyGUI::IntCoord(sidemargin,widget->getTop(),width,width));
        for (size_t i = 1; i < count; i++)
        {
            widget = mCategories->getChildAt(i);
            widget->setCoord(MyGUI::IntCoord(mCategories->getChildAt(i-1)->getLeft()+width+padding,widget->getTop(),width,width));
        }
    }

    SpellWindow::~SpellWindow()
    {
        delete mSpellIcons;
    }

    void SpellWindow::onPinToggled()
    {
        Settings::Manager::setBool("spells pin", "Windows", mPinned);

        MWBase::Environment::get().getWindowManager()->setSpellVisibility(!mPinned);
    }

    void SpellWindow::onCategoryFilterChanged(MyGUI::Widget* _sender)
    {
        setTitle(_sender->getUserString("Title"));
        if (_sender == mAllButton)
            mSortModel->setCategory(SpellModel::Category_All);
        if (_sender == mPowersButton)
            mSortModel->setCategory(SpellModel::Category_Powers);
        if (_sender == mItemsButton)
            mSortModel->setCategory(SpellModel::Category_Items);
        if (_sender == mAlterationButton)
            mSortModel->setCategory(SpellModel::Category_Alteration);
        if (_sender == mConjurationButton)
            mSortModel->setCategory(SpellModel::Category_Conjuration);
        if (_sender == mDestructionButton)
            mSortModel->setCategory(SpellModel::Category_Destruction);
        if (_sender == mIllusionButton)
            mSortModel->setCategory(SpellModel::Category_Illusion);
        if (_sender == mMysticismButton)
            mSortModel->setCategory(SpellModel::Category_Mysticism);
        if (_sender == mRestorationButton)
            mSortModel->setCategory(SpellModel::Category_Restoration);

        mAllButton->setStateSelected(false);
        mPowersButton->setStateSelected(false);
        mItemsButton->setStateSelected(false);
        mAlterationButton->setStateSelected(false);
        mConjurationButton->setStateSelected(false);
        mDestructionButton->setStateSelected(false);
        mIllusionButton->setStateSelected(false);
        mMysticismButton->setStateSelected(false);
        mRestorationButton->setStateSelected(false);


        mSpellView->update();
        updateFilterEffect();

        _sender->castType<Gui::ImagePushButton>()->setStateSelected(true);
    }

    void SpellWindow::updateFilterEffect()
    {
        const auto& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        const auto& esm = MWBase::Environment::get().getWorld()->getStore();

        std::set<std::string> effects; 

        for (size_t i = 0; i < mSortModel->getItemCount(); i++)
        {
            auto item = mSortModel->getItem(i);

            if (item.mType == Spell::Type_EnchantedItem)
            {
                const std::string enchantId = item.mItem.getClass().getEnchantment(item.mItem);
                const ESM::Enchantment* enchant = esm.get<ESM::Enchantment>().search(enchantId);
            
                for (const ESM::ENAMstruct& spellEffect : enchant->mEffects.mList)
                {
                    std::string name = gmst.find(ESM::MagicEffect::effectIdToString(spellEffect.mEffectID))->mValue.getString();
                    effects.insert(name);
                }
            }
            else 
            {
                auto spell = esm.get<ESM::Spell>().find(item.mId);
                for (const ESM::ENAMstruct& spellEffect : spell->mEffects.mList)
                {
                    std::string name = gmst.find(ESM::MagicEffect::effectIdToString(spellEffect.mEffectID))->mValue.getString();
                    effects.insert(name);
                }
            }
        }
        mFilterEdit->setCaption({});
        mFilterValue->setCaption({});
        mSortModel->setEffectFilter({});
        mFilterValue->removeAllItems();
        for (auto e : effects)
            mFilterValue->addItem(e);
    }

    void SpellWindow::applyFilter(const std::string& filter)
    {
        mSortModel->setEffectFilter(filter);
        mSpellView->update();
    }

    void SpellWindow::onFilterChanged(MyGUI::ComboBox* _sender, size_t _index)
    {
        if (_index != MyGUI::ITEM_NONE)
            applyFilter(_sender->getItemNameAt(_index));
    }

    void SpellWindow::onFilterEdited(MyGUI::EditBox* _sender)
    {
        applyFilter(_sender->getCaption());
    }

    void SpellWindow::onTitleDoubleClicked()
    {
        if (MyGUI::InputManager::getInstance().isShiftPressed())
            MWBase::Environment::get().getWindowManager()->toggleMaximized(this);
        else if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Magic);
    }

    void SpellWindow::onOpen()
    {
        setTitle(mAllButton->getUserString("Title"));
        // Reset the filter focus when opening the window
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        if (focus == mFilterEdit)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nullptr);

        updateSpells();
        updateFilterEffect();
    }

    void SpellWindow::onFrame(float dt) 
    {
        NoDrop::onFrame(dt);
        mUpdateTimer += dt;
        if (0.5f < mUpdateTimer)
        {
            mUpdateTimer = 0;
            mSpellView->incrementalUpdate();
        }

        // Update effects in-game too if the window is pinned
        if (mPinned && !MWBase::Environment::get().getWindowManager()->isGuiMode())
            mSpellIcons->updateWidgets(mEffectBox, false);

        adjustCategoryHeader();
    }

    void SpellWindow::updateSpells()
    { 
        mSpellIcons->updateWidgets(mEffectBox, false);

        if (!mSortModel)
        {
            mSortModel = new SpellModel(MWMechanics::getPlayer());
            mSpellView->setModel(mSortModel);
            onCategoryFilterChanged(mAllButton);
        }

        mSpellView->update();
    }

    void SpellWindow::onEnchantedItemSelected(MWWorld::Ptr item, bool alreadyEquipped)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        // retrieve ContainerStoreIterator to the item
        MWWorld::ContainerStoreIterator it = store.begin();
        for (; it != store.end(); ++it)
        {
            if (*it == item)
            {
                break;
            }
        }
        if (it == store.end())
            throw std::runtime_error("can't find selected item");

        // equip, if it can be equipped and is not already equipped
        if (!alreadyEquipped
            && !item.getClass().getEquipmentSlots(item).first.empty())
        {
            MWBase::Environment::get().getWindowManager()->useItem(item);
            // make sure that item was successfully equipped
            if (!store.isEquipped(item))
                return;
        }

        store.setSelectedEnchantItem(it);
        // to reset WindowManager::mSelectedSpell immediately
        MWBase::Environment::get().getWindowManager()->setSelectedEnchantItem(*it);

        updateSpells();
    }

    void SpellWindow::askDeleteSpell(const std::string &spellId)
    {
        // delete spell, if allowed
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        std::string raceId = player.get<ESM::NPC>()->mBase->mRace;
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(raceId);
        // can't delete racial spells, birthsign spells or powers
        bool isInherent = race->mPowers.exists(spell->mId) || spell->mData.mType == ESM::Spell::ST_Power;
        const std::string& signId = MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();
        if (!isInherent && !signId.empty())
        {
            const ESM::BirthSign* sign = MWBase::Environment::get().getWorld()->getStore().get<ESM::BirthSign>().find(signId);
            isInherent = sign->mPowers.exists(spell->mId);
        }

        if (isInherent)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sDeleteSpellError}");
        }
        else
        {
            // ask for confirmation
            mSpellToDelete = spellId;
            ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
            std::string question = MWBase::Environment::get().getWindowManager()->getGameSettingString("sQuestionDeleteSpell", "Delete %s?");
            question = Misc::StringUtils::format(question, spell->mName);
            dialog->askForConfirmation(question);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &SpellWindow::onDeleteSpellAccept);
            dialog->eventCancelClicked.clear();
        }
    }

    void SpellWindow::onModelIndexSelected(SpellModel::ModelIndex index)
    {
        const Spell& spell = mSpellView->getModel()->getItem(index);
        if (spell.mType == Spell::Type_EnchantedItem)
        {
            onEnchantedItemSelected(spell.mItem, spell.mActive);
        }
        else
        {
            if (MyGUI::InputManager::getInstance().isShiftPressed())
                askDeleteSpell(spell.mId);
            else
                onSpellSelected(spell.mId);
        }
    }

    void SpellWindow::onNameFilterChanged(MyGUI::EditBox *sender)
    {
        mSpellView->getModel()->setNameFilter(sender->getCaption());
        mSpellView->update();
    }

    void SpellWindow::onDeleteClicked(MyGUI::Widget *widget)
    {
        SpellModel::ModelIndex selected = mSpellView->getModel()->getSelectedIndex();
        if (selected < 0)
            return;

        const Spell& spell = mSpellView->getModel()->getItem(selected);
        if (spell.mType != Spell::Type_EnchantedItem)
            askDeleteSpell(spell.mId);
    }

    void SpellWindow::onSpellSelected(const std::string& spellId)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        store.setSelectedEnchantItem(store.end());
        MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));

        updateSpells();
    }

    void SpellWindow::onDeleteSpellAccept()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell() == mSpellToDelete)
            MWBase::Environment::get().getWindowManager()->unsetSelectedSpell();

        spells.remove(mSpellToDelete);

        updateSpells();
    }

    void SpellWindow::cycle(bool next)
    {

        MWWorld::Ptr player = MWMechanics::getPlayer();

        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player))
            return;

        const MWMechanics::CreatureStats &stats = player.getClass().getCreatureStats(player);
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead() || stats.getHitRecovery())
            return;

        updateSpells();

        SpellModel::ModelIndex selected = mSpellView->getModel()->getSelectedIndex();
        if (selected < 0)
            selected = 0;

        selected += next ? 1 : -1;
        int itemcount = mSpellView->getModel()->getItemCount();
        if (itemcount == 0)
            return;
        selected = (selected + itemcount) % itemcount;

        const Spell& spell = mSpellView->getModel()->getItem(selected);
        if (spell.mType == Spell::Type_EnchantedItem)
            onEnchantedItemSelected(spell.mItem, spell.mActive);
        else
            onSpellSelected(spell.mId);
    }
}
