#pragma execution_character_set("utf-8")

#include "frmsavelog.h"
#include "ui_frmsavelog.h"
#include "savelog.h"
#include "qdatetime.h"
#include "qtimer.h"
#include "qdebug.h"

#include <qsettings.h>

frmSaveLog::frmSaveLog(QWidget *parent) : QWidget(parent), ui(new Ui::frmSaveLog)
{
    this->setAttribute(Qt::WA_QuitOnClose,false);

    ui->setupUi(this);

    this->initForm();

    rdbList.clear();
    rdbList<<ui->rb_LogtoFile<<ui->rb_LogtoNet<<ui->rb_LogtoForm<<ui->rb_LogCancel;
    foreach (QRadioButton* btn, rdbList)
    {
        connect(btn, SIGNAL(clicked()), this, SLOT(getLogDirection()));
    }

    LoadLogSettings();

    // 软件没有保存重定向的方向，这样每次打开软件需要手动使能重定向,方便开发
    ui->rb_LogCancel->setChecked(true);

    //设置是否开启日志上下文打印比如行号、函数等 此功能还未实现
    //SaveLog::Instance()->setUseContext(true);

    //设置文件存储目录
    SaveLog::Instance()->setPath(qApp->applicationDirPath() + "/log");
}

frmSaveLog::~frmSaveLog()
{
    this->SaveSettings();
    SaveLog::Instance()->stop();
    delete ui;
}

void frmSaveLog::LoadLogSettings()
{
    // 设置默认的文件存储行数与上面的文件存储大小会产生优先级，行数限制优先级比文件大小的优先级高
    // 如果文件行数与文件大小都不设置初值，则会在日期改变后才会创建新文件

    QSettings *settings = new QSettings("config.ini", QSettings::IniFormat);
    ui->cboxRow->setCurrentIndex(settings->value("frmSaveLog/LogFileRows","3").toInt());   // 设置文件最多保存2000行
    ui->cboxSize->setCurrentIndex(settings->value("frmSaveLog/LogFileSize","4").toInt());  // 设置默认的文件存储大小 1M
    ui->cboxViewRows->setCurrentIndex(settings->value("frmSaveLog/ViewRows","1").toInt()); // 设置窗口最多显示 300行
    ui->txtPort->setText(settings->value("frmSaveLog/port","6000").toString());            // 设置端口6000

    delete settings; 
}

void frmSaveLog::SaveSettings()
{
    QSettings *settings = new QSettings("config.ini", QSettings::IniFormat);

    settings->setValue("frmSaveLog/LogFileRows", ui->cboxRow->currentIndex());
    settings->setValue("frmSaveLog/LogFileSize", ui->cboxSize->currentIndex());
    settings->setValue("frmSaveLog/ViewRows", ui->cboxViewRows->currentIndex());
    settings->setValue("frmSaveLog/port", ui->txtPort->text().toInt());

    delete settings;
}

void frmSaveLog::initForm()
{
    // 创建拦截信息与窗体追加报文的连接
    connect(SaveLog::Instance(), SIGNAL(sendToForm(QString)), this, SLOT(getstr(QString)));

    this->setWindowTitle(tr("日志显示"));

    //添加消息类型
    QStringList types, datas;
    types << "Debug" << "Info" << "Warning" << "Critical" << "Fatal";
    datas << "1" << "2" << "4" << "8" << "16";
    //ui->cboxType->addItems(types);

    //添加消息类型到列表用于勾选设置哪些类型需要重定向
    int count = types.count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(types.at(i));
        item->setData(Qt::UserRole, datas.at(i));
        item->setCheckState(Qt::Checked);
        ui->listType->addItem(item);
    }

    //添加日志文件大小下拉框
    ui->cboxSize->addItem("不启用", 0);
    ui->cboxSize->addItem("5kb", 5);
    ui->cboxSize->addItem("10kb", 10);
    ui->cboxSize->addItem("30kb", 30);
    ui->cboxSize->addItem("1mb", 1024);

    ui->cboxRow->addItem("不启用", 0);
    ui->cboxRow->addItem("100条", 100);
    ui->cboxRow->addItem("500条", 500);
    ui->cboxRow->addItem("2000条", 2000);
    ui->cboxRow->addItem("10000条", 10000);

    ui->cboxViewRows->addItem("100条", 100);
    ui->cboxViewRows->addItem("300条", 300);
    ui->cboxViewRows->addItem("700条", 700);
    ui->cboxViewRows->addItem("1000条", 1000);
    ui->cboxViewRows->addItem("2000条", 2000);
}

// 此append函数仅用于调试所用

void frmSaveLog::append(const QString &flag)
{
    Q_UNUSED(flag);
#if 0
    if (count >= 100) {
        count = 0;
        ui->txtMain->clear();
    }

    QString str1;
    int type = ui->cboxType->currentIndex();
    if (!ui->ckSave->isChecked()) {
        if (type == 0) {
            str1 = "Debug ";
        } else if (type == 1) {
            str1 = "Infox ";
        } else if (type == 2) {
            str1 = "Warnx ";
        } else if (type == 3) {
            str1 = "Error ";
        } else if (type == 4) {
            str1 = "Fatal ";
        }
    }

    QString str2 = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString str3 = flag.isEmpty() ? "自动插入消息" : flag;
    QString msg = QString("%1当前时间: %2 %3").arg(str1).arg(str2).arg(str3);

    //开启网络重定向换成英文方便接收解析不乱码
    //对方接收解析的工具未必是utf8
    if (ui->ckNet->isChecked()) {
        msg = QString("%1time: %2 %3").arg(str1).arg(str2).arg("(QQ: 517216493 WX: feiyangqingyun)");
    }

    count++;
    ui->txtMain->append(msg);

    //根据不同的类型打印
    //TMD转换要分两部走不然msvc的debug版本会乱码(英文也一样)
    //char *data = msg.toUtf8().data();
    QByteArray buffer = msg.toUtf8();
    const char *data = buffer.constData();
    if (type == 0) {
        qDebug(data);
    } else if (type == 1) {
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
        qInfo(data);
#endif
    } else if (type == 2) {
        qWarning(data);
    } else if (type == 3) {
        qCritical(data);
    } else if (type == 4) {
        //调用下面这个打印完会直接退出程序
        qFatal(data);
    }
#endif
}

void frmSaveLog::getLogDirection()
{
    QString str = "11";
    foreach (QRadioButton *var, rdbList)
    {
        if(var->isChecked())
        {
            str = var->objectName();
        }
    }

    if(str == "rb_LogtoFile")
    {
        SaveLog::Instance()->setDirection(RedirectionType::To_File);
    }
    else if(str == "rb_LogtoNet")
    {
        SaveLog::Instance()->setListenPort(ui->txtPort->text().toInt());
        SaveLog::Instance()->setDirection(RedirectionType::To_Net);
    }
    else if(str == "rb_LogtoForm")
    {
        SaveLog::Instance()->setDirection(RedirectionType::To_Form);
    }
    else if(str == "rb_LogCancel")
    {
        SaveLog::Instance()->setDirection(RedirectionType::Cancel);
    }
}

void frmSaveLog::on_btnLog_clicked()
{
    ui->txtMain->clear();
}


void frmSaveLog::on_cboxSize_currentIndexChanged(int index)
{
    int size = ui->cboxSize->itemData(index).toInt();
    SaveLog::Instance()->setMaxSize(size);
}

void frmSaveLog::on_cboxRow_currentIndexChanged(int index)
{
    int row = ui->cboxRow->itemData(index).toInt();
    SaveLog::Instance()->setMaxRow(row);
}

void frmSaveLog::on_listType_itemPressed(QListWidgetItem *item)
{
    //切换选中行状态
    item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);

    //找到所有勾选的类型进行设置
    quint8 types = 0;
    int count = ui->listType->count();
    for (int i = 0; i < count; ++i) {
        QListWidgetItem *item = ui->listType->item(i);
        if (item->checkState() == Qt::Checked) {
            types += item->data(Qt::UserRole).toInt();
        }
    }

    SaveLog::Instance()->setMsgType((MsgType)types);
}

void frmSaveLog::getstr(const QString &content)
{
    ui->txtMain->append(content);
}

void frmSaveLog::on_cboxViewRows_currentIndexChanged(int index)
{
    bool ok;
    int size = ui->cboxViewRows->itemData(index).toInt(&ok);

    if(ok)
    {
        ui->txtMain->document()->setMaximumBlockCount(size);
    }
}



