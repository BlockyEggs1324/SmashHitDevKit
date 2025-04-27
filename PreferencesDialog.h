#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QSlider>
#include <QTextEdit>

struct Preferences {
    float m_fov;
    QString m_rootDir;
    float m_sensitivity;

    Preferences() : m_fov(60), m_rootDir("C:/"), m_sensitivity(0.1f) {}
    Preferences(float fov, QString rootDir, float sense) : m_fov(fov), m_rootDir(rootDir), m_sensitivity(sense) {}
};

using Prefs = Preferences;

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr, Prefs currentPrefs = Prefs());

    Prefs prefs;

private:
    void setFov(int value);
    void setRootDir(QString value);

    QSlider *fovSlider;
    QTextEdit *rootDirBox;

};
#endif // PREFERENCESDIALOG_H
