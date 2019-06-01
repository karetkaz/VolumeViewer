#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QFile>
#include <QUrl>

class Settings: public QSettings {
	Q_OBJECT
public:
	explicit Settings(const QString &name): QSettings(QSettings::IniFormat, QSettings::UserScope, name) {
		this->sync();
	}

	Q_INVOKABLE QVariant path() {
		return fileName();
	}

	Q_INVOKABLE QVariant getValue(const QString &section, const QString &key, const QVariant &defaultValue = QVariant()) {
		this->beginGroup(section);
		QVariant value = QSettings::value(key, defaultValue);
		QSettings::endGroup();
		return value;
	}
	Q_INVOKABLE void setValue(const QString &section, const QString &key, const QVariant &value) {
		QSettings::beginGroup(section);
		QSettings::setValue(key, value);
		QSettings::endGroup();
		//return value;
	}

	Q_INVOKABLE QVariant getArray(const QString &section, const QString &key) {
		QList<QVariant> list;
		QSettings::beginGroup(section);
		int size = QSettings::beginReadArray(key);
		for (int i = 0; i < size; ++i) {
			QSettings::setArrayIndex(i);
			list.append(QSettings::value("value"));
		}
		QSettings::endArray();
		QSettings::endGroup();
		return QVariant(list);
	}
	Q_INVOKABLE void setArray(const QString &section, const QString &key, const QVariant &value) {
		QList<QVariant> list = value.toList();
		QSettings::beginGroup(section);
		QSettings::beginWriteArray(key);
		for (int i = 0; i < list.size(); ++i) {
			QSettings::setArrayIndex(i);
			QSettings::setValue("value", list.at(i));
		}
		QSettings::endArray();
		QSettings::endGroup();
		//return QVariant(list);
	}
};

#endif
