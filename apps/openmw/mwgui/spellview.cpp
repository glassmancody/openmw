#include "spellview.hpp"

#include <MyGUI_FactoryManager.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

#include <components/widgets/sharedstatebutton.hpp>
#include <components/widgets/box.hpp>

#include "tooltips.hpp"

namespace MWGui
{

    const char* SpellView::sSpellModelIndex = "SpellModelIndex";

    SpellView::LineInfo::LineInfo(SpellListWidget* widget, SpellModel::ModelIndex spellIndex)
        : mItem(widget)
        , mSpellIndex(spellIndex)
    {

    }

    SpellView::SpellView()
        : mScrollView(nullptr)
        , mShowCostColumn(true)
        , mHighlightSelected(true)
        , mHeader(nullptr)
    {
    }

    void SpellView::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mScrollView, "ScrollView");
        if (mScrollView == nullptr)
            throw std::runtime_error("Item view needs a scroll view");
        assignWidget(mHeader, "Header");
        if (mHeader == nullptr)
            throw std::runtime_error("Item view needs a header");

        mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    }

    void SpellView::registerComponents()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<SpellView>("Widget");
    }

    void SpellView::setModel(SpellModel *model)
    {
        mModel.reset(model);
        update();
    }

    SpellModel* SpellView::getModel()
    {
        return mModel.get();
    }

    void SpellView::setShowCostColumn(bool show)
    {
        if (show != mShowCostColumn)
        {
            mShowCostColumn = show;
            update();
        }
    }

    void SpellView::setHighlightSelected(bool highlight)
    {
        if (highlight != mHighlightSelected)
        {
            mHighlightSelected = highlight;
            update();
        }
    }

    void SpellView::update()
    {
        if (!mModel.get())
            return;

        mModel->update();

        int curType = -1;

        const int spellHeight = 18;

        mLines.clear();

        while (mScrollView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));

        while (mHeader->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mHeader->getChildAt(0));

        int category = mModel->getCategory();

        if (category == MWGui::SpellModel::Category_All)
            mHeader->changeWidgetSkin("MW_SpellListHeader_All"); 
        else if (category == MWGui::SpellModel::Category_Simple)
            mHeader->changeWidgetSkin("MW_SpellListHeader_Simple"); 
        else 
            mHeader->changeWidgetSkin("MW_SpellListHeader"); 

        for (SpellModel::ModelIndex i = 0; i<int(mModel->getItemCount()); ++i)
        {
            const Spell& spell = mModel->getItem(i);

            SpellListWidget* spellWidget = mScrollView->createWidget<SpellListWidget>("MW_SpellList",
                MyGUI::IntCoord(0,0,mScrollView->getWidth(), 35), MyGUI::Align::HStretch | MyGUI::Align::Top);

            spellWidget->setSpell(spell, category);
            mLines.push_back(LineInfo(spellWidget,i));
            adjustSpellWidget(spell,i,spellWidget);
        }

        layoutWidgets();
    }

    void SpellView::incrementalUpdate()
    {
        if (!mModel.get())
            return;
        update();
    }

    ItemListWidgetHeader* SpellView::getHeader()
    {
        return mHeader;
    }

    void SpellView::layoutWidgets()
    {
        int height = 0;
        for (LineInfo& line : mLines)
        {
            height += line.mItem->getHeight();
        }

        bool scrollVisible = height > mScrollView->getHeight();
        int rightMargin = (scrollVisible) ? 45 : 10;
        mHeader->setSize(MyGUI::IntSize(mScrollView->getWidth()-rightMargin,36));
        
        height = 0;
        for (LineInfo& line : mLines)
        {
            int lineHeight = line.mItem->getHeight();
            line.mItem->setCoord(MyGUI::IntCoord(
                0,height, mScrollView->getWidth()-rightMargin, lineHeight));
            height += lineHeight;
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mScrollView->setVisibleVScroll(false);
        mScrollView->setCanvasSize(mScrollView->getWidth(), std::max(mScrollView->getHeight(), height));
        mScrollView->setVisibleVScroll(true);
    }

    void SpellView::setSize(const MyGUI::IntSize &_value)
    {
        bool changed = (_value.width != getWidth() || _value.height != getHeight());
        Base::setSize(_value);
        if (changed)
            layoutWidgets();
    }

    void SpellView::setCoord(const MyGUI::IntCoord &_value)
    {
        bool changed = (_value.width != getWidth() || _value.height != getHeight());
        Base::setCoord(_value);
        if (changed)
            layoutWidgets();
    }

    void SpellView::adjustSpellWidget(const Spell &spell, SpellModel::ModelIndex index, SpellListWidget *widget)
    {
        if (spell.mType == Spell::Type_EnchantedItem)
        {
            widget->setUserData(MWWorld::Ptr(spell.mItem));
            widget->setUserString("ToolTipType", "ItemPtr");
        }
        else
        {
            widget->setUserString("ToolTipType", "Spell");
            widget->setUserString("Spell", spell.mId);
        }

        widget->setUserString(sSpellModelIndex, MyGUI::utility::toString(index));
        widget->eventMouseWheel += MyGUI::newDelegate(this, &SpellView::onMouseWheelMoved);
        widget->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellView::onSpellSelected);
        widget->setNeedKeyFocus(true);
        widget->setNeedMouseFocus(true);
        widget->eventKeyButtonPressed += MyGUI::newDelegate(this, &SpellView::onKeyButtonPressed);
        widget->eventItemFocused += MyGUI::newDelegate(this, &SpellView::onItemFocused);
    }

    void SpellView::onKeyButtonPressed(MyGUI::Widget *sender, MyGUI::KeyCode key, MyGUI::Char character)
    {
        SpellModel::ModelIndex index = getSpellModelIndex(sender);

        if (key == MyGUI::KeyCode::ArrowUp || key == MyGUI::KeyCode::ArrowDown)
        {
            if (mLines.size() > 0)
            {
                int count = mLines.size();
                for (auto i = 0; i < count; i++)
                    mLines[i].mItem->setStateFocused(false);

                if (index < 0 || index > count - 1)
                    index = 0;

                if (key == MyGUI::KeyCode::ArrowUp)
                {
                    if (index < 2)
                        index = 0;
                    else 
                        index -= 1;
                }
                else
                {
                    if (index >= count - 2)
                        index = count - 1;
                    else 
                        index += 1;
                }

                SpellListWidget* w = mLines[index].mItem;
                w->setStateFocused(true);
                onItemFocused(w);

                // make sure we adjust for scrolling, this is just an approximation 
                if (mScrollView->isVisibleVScroll())
                {
                    // how many items the scrollview can show 
                    int maxItems = mScrollView->getHeight() / w->getHeight();
                    int minIndex = std::ceil((-mScrollView->getViewOffset().top / static_cast<float>(w->getHeight())));
                    int maxIndex = minIndex + maxItems - 1;

                    // this is *not* a scroll-to, it assumes the item is already in view 
                    if (index > maxIndex)
                    {
                        mScrollView->setViewOffset(MyGUI::IntPoint(0,static_cast<int>(mScrollView->getViewOffset().top - w->getHeight())));
                    }
                    else if (index < minIndex)
                    {
                        mScrollView->setViewOffset(MyGUI::IntPoint(0,static_cast<int>(mScrollView->getViewOffset().top + w->getHeight())));
                    }
                }
            }
        }
        else if (key == MyGUI::KeyCode::Return)
        {
            dynamic_cast<SpellListWidget*>(sender)->setStateSelected(true);
            eventSpellClicked(index);
        }
    }

    void SpellView::onItemFocused(SpellListWidget* item)
    {
        return;
        for (auto line : mLines)
        {
            if (line.mItem != item)
                line.mItem->setStateFocused(false);
        }
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(item);
        onSpellSelected(item);
    }


    SpellModel::ModelIndex SpellView::getSpellModelIndex(MyGUI::Widget* widget)
    {
        return MyGUI::utility::parseInt(widget->getUserString(sSpellModelIndex));
    }

    void SpellView::onSpellSelected(MyGUI::Widget* _sender)
    {
        dynamic_cast<SpellListWidget*>(_sender)->setStateSelected(true);
        eventSpellClicked(getSpellModelIndex(_sender));
    }

    void SpellView::onMouseWheelMoved(MyGUI::Widget* _sender, int _rel)
    {
        if (mScrollView->getViewOffset().top + _rel*0.3f > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(MyGUI::IntPoint(0, static_cast<int>(mScrollView->getViewOffset().top + _rel*0.3f)));
    }

    void SpellView::resetScrollbars()
    {
        mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
    }
}
