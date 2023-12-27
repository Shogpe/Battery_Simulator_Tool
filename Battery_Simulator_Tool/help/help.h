#ifndef HELP_H
#define HELP_H

#include <QWidget>
#include <QObject>

namespace Ui {
class help;
}

class help : public QWidget
{
    Q_OBJECT

public:
    explicit help(QWidget *parent = nullptr);
    ~help();

private:
    Ui::help *ui;
};

extern help helps;
#endif // HELP_H
