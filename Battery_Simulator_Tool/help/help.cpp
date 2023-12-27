#include "help.h"
#include "ui_help.h"
#pragma execution_character_set("utf-8")

help::help(QWidget *parent) : QWidget(parent), ui(new Ui::help)
{
    ui->setupUi(this);
}

help::~help()
{
    delete ui;
}

