#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QTextEdit>

struct Preferences {
    float m_fov;
    QString m_rootDir;
    QString m_theme;
    float m_sensitivity;

    Preferences() : m_fov(60), m_rootDir("C:/"), m_theme("Light"), m_sensitivity(1.0f) {}
    Preferences(float fov, QString rootDir, float sense, QString theme) : m_fov(fov), m_rootDir(rootDir), m_theme(theme), m_sensitivity(sense) {}
};

using Prefs = Preferences;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr, Prefs currentPrefs = Prefs());

    Prefs prefs;

private:
    void setTheme(QString value);
    void setFov(int value);
    void setSens(int value);
    void setRootDir(QString value);

    QLabel *fovLabel;
    QSlider *fovSlider;

    QLabel *sensLabel;
    QSlider *sensSlider;

    QTextEdit *rootDirBox;

};
#endif // PREFERENCESDIALOG_H
