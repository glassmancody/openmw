#ifndef GAMESETTINGS_HPP
#define GAMESETTINGS_HPP

#include <QTextStream>
#include <QStringList>
#include <QString>
#include <QFile>
#include <QMultiMap>
#include <QList>
#include <QDebug>

#include <boost/filesystem/path.hpp>

#include <components/debug/debuglog.hpp>

#include <osg/Vec4f>

namespace Files
{
  typedef std::vector<boost::filesystem::path> PathContainer;
  struct ConfigurationManager;
}

namespace Config
{
    class GameSettings
    {
    public:
        GameSettings(Files::ConfigurationManager &cfg);
        ~GameSettings();

        inline QString value(const QString &key, const QString &defaultValue = QString())
        {
            return mSettings.value(key).isEmpty() ? defaultValue : mSettings.value(key);
        }


        inline void setValue(const QString &key, const QString &value)
        {
            mSettings.remove(key);
            mSettings.insert(key, value);
            mUserSettings.remove(key);
            mUserSettings.insert(key, value);
        }

        inline void setMultiValue(const QString &key, const QString &value)
        {
            QStringList values = mSettings.values(key);
            if (!values.contains(value))
                mSettings.insert(key, value);

            values = mUserSettings.values(key);
            if (!values.contains(value))
                mUserSettings.insert(key, value);
        }

        inline void remove(const QString &key)
        {
            mSettings.remove(key);
            mUserSettings.remove(key);
        }

        inline QStringList getDataDirs() { return mDataDirs; }

        inline void removeDataDir(const QString &dir) { if(!dir.isEmpty()) mDataDirs.removeAll(dir); }
        inline void addDataDir(const QString &dir) { if(!dir.isEmpty()) mDataDirs.append(dir); }
        inline QString getDataLocal() {return mDataLocal; }

        bool hasMaster();

        QStringList values(const QString &key, const QStringList &defaultValues = QStringList()) const;

        bool readFile(QTextStream &stream);
        bool readFile(QTextStream &stream, QMultiMap<QString, QString> &settings);
        bool readUserFile(QTextStream &stream);

        bool writeFile(QTextStream &stream);
        bool writeFileWithComments(QFile &file);

        void setContentList(const QStringList& fileNames);
        QStringList getContentList() const;

        void clear();

        QMultiMap<QString, QString> getUserSettings() { return mUserSettings; }

        osg::Vec4f getWeatherColor(const std::string& key)
        {
            return getWeatherColor(QString::fromStdString(key));
        }

        osg::Vec4f getWeatherColor(const QString& key)
        {
            osg::Vec4f col;
            
            QMultiMap<QString, QString>::iterator i = mUserSettings.find("fallback");
            while (i != mUserSettings.end() && i.key() == "fallback") {
                QStringList items = i.value().split(",", Qt::SkipEmptyParts);

                if (items.size() != 4 || items[0] != key) {
                    i++;
                    continue;
                }
                for (size_t idx = 0; idx < 3; idx++)
                    col[idx] = items[idx+1].toFloat() / 255.0;
            
                i++;
            }
            col[3] = 1.0;
            return col;
        }

        float getTime(const QString& key) // Weather_Sunrise_Time, Weather_Sunset_Time
        {
            int time = -1;
            QMultiMap<QString, QString>::iterator i = mUserSettings.find("fallback");
            while (i != mUserSettings.end() && i.key() == "fallback") {
                QStringList items = i.value().split(",", Qt::SkipEmptyParts);

                if (items.size() != 2 || items[0] != key) {
                    i++;
                    continue;
                }
                return items[1].toFloat();
                i++;
            }
            return time;
        }
        void setWeatherColor(const std::string& key, const osg::Vec4f& col)
        {
            setWeatherColor(QString::fromStdString(key), col);
        }
        void setWeatherColor(const QString& key, const osg::Vec4f& col)
        {
            QMultiMap<QString, QString>::iterator i = mUserSettings.find("fallback");
            while (i != mUserSettings.end() && i.key() == "fallback") {
                if (i.value().startsWith(key)) {
                    QString res = key + ","
                        + QString::number(static_cast<int>(col[0] * 255)) + "," 
                        + QString::number(static_cast<int>(col[1] * 255)) + ","
                        + QString::number(static_cast<int>(col[2] * 255));
                    *i = res;
                    Log(Debug::Warning) << "set key to: " << res.toStdString();
                    break;
                }
                i++;
            }
        }

    private:
        Files::ConfigurationManager &mCfgMgr;

        void validatePaths();
        QMultiMap<QString, QString> mSettings;
        QMultiMap<QString, QString> mUserSettings;

        QStringList mDataDirs;
        QString mDataLocal;

        static const char sContentKey[];
        static const char sFallbackKey[];

        bool isOrderedLine(const QString& line) const;
    };
}
#endif // GAMESETTINGS_HPP
