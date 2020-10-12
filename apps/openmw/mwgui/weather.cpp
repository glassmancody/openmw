#include <QStringList>
#include <QString>
#include <QDebug>
#include <QVariant>
#include <QMultiMap>


#include <MyGUI_FactoryManager.h>
#include "MyGUI_EditText.h"
#include "MyGUI_StringUtility.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_Widget.h"
#include "MyGUI_Button.h"
#include "MyGUI_ScrollBar.h"

#include "MyGUI_TextBox.h"
#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

#include "weather.hpp"

#include <osg/Vec4f>

#include <components/config/gamesettings.hpp>
#include <components/resource/scenemanager.hpp>

#include "../mwrender/renderingmanager.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/timestamp.hpp"

#include "../mwmechanics/actorutil.hpp"

#include "timeadvancer.hpp"

namespace {

    // rgb -> [0,1]
    // h -> [0, 360], s,v -> [0,1]
    osg::Vec4f rgb_to_hsv(const osg::Vec4f &in)
    {
        using std::min;
        using std::max;

        osg::Vec4f out;
        out[3] = 1.0;

        float fCMax = max(max(in[0], in[1]), in[2]);
        float fCMin = min(min(in[0], in[1]), in[2]);
        float fDelta = fCMax - fCMin;

        if (fDelta > 0)
        {
            if (fCMax == in[0])
            {
                out[0] = 60 * (fmod(((in[1] - in[2]) / fDelta), 6));
            }
            else if (fCMax == in[1])
            {
                out[0] = 60 * (((in[2] - in[0]) / fDelta) + 2);
            }
            else if (fCMax == in[2])
            {
                out[0] = 60 * (((in[0] - in[1]) / fDelta) + 4);
            }

            if (fCMax > 0)
            {
                out[1] = fDelta / fCMax;
            }
            else
            {
                out[1] = 0;
            }

            out[2] = fCMax;
        }
        else
        {
            out[0] = 0;
            out[1] = 0;
            out[2] = fCMax;
        }

        if (out[0] < 0)
        {
            out[0] = 360 + out[0];
        }
        return out;
    }

    // rgb -> [0,1]
    // h -> [0, 360], s,v -> [0,1]
    osg::Vec4f hsv_to_rgb(float &fH, float &fS, float &fV)
    {
        using std::min;
        using std::max;

        osg::Vec4f out;
        out[3] = 1.0;

        float fC = fV * fS; // Chroma
        float fHPrime = fmod(fH / 60.0, 6);
        float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
        float fM = fV - fC;

        if (0 <= fHPrime && fHPrime < 1)
        {
            out[0] = fC;
            out[1] = fX;
            out[2] = 0;
        }
        else if (1 <= fHPrime && fHPrime < 2)
        {
            out[0] = fX;
            out[1] = fC;
            out[2] = 0;
        }
        else if (2 <= fHPrime && fHPrime < 3)
        {
            out[0] = 0;
            out[1] = fC;
            out[2] = fX;
        }
        else if (3 <= fHPrime && fHPrime < 4)
        {
            out[0] = 0;
            out[1] = fX;
            out[2] = fC;
        }
        else if (4 <= fHPrime && fHPrime < 5)
        {
            out[0] = fX;
            out[1] = 0;
            out[2] = fC;
        }
        else if (5 <= fHPrime && fHPrime < 6)
        {
            out[0] = fC;
            out[1] = 0;
            out[2] = fX;
        }
        else
        {
            out[0] = 0;
            out[1] = 0;
            out[2] = 0;
        }

        out[0] += fM;
        out[1] += fM;
        out[2] += fM;
        return out;
    }
}

namespace MWGui
{
    std::map<Weather::TimeInterval, std::string> Weather::mTimeMappings = {
        {Day, "Day"},
        {Sunrise, "Sunrise"},
        {Sunset, "Sunset"},
        {Night, "Night"},
    };

  std::map<int, std::string> Weather::mWeatherMappings = {
        {0, "Clear"},
        {1, "Cloudy"},
        {2, "Foggy"},
        {3, "Overcast"},
        {4, "Rain"},
        {5, "Thunderstorm"},
        {6, "Ashstorm"},
        {7, "Blight"},
        {8, "Snow"},
        {9, "Blizzard"}
    };

    ColorPicker::ColorPicker()
    {
    }

    void ColorPicker::registerComponents()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::ColorPicker>("Widget");
    }

    void ColorPicker::setColor(const osg::Vec4f& rgb)
    {
         // float pickerWidth = myPickerImage.size.width;
        // float radius = pickerWidth / 2;
        // float colorRadius = saturation * radius;
        // float angle = (1 - hue) * (2 * pi);
        // float midX = myPicker.size.width / 2; //midpoint of the circle
        // float midY = myPicker.size.height / 2;
        // float xOffset = cos(angle) * colorRadius; //offset from the midpoint of the circle
        // float yOffset = sin(angle) * colorRadius;
        // Point colorPoint = new Point(midX + xOffset, midY + yOffset);
    }

    osg::Vec4f ColorPicker::getColor()
    {
        int x = mHuePicker->getPosition().left;
        int y = mHuePicker->getPosition().top;

        // angle from point to center
        float hue = osg::RadiansToDegrees(atan2(y,x)) + 90.0;

        if (x < 0 && y < 0)
            hue += 360; 

        // how far we are from circle as a percentage of the radius
        float sat = sqrt(pow(x,2) + pow(y,2)) / 110;
        
        float val = (255-mValueSlider->getScrollPosition()) / 255.0;

        osg::Vec4f col = hsv_to_rgb(hue, sat, val);

        return col;
    }

    void ColorPicker::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mHuePicker, "HuePicker");
        assignWidget(mHue, "Hue");
        assignWidget(mValueSlider, "ValueSlider");
        
        mHuePicker->setNeedMouseFocus(true);
        mHuePicker->eventMouseDrag += MyGUI::newDelegate(this, &ColorPicker::onMouseDrag);
        mHuePicker->eventMouseButtonPressed += MyGUI::newDelegate(this, &ColorPicker::onDragStart);
        mHuePicker->eventMouseButtonClick += MyGUI::newDelegate(this, &ColorPicker::onMouseClick);

        mValueSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &ColorPicker::onValueSliderChanged);

    }

    void ColorPicker::onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id!= MyGUI::MouseButton::Left && _id != MyGUI::MouseButton::Right) return;
    }

    void ColorPicker::onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id!=MyGUI::MouseButton::Left  && _id != MyGUI::MouseButton::Right ) return;

        MyGUI::IntCoord widgetPos = mHue->getAbsoluteCoord();
        MyGUI::IntPoint relMousePos = MyGUI::InputManager::getInstance ().getMousePosition () - MyGUI::IntPoint(widgetPos.left, widgetPos.top);

        int x = relMousePos.left - 110;
        int y = relMousePos.top - 110;

        x = std::min(std::max(-110, x), 110);
        y = std::min(std::max(-110, y), 110);
        
        if ((pow(x, 2) + pow(y, 2)) > pow(110, 2)) {
            int xn = (x > 0) ? 1 : -1;
            int yn = (y > 0) ? 1 : -1;
            x = sqrt(pow(110,2) - pow(y,2)) * xn;
            y = sqrt(pow(110,2) - pow(x,2)) * yn;
        }

        mHuePicker->setPosition(MyGUI::IntPoint(x, y));
        eventColorChanged(this);
    }

    void ColorPicker::onValueSliderChanged(MyGUI::ScrollBar* _sender, size_t pos)
    {
        eventColorChanged(this);
    }

    void ColorPicker::onMouseClick(MyGUI::Widget* _sender)
    {
        onMouseDrag(nullptr, 0,0, MyGUI::MouseButton::Left);
    }

    Weather::Weather(Resource::ResourceSystem* resourceSystem)
        : WindowPinnableBase("openmw_weather.layout")
        , mResourceSystem(resourceSystem)
        ,mTimeInterval(TimeInterval::Day) 
        ,mCurrentID(0)
    {
        center();

        MyGUI::Widget* w = nullptr;
        getWidget(w, "weather0");
        for (int i = 0; i < 5; i++) {
            mWeatherButtons.emplace(w->getChildAt(i), i);
            w->getChildAt(i)->eventMouseButtonClick += MyGUI::newDelegate(this, &Weather::onWeatherButtonClicked);
        }
        getWidget(w, "weather1");
        for (int i = 0; i < 5; i++) {
            mWeatherButtons.emplace(w->getChildAt(i), i+5);
            w->getChildAt(i)->eventMouseButtonClick += MyGUI::newDelegate(this, &Weather::onWeatherButtonClicked);
        }

        getWidget(mHourSlider, "HourSlider");
        mHourSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &Weather::onHourSliderChanged);
        mHourSlider->setScrollPosition(MWBase::Environment::get().getWorld()->getTimeStamp().getHour() * 100);

        getWidget(mTimeText, "TimeText");
        getWidget(mTimeNight, "TimeNight");
        getWidget(mTimeSunrise, "TimeSunrise");
        getWidget(mTimeDay, "TimeDay");
        getWidget(mTimeSunset, "TimeSunset");

        mTimeNight->eventMouseButtonClick += MyGUI::newDelegate(this, &Weather::onTimeButtonClicked);
        mTimeSunrise->eventMouseButtonClick += MyGUI::newDelegate(this, &Weather::onTimeButtonClicked);
        mTimeDay->eventMouseButtonClick += MyGUI::newDelegate(this, &Weather::onTimeButtonClicked);
        mTimeSunset->eventMouseButtonClick += MyGUI::newDelegate(this, &Weather::onTimeButtonClicked);

        getWidget(mSkyPicker, "SkyPicker");
        getWidget(mFogPicker, "FogPicker");
        getWidget(mSunPicker, "SunPicker");
        getWidget(mAmbientPicker, "AmbientPicker");

        mSkyPicker->eventColorChanged += MyGUI::newDelegate(this, &Weather::onColorChanged);
        mFogPicker->eventColorChanged += MyGUI::newDelegate(this, &Weather::onColorChanged);
        mSunPicker->eventColorChanged += MyGUI::newDelegate(this, &Weather::onColorChanged);
        mAmbientPicker->eventColorChanged += MyGUI::newDelegate(this, &Weather::onColorChanged);

        onTimeChanged();
    }

    void Weather::onColorChanged(ColorPicker* _sender)
    {
        std::string wtype = "";
        if (_sender == mSkyPicker)
            wtype="Sky";
        else if (_sender == mFogPicker)
            wtype="Fog";
        else if (_sender == mSunPicker)
            wtype="Sun";
        else if (_sender == mAmbientPicker)
            wtype="Ambient";
        else 
            return;

        std::string query = "Weather_" + mWeatherMappings[mCurrentID] + "_" + wtype + "_" + mTimeMappings[mTimeInterval] + "_Color"; 
        mResourceSystem->getGameSettings()->setWeatherColor(query, _sender->getColor());
    }

    bool Weather::exit()
    {
        return true;
    }

    void Weather::onTimeChanged()
    {
        float hourf = MWBase::Environment::get().getWorld()->getTimeStamp().getHour();
        int hour = static_cast<int>(hourf);
        bool pm = hour >= 12;
        if (hour >= 13) hour -= 12;
        if (hour == 0) hour = 12;
        float minutes = 60.0 * (hourf - int(hourf));
        std::string ampm = (pm) ? "PM" : "AM";
        mTimeText->setCaption(Misc::StringUtils::format("%d:%02.0f %s", hour, minutes, ampm));
        mHourSlider->setScrollPosition(hourf *  100);


        float sunrise = mResourceSystem->getGameSettings()->getTime("Weather_Sunrise_Time");
        float sunset = mResourceSystem->getGameSettings()->getTime("Weather_Sunset_Time"); 
        float sunrised = mResourceSystem->getGameSettings()->getTime("Weather_Sunrise_Duration");
        float sunsetd = mResourceSystem->getGameSettings()->getTime("Weather_Sunset_Duration"); 


        if (hourf > (sunrise + sunrised) && hourf < sunset)
            mTimeInterval = TimeInterval::Day; 
        else if (hourf >= sunrise && hourf <= (sunrise + sunrised) )
            mTimeInterval = TimeInterval::Sunrise;  
        else if (hourf >= sunset && hourf <= (sunset + sunsetd) )
            mTimeInterval = TimeInterval::Sunset; 
        else
            mTimeInterval = TimeInterval::Night;
        
        mCurrentID = MWBase::Environment::get().getWorld()->getCurrentWeather();
    }

    void Weather::onTimeButtonClicked(MyGUI::Widget* _sender) 
    {
        float hour = MWBase::Environment::get().getWorld()->getTimeStamp().getHour();
        float sunrise = mResourceSystem->getGameSettings()->getTime("Weather_Sunrise_Time");
        float sunset = mResourceSystem->getGameSettings()->getTime("Weather_Sunset_Time"); 

        if (_sender == mTimeNight)
            MWBase::Environment::get().getWorld()->advanceTime((sunrise - 3) - hour);
        else if (_sender == mTimeSunrise)
            MWBase::Environment::get().getWorld()->advanceTime(sunrise - hour);
        else if (_sender == mTimeDay)
            MWBase::Environment::get().getWorld()->advanceTime((sunrise + 6) - hour);
        else if (_sender == mTimeSunset)
            MWBase::Environment::get().getWorld()->advanceTime(sunset - hour);

        onTimeChanged();
    }

    void Weather::onHourSliderChanged(MyGUI::ScrollBar* _sender, size_t pos)
    {
        MWBase::Environment::get().getWorld()->advanceTime(((float)pos / 100.0) - MWBase::Environment::get().getWorld()->getTimeStamp().getHour());
        onTimeChanged();
    }

    //Log(Debug::Warning) << "\tTESTING: " << test;    //osg::Vec4f test = mResourceSystem->getGameSettings()->getWeatherColor("Weather_Blight_Fog_Day_Color");

    void Weather::onWeatherButtonClicked(MyGUI::Widget* _sender)
    {
        auto it = mWeatherButtons.find(_sender);
        if (it != mWeatherButtons.end())
        {
            Log(Debug::Warning) << "Forcing Weather: " << it->second;            
            const std::string region = Misc::StringUtils::lowerCase(MWMechanics::getPlayer().getCell()->getCell()->mRegion);
            MWBase::Environment::get().getWorld()->forceWeather(it->second);
        }

        onTimeChanged();
    }

}