// PreferencesDialog.cpp
#include "PreferencesDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QCheckBox>
#include <QSlider>

PreferencesDialog::PreferencesDialog(QWidget *parent, Prefs currentPrefs)
    : QDialog(parent), prefs(currentPrefs) {

    setWindowTitle("Preferences");
    setModal(true);  // Makes it block the main window

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Enable cool feature:"));

    QCheckBox *featureCheckbox = new QCheckBox("Enable feature", this);
    layout->addWidget(featureCheckbox);

    QLabel *fovLabel = new QLabel("Field of View", this);
    layout->addWidget(fovLabel);

    fovSlider = new QSlider(this);
    fovSlider->setOrientation(Qt::Horizontal);
    fovSlider->setMinimum(45);
    fovSlider->setMaximum(90);
    fovSlider->setSliderPosition(prefs.m_fov);
    connect(fovSlider, &QSlider::sliderMoved, this, &PreferencesDialog::setFov);
    layout->addWidget(fovSlider);

    QLabel *rootDirLabel = new QLabel("Game assets root directory", this);
    layout->addWidget(rootDirLabel);

    rootDirBox = new QTextEdit(this);
    rootDirBox->setText(prefs.m_rootDir);
    connect(rootDirBox, &QTextEdit::textChanged, this, [this]() {
        setRootDir(rootDirBox->toPlainText());
    });
    layout->addWidget(rootDirBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::accept); // Ok button
    //connect(buttonBox, &QDialogButtonBox::clicked, this, &PreferencesDialog::accept); // Apply button
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject); // Cancel button
    layout->addWidget(buttonBox);
}

void PreferencesDialog::setFov(int value) {
    prefs.m_fov = value;
}

void PreferencesDialog::setRootDir(QString value) {
    prefs.m_rootDir = value;
}
