#ifndef WHITELISTDIALOG_H
#define WHITELISTDIALOG_H

#include <QDialog>
#include <QStringListModel>

namespace Ui {
class WhiteListDialog;
}

class WhiteListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WhiteListDialog(QWidget *parent = 0);
    ~WhiteListDialog();

private slots:
    void on_addButton_clicked();

    void on_removeButton_clicked();

    void on_okButton_clicked();

    void on_cancelButton_clicked();

private:
    Ui::WhiteListDialog *ui;
    QStringListModel listModel;
};

#endif // WHITELISTDIALOG_H
