#ifndef OPENMW_GAME_MWGUI_WEATHER_H
#define OPENMW_GAME_MWGUI_WEATHER_H

#include <map>
#include <string>

#include <osg/Vec4f>

#include <MyGUI_Gui.h>
#include <MyGUI_Widget.h>
#include <MyGUI_Delegate.h>

#include "windowpinnablebase.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <components/resource/resourcesystem.hpp>

namespace MyGUI {
    class Widget;
    class TextBox;
    class ScrollBar;
    class Button;
    class ImageBox;
}

namespace MWGui
{

    class ColorPicker final : public MyGUI::Widget 
    {
        MYGUI_RTTI_DERIVED(ColorPicker)

    public:
        ColorPicker();

        static void registerComponents();

        osg::Vec4f getColor(); 
        void setColor(const osg::Vec4f& rgb);

        typedef MyGUI::delegates::CMultiDelegate1<ColorPicker*> EventHandle_Widget;

        EventHandle_Widget eventColorChanged;

        void setColorWheels(int weather);

    protected:
        void initialiseOverride() final;
        void onMouseClick(MyGUI::Widget* _sender);
        void onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onValueSliderChanged(MyGUI::ScrollBar* _sender, size_t pos);

    private:
        MyGUI::ImageBox* mHuePicker;
        MyGUI::ImageBox* mHue;
        MyGUI::ScrollBar* mValueSlider;
        MyGUI::ImageBox* mValueSliderBG;
    };


    class Weather : public WindowPinnableBase
    {
    public:
        Weather(Resource::ResourceSystem* resourceSystem);

        virtual bool exit();

        virtual void onPinToggled() {}

        void update();

    protected:
        void onWeatherButtonClicked(MyGUI::Widget* _sender);
        void onHourSliderChanged(MyGUI::ScrollBar* _sender, size_t pos);
        void onTimeButtonClicked(MyGUI::Widget* _sender);
        void onTimeChanged();
        void onColorChanged(ColorPicker* _sender);
        void onIntColorChanged(ColorPicker* _sender);
        void onKeyPressed(MyGUI::Widget *sender, MyGUI::KeyCode key, MyGUI::Char character);

        enum TimeInterval {
            Day,
            Sunrise,
            Sunset,
            Night
        };

        void setColorWheels(int id);

    private:
        // (name, (fallback, color))
        std::map<std::string, std::map<std::string, osg::Vec4f>> mProfiles;

        Resource::ResourceSystem* mResourceSystem;
        std::map<MyGUI::Widget*, int> mWeatherButtons; 
        MyGUI::ScrollBar* mHourSlider;
        MyGUI::TextBox* mTimeText;
        MyGUI::Button* mTimeNight;
        MyGUI::Button* mTimeSunrise;
        MyGUI::Button* mTimeDay;
        MyGUI::Button* mTimeSunset;

        ColorPicker* mSkyPicker;
        ColorPicker* mFogPicker;
        ColorPicker* mSunPicker;
        ColorPicker* mAmbientPicker;
        ColorPicker* mSunDisc;
        ColorPicker* mIntSunPicker;
        ColorPicker* mIntAmbientPicker;
        ColorPicker* mIntFogPicker;

        TimeInterval mTimeInterval;

        static std::map<TimeInterval, std::string> mTimeMappings;
        static std::map<int, std::string> mWeatherMappings;

        int mCurrentID; // weather id 
    };


}

#endif 
