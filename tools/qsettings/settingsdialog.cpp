#include "settingsdialog.h"
#include "whitelistdialog.h"
#include "ui_settingsdialog.h"
#include "config.h"

#include <QDir>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QWidget>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->styleComboBox->addItems( QStyleFactory::keys() );

    crypt = new SimpleCrypt(Q_UINT64_C(0x3de48151623423de));

    QDir iconsDir("/boot/system/data/icons");
    QStringList iconSets = iconsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int n=0; n < iconSets.count(); n++) {
        QString themeFile = iconsDir.absolutePath() +"/" + iconSets[n] + "/index.theme";
        bool isTheme = QFile::exists(themeFile);
        if (isTheme) {
            ui->iconSetComboBox->addItem( iconSets[n], iconSets[n] );
        }
    }

    ui->messagesCheckBox->setHidden(true);
    ui->colorPickerCheckBox->setHidden(true);
    ui->OpenGlGroup->setHidden(true);

    readSettings();
}

SettingsDialog::~SettingsDialog()
{
    writeSettings();
    delete ui;
    delete crypt;
}

void SettingsDialog::writeSettings()
 {
    QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
    settings.beginGroup("QPA");
    settings.setValue("messages_native", ui->messagesCheckBox->isChecked());
    settings.setValue("filepanel_native", ui->filePanelCheckBox->isChecked());
    settings.setValue("colorpicker_native", ui->colorPickerCheckBox->isChecked());
    settings.setValue("opengl_enabled", ui->openGlCheckBox->isChecked());
    settings.setValue("qml_softwarecontext", ui->softContextCheckBox->isChecked());
    settings.setValue("hide_from_deskbar", ui->deskBarCheckBox->isChecked());
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
    settings.beginGroup("Network");
    settings.setValue("use_proxy", ui->proxyGroupBox->isChecked());
    settings.setValue("http_proxy_enable", ui->httpProxyEnable->isChecked());
    settings.setValue("http_proxy_scheme", ui->httpProxyScheme->currentText());
    settings.setValue("http_proxy_address", ui->httpProxyAddress->text());
    settings.setValue("http_proxy_port", ui->httpProxyPort->value());
    settings.setValue("http_proxy_username", ui->httpProxyUser->text());
    settings.setValue("http_proxy_password", crypt->encryptToString(ui->httpProxyPassword->text()));
    settings.setValue("https_proxy_enable", ui->httpsProxyEnable->isChecked());
    settings.setValue("https_proxy_scheme", ui->httpsProxyScheme->currentText());
    settings.setValue("https_proxy_address", ui->httpsProxyAddress->text());
    settings.setValue("https_proxy_port", ui->httpsProxyPort->value());
    settings.setValue("https_proxy_username", ui->httpsProxyUser->text());
    settings.setValue("https_proxy_password", crypt->encryptToString(ui->httpsProxyPassword->text()));
    settings.setValue("ftp_proxy_enable", ui->ftpProxyEnable->isChecked());
    settings.setValue("ftp_proxy_scheme", ui->ftpProxyScheme->currentText());
    settings.setValue("ftp_proxy_address", ui->ftpProxyAddress->text());
    settings.setValue("ftp_proxy_port", ui->ftpProxyPort->value());
    settings.setValue("ftp_proxy_username", ui->ftpProxyUser->text());
    settings.setValue("ftp_proxy_password", crypt->encryptToString(ui->ftpProxyPassword->text()));
    settings.setValue("no_proxy_list", ui->noProxyList->toPlainText());
    settings.endGroup();
}

void SettingsDialog::readSettings()
{
    QSettings settings(QT_SETTINGS_FILENAME, QSettings::NativeFormat);
    settings.beginGroup("QPA");
    ui->messagesCheckBox->setChecked(settings.value("messages_native", true).toBool());
    ui->filePanelCheckBox->setChecked(settings.value("filepanel_native", true).toBool());
    ui->colorPickerCheckBox->setChecked(settings.value("colorpicker_native", true).toBool());
    ui->openGlCheckBox->setChecked(settings.value("opengl_enabled", false).toBool());
    ui->softContextCheckBox->setChecked(settings.value("qml_softwarecontext", true).toBool());
    ui->deskBarCheckBox->setChecked(settings.value("hide_from_deskbar", true).toBool());
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
    settings.beginGroup("Network");
    QString emptyString = crypt->encryptToString(QString(""));
    ui->proxyGroupBox->setChecked(settings.value("use_proxy", false).toBool());
    ui->httpProxyEnable->setChecked(settings.value("http_proxy_enable", false).toBool());
    ui->httpProxyScheme->setCurrentText(settings.value("http_proxy_scheme", QString("http://")).toString());
    ui->httpProxyAddress->setText(settings.value("http_proxy_address", QString("")).toString());
    ui->httpProxyPort->setValue(settings.value("http_proxy_port", 8080).toInt());
    ui->httpProxyUser->setText(settings.value("http_proxy_username", QString("")).toString());
    ui->httpProxyPassword->setText(crypt->decryptToString(settings.value("http_proxy_password", emptyString).toString()));
    ui->httpsProxyEnable->setChecked(settings.value("https_proxy_enable", false).toBool());
    ui->httpsProxyScheme->setCurrentText(settings.value("https_proxy_scheme", QString("http://")).toString());
    ui->httpsProxyAddress->setText(settings.value("https_proxy_address", QString("")).toString());
    ui->httpsProxyPort->setValue(settings.value("https_proxy_port", 8080).toInt());
    ui->httpsProxyUser->setText(settings.value("https_proxy_username", QString("")).toString());
    ui->httpsProxyPassword->setText(crypt->decryptToString(settings.value("https_proxy_password", emptyString).toString()));
    ui->ftpProxyEnable->setChecked(settings.value("ftp_proxy_enable", false).toBool());
    ui->ftpProxyScheme->setCurrentText(settings.value("ftp_proxy_scheme", QString("http://")).toString());
    ui->ftpProxyAddress->setText(settings.value("ftp_proxy_address", QString("")).toString());
    ui->ftpProxyPort->setValue(settings.value("ftp_proxy_port", 8080).toInt());
    ui->ftpProxyUser->setText(settings.value("ftp_proxy_username", QString("")).toString());
    ui->ftpProxyPassword->setText(crypt->decryptToString(settings.value("ftp_proxy_password", emptyString).toString()));
    ui->noProxyList->setPlainText(settings.value("no_proxy_list", QString("localhost,127.0.0.1")).toString());
    settings.endGroup();
 }

void SettingsDialog::on_defaultsButton_clicked()
{
    ui->messagesCheckBox->setChecked(true);
    ui->filePanelCheckBox->setChecked(true);
    ui->colorPickerCheckBox->setChecked(true);
    ui->openGlCheckBox->setChecked(false);
    ui->softContextCheckBox->setChecked(true);
    ui->styleComboBox->setCurrentText("haiku");
    ui->iconSetComboBox->setCurrentText("haiku");
    ui->smallIconSizeSpinBox->setValue(16);
    ui->largeIconSizeSpinBox->setValue(32);
    ui->toolbarIconSizeSpinBox->setValue(24);
    ui->toolbarModeComboBox->setCurrentIndex(0);
    ui->menuIconCheckBox->setChecked(true);
    ui->deskBarCheckBox->setChecked(true);
}

void SettingsDialog::on_revertButton_clicked()
{
    readSettings();
}

void SettingsDialog::on_whiteListButton_clicked()
{
    WhiteListDialog *dlg = new WhiteListDialog(this);
    dlg->show();
}

void SettingsDialog::on_styleComboBox_currentIndexChanged(int index)
{
    QString styleName = ui->styleComboBox->itemText(index);
    QApplication::setStyle(QStyleFactory::create(styleName));
}
