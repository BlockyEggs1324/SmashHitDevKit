// PreferencesDialog.cpp
#include "PreferencesDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QComboBox>

PreferencesDialog::PreferencesDialog(QWidget *parent, Prefs currentPrefs)
    : QDialog(parent), prefs(currentPrefs) {

    setWindowTitle("Preferences");
    setModal(true);  // Makes it block the main window

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Enable cool feature:"));

    QCheckBox *featureCheckbox = new QCheckBox("Enable feature", this);
    layout->addWidget(featureCheckbox);

    QComboBox *themeDropdown = new QComboBox(this);
    themeDropdown->addItem("System Theme");
    themeDropdown->addItem("Light");
    themeDropdown->addItem("Dark");
    themeDropdown->addItem("Dark Purple");
    themeDropdown->addItem("Swamp");

    connect(themeDropdown, &QComboBox::currentTextChanged, this, &PreferencesDialog::setTheme);
    layout->addWidget(themeDropdown);

    int fov = prefs.m_fov;

    QString fovText = "Field of View: " + QString::fromStdString(std::to_string(fov));

    fovLabel = new QLabel(fovText, this);
    layout->addWidget(fovLabel);

    fovSlider = new QSlider(this);
    fovSlider->setOrientation(Qt::Horizontal);
    fovSlider->setMinimum(45);
    fovSlider->setMaximum(90);
    fovSlider->setSliderPosition(prefs.m_fov);
    connect(fovSlider, &QSlider::sliderMoved, this, &PreferencesDialog::setFov);
    connect(fovSlider, &QSlider::valueChanged, this, &PreferencesDialog::setFov);
    layout->addWidget(fovSlider);

    QString labelText = "Sensitivity: " + QString::fromStdString(std::to_string(prefs.m_sensitivity).substr(0, 4));

    sensLabel = new QLabel(labelText, this);
    layout->addWidget(sensLabel);

    sensSlider = new QSlider(this);
    sensSlider->setOrientation(Qt::Horizontal);
    sensSlider->setMinimum(5);
    sensSlider->setMaximum(50);
    sensSlider->setSliderPosition(prefs.m_sensitivity * 10.0);
    connect(sensSlider, &QSlider::sliderMoved, this, &PreferencesDialog::setSens);
    connect(sensSlider, &QSlider::valueChanged, this, &PreferencesDialog::setSens);
    layout->addWidget(sensSlider);

    QLabel *rootDirLabel = new QLabel("Game assets root directory", this);
    layout->addWidget(rootDirLabel);

    // In your widget/dialog constructor or setup function:
    QPushButton* openFolderButton = new QPushButton("Open Root Folder", this);

    QRect buttonRect(rootDirLabel->geometry().topRight(), QPoint(10, 10));

    connect(openFolderButton, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Folder", prefs.m_rootDir);
        if (!dir.isEmpty()) {
            rootDirBox->setText(dir);
        }
    });

    layout->addWidget(openFolderButton);

    rootDirBox = new QTextEdit(this);
    rootDirBox->setFixedHeight(60);
    rootDirBox->setText(prefs.m_rootDir);
    connect(rootDirBox, &QTextEdit::textChanged, this, [this]() {
        setRootDir(rootDirBox->toPlainText());
    });

    layout->addWidget(rootDirBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel);

    // Connect the Apply button
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &PreferencesDialog::accept);

    // Connect the Cancel button
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &PreferencesDialog::reject);
    layout->addWidget(buttonBox);

}

void PreferencesDialog::setTheme(QString value) {
    prefs.m_theme = value;
}

void PreferencesDialog::setFov(int value) {
    fovLabel->setText("Field of View: " + QString::fromStdString(std::to_string(value)));
    prefs.m_fov = value;
}

void PreferencesDialog::setSens(int value) {
    sensLabel->setText("Sensitivity: " + QString::fromStdString(std::to_string(value / 10.0).substr(0, 4)));
    prefs.m_sensitivity = value / 10.0;
}

void PreferencesDialog::setRootDir(QString value) {
    prefs.m_rootDir = value;
}
