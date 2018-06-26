#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QDir>
#include <QString>
#include <QStringList>
#include <QStyleFactory>

#define QT_SETTINGS_FILENAME "/boot/home/config/settings/qt-plugins"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->styleComboBox->addItems( QStyleFactory::keys() );

    QDir iconsDir("/boot/system/data/icons");
    QStringList iconSets = iconsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int n=0; n < iconSets.count(); n++) {
        QString themeFile = iconsDir.absolutePath() +"/" + iconSets[n] + "/index.theme";
        bool isTheme = QFile::exists(themeFile);
        if (isTheme) {
            ui->iconSetComboBox->addItem( iconSets[n], iconSets[n] );
        }
    }

    readSettings();
}

SettingsDialog::~SettingsDialog()
{
    writeSettings();
    delete ui;
}

void SettingsDialog::writeSettings()
 {
    QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
    settings.beginGroup("QPA");
    settings.setValue("native_messages", ui->messagesCheckBox->isChecked());
    settings.setValue("native_filepanel", ui->filePanelCheckBox->isChecked());
    settings.setValue("native_colorpicker", ui->colorPickerCheckBox->isChecked());
    settings.setValue("opengl_enabled", ui->openGlCheckBox->isChecked());
    settings.setValue("qml_softwarecontext", ui->softContextCheckBox->isChecked());
    settings.endGroup();
    settings.beginGroup("Style");
    settings.setValue("widget_style", ui->styleComboBox->currentText());
    settings.setValue("icons_iconset", ui->iconSetComboBox->currentText());
    settings.setValue("icons_small_size", ui->smallIconSizeSpinBox->value());
    settings.setValue("icons_large_size", ui->largeIconSizeSpinBox->value());
    settings.setValue("icons_toolbar_icon_size", ui->toolbarIconSizeSpinBox->value());
    settings.setValue("icons_toolbar_mode", ui->toolbarModeComboBox->currentIndex());
    settings.setValue("icons_menu_icons", ui->menuIconCheckBox->isChecked());
    settings.endGroup();
}

void SettingsDialog::readSettings()
{
    QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
    settings.beginGroup("QPA");
    ui->messagesCheckBox->setChecked(settings.value("native_messages", true).toBool());
    ui->filePanelCheckBox->setChecked(settings.value("native_filepanel", false).toBool());
    ui->colorPickerCheckBox->setChecked(settings.value("native_colorpicker", false).toBool());
    ui->openGlCheckBox->setChecked(settings.value("opengl_enabled", false).toBool());
    ui->softContextCheckBox->setChecked(settings.value("qml_softwarecontext", true).toBool());
    settings.endGroup();
    settings.beginGroup("Style");
    ui->styleComboBox->setCurrentText(settings.value("widget_style", "haiku").toString());
    ui->iconSetComboBox->setCurrentText(settings.value("icons_iconset", "haiku").toString());
    ui->smallIconSizeSpinBox->setValue(settings.value("icons_small_size", 16).toInt());
    ui->largeIconSizeSpinBox->setValue(settings.value("icons_large_size", 32).toInt());
    ui->toolbarIconSizeSpinBox->setValue(settings.value("icons_toolbar_icon_size", 24).toInt());
    ui->toolbarModeComboBox->setCurrentIndex(settings.value("icons_toolbar_mode", 0).toInt());
    ui->menuIconCheckBox->setChecked(settings.value("icons_menu_icons", true).toBool());
    settings.endGroup();
 }

void SettingsDialog::on_pushButtonDefaults_clicked()
{
    ui->messagesCheckBox->setChecked(true);
    ui->filePanelCheckBox->setChecked(false);
    ui->colorPickerCheckBox->setChecked(false);
    ui->openGlCheckBox->setChecked(false);
    ui->softContextCheckBox->setChecked(true);
    ui->styleComboBox->setCurrentText("haiku");
    ui->iconSetComboBox->setCurrentText("haiku");
    ui->smallIconSizeSpinBox->setValue(16);
    ui->largeIconSizeSpinBox->setValue(32);
    ui->toolbarIconSizeSpinBox->setValue(24);
    ui->toolbarModeComboBox->setCurrentIndex(0);
    ui->menuIconCheckBox->setChecked(true);
}

void SettingsDialog::on_pushButtonRevert_clicked()
{
    readSettings();
}
