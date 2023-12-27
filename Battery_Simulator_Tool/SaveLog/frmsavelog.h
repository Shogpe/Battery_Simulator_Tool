#ifndef FRMSAVELOG_H
#define FRMSAVELOG_H

#include <QWidget>
#include <QListWidgetItem>


class QRadioButton;
namespace Ui {
class frmSaveLog;
}

class frmSaveLog : public QWidget
{
    Q_OBJECT

public:
    explicit frmSaveLog(QWidget *parent = 0);
    ~frmSaveLog();



private:
    Ui::frmSaveLog *ui;
    QList<QRadioButton*> rdbList;

    void SaveSettings();
    void LoadLogSettings();

private slots:
    void initForm();
    void append(const QString &flag = QString());
    void getLogDirection();
    void getstr(const QString &content);

private slots:
    void on_btnLog_clicked(); 
    void on_cboxSize_currentIndexChanged(int index);
    void on_cboxRow_currentIndexChanged(int index);
    void on_listType_itemPressed(QListWidgetItem *item);
    void on_cboxViewRows_currentIndexChanged(int index);
};

#endif // FRMSAVELOG_H
