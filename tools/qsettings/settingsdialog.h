#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>

#include "simplecrypt.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

private slots:
    void on_defaultsButton_clicked();

    void on_revertButton_clicked();

    void on_whiteListButton_clicked();

	void on_styleComboBox_currentIndexChanged(int index);

private:
    void writeSettings();
    void readSettings();

    Ui::SettingsDialog *ui;
    QSettings *settings;
    SimpleCrypt *crypt;
};

#endif // SETTINGSDIALOG_H
