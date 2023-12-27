#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "can_protocol.h"


#include <QAxObject>
#include <QFileDialog>
#include <QDesktopServices>

#include <qsettings.h>

#include <QStandardItemModel>
# pragma execution_character_set("utf-8")


MainWindow::MainWindow(QMainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_can = new ECanClass();
    m_thread = new CanReadThread;
    m_thread->setCan(m_can);

    simData = {{0}};
    canTxCnt = 0;
    canRxCnt = 0;
    canLogError = 0;
    canLogWarning = 0;

    runProgressTotal = 0;
    runProgress = 0;

    //配置电流最大值
    for(int i = 0; i< CELL_NUM_MAX;i++)
    {
        simData.dbCellCurrSet[i] = 1100;
    }

    qRegisterMetaType<can_message_t>("can_message_t");
    connect(m_thread, &CanReadThread::receive_data, this, [this](can_message_t msg) {recvieCANData(msg);});

    canOpenIsOk = false;

    // 标题菜单栏
    connect(ui->menubar, &QMenuBar::triggered, this, [=](QAction* action) {
        qDebug() << action->objectName();
        this->deal_msg(action->objectName());
    });

    //按钮
    connect(ui->ConnectCan, &QPushButton::clicked, this, &MainWindow::btnClick);
    connect(ui->DisconnectCan, &QPushButton::clicked, this, &MainWindow::btnClick);
    connect(ui->searchDevice, &QPushButton::clicked, this, &MainWindow::btnClick);
    connect(ui->boaudSetButton, &QPushButton::clicked, this, &MainWindow::btnClick);
    connect(ui->valSetBotton, &QPushButton::clicked, this, &MainWindow::btnClick);
    connect(ui->resetCntButton, &QPushButton::clicked, this, &MainWindow::btnClick);

    connect(ui->AddButton, &QPushButton::clicked, this, &MainWindow::CmdList_Add);
    connect(ui->DelButton, &QPushButton::clicked, this, &MainWindow::CmdList_Delete);
    connect(ui->RunButton, &QPushButton::clicked, this, &MainWindow::CmdList_Run);
    connect(ui->StopButton, &QPushButton::clicked, this, &MainWindow::CmdList_Stop);

    ui->StopButton->setEnabled(false);
    cmdRuningStat = false;

    //表格
    ui->tableWidget_CellInfo->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_Cmd->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for(int i=0;i<ui->tableWidget_Cmd->rowCount();i++)
    {
        QTableWidgetItem* p_check = new QTableWidgetItem();
        p_check->setCheckState(Qt::Unchecked);        //设置首列为 CheckBox 控件
        ui->tableWidget_Cmd->setItem(i,0,p_check);
    }

    connect(ui->tableWidget_CellInfo, &QTableWidget::itemSelectionChanged, this, &MainWindow::CellList_Sellect);
    //timerS
    timerForUI.setInterval(1000);
    connect(&timerForUI, &QTimer::timeout, this, &MainWindow::CellList_UIFreasher);
    timerForUI.start();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::deal_msg(const QString& msg)
{
    if(msg == "debugLog")
    {
        savelog.show();
    }

    if(msg == "importCfg")
    {
        CmdList_Import();
    }

    if(msg == "exportCfg")
    {
        CmdList_Export();
    }
    if(msg == "actionhelp")
    {

        helps.show();
    }
}



void MainWindow::btnClick()
{
    can_message_t msg;
    auto btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    if (btn->objectName() == "ConnectCan")
    {
        //最后固定上位机波特率
        m_can->canSetBaudRate((CAN_BAUDRATE)(ui->CANBaudrate->currentIndex()));
        m_can->canSetMode(VCI_MODE_NORMAL);
        for(int i =0;i<10;i++)
        {
            canOpenIsOk = m_can->Initialize(USBCAN2, i, ui->canIdSel->currentIndex());
            if(canOpenIsOk)break;
        }
        if (!m_thread->isRunning())
        {
            m_thread->start();
//            timerForUI.start();
        }

        if (canOpenIsOk == STATUS_OK)
        {
            if (!m_thread->isRunning())
            {
                m_thread->start();
//                timerForUI.start();
            }
            ui->canConnectState->setStyleSheet("color:green");
            ui->canConnectState->setText(tr("已连接"));
            ui->canIdSel->setEnabled(false);
            ui->CANBaudrate->setEnabled(false);
            qInfo()<<"connectted can id "<<ui->canIdSel->currentIndex()<<" with boaud"<<ui->CANBaudrate->currentText();

        }
        else
        {
            ui->canConnectState->setStyleSheet("color:red");
            ui->canConnectState->setText(tr("连接失败"));
        }
        QSettings("config.ini", QSettings::IniFormat).setValue("bmu/baudrate", ui->CANBaudrate->currentText());

    }
    else if (btn->objectName() == "DisconnectCan")
    {
        if (m_thread->isRunning())
        {
            m_thread->stopImmediately();
        }
        m_can->Uninitialize();
        canOpenIsOk = false;
        ui->canConnectState->setStyleSheet("color:black");
        ui->canConnectState->setText(tr("未连接"));
        ui->canIdSel->setEnabled(true);
        ui->CANBaudrate->setEnabled(true);
    }
    else if(btn->objectName() == "searchDevice")
    {
        ui->searchDevice->setEnabled(false);
        SearchDevices();
        ui->searchDevice->setEnabled(true);
    }
    else if(btn->objectName() == "boaudSetButton")
    {
        ClearMsgData(msg.data);
        msg = MakeUpCanPacketId( CMD_CODE_SYS_SET_BAUD , CMD_PAGE_SYS , UPPER_PC_ADDR , BROADCAST_ADDR , CMD_MODE_W );
        msg.data[0] = (uint8_t)(ui->CANBaudrateSet->currentIndex());
        (void)sendCANData(msg);

        for(int i=0;i<1000000;i++);
    }
    else if(btn->objectName() == "valSetBotton")
    {
        CellList_Modifier();
    }
    else if(btn->objectName() == "resetCntButton")
    {
        canTxCnt = 0;
        canRxCnt = 0;
        canLogError = 0;
        canLogWarning = 0;
    }



}





bool MainWindow::sendCANData(can_message_t &msg)
{
    if (m_can->Write(msg) == STATUS_OK) {
        canTxCnt++;
        return true;
    } else {
        return false;
    }

}

bool MainWindow::recvieCANData(can_message_t &msg)
{
    canMsgId_T unpackMsg;

    if(msg.IDE == 0)
    {
        return false;
    }

    canRxCnt++;
    if(ui->SimulatorType->currentIndex()==SIMULATOR_JCY2200)
    {
        //qDebug() << "react SIMULATOR_JCY2200" ;

        unpackMsg.u32Word = msg.id;
        if(unpackMsg.Bits.cmdPage == CMD_PAGE_GEN)
        {
            switch(unpackMsg.Bits.cmdCode)
            {
            case CMD_CODE_GEN_VOLTAGE:

            break;

            case CMD_CODE_GEN_CURR_RANGE:

            break;

            case CMD_CODE_GEN_PARA_METER:

            break;

            case CMD_CODE_GEN_OUT_RELY:

            break;

            case CMD_CODE_GEN_READ_TEMP:

            break;

            case CMD_CODE_GEN_READ_PARA:

                if(devicSearching == true)
                {
                    simData.u8DeviceNum++;
                    ui->devices->setValue(simData.u8DeviceNum);
                    ui->devices->repaint ();
                    qInfo() << "react code : read para ，id ："<<(unpackMsg.Bits.srcAddr-1);
                    qDebug() << simData.u8DeviceNum;
                }

                simData.simIdValid[unpackMsg.Bits.srcAddr-1] = SimId_Valid;
                simData.dbCellVolt[unpackMsg.Bits.srcAddr-1] = TransformVoltDataMV(&msg.data[0]);
                simData.dbCellCurr[unpackMsg.Bits.srcAddr-1] = TransformCurrDataMA(&msg.data[3]);
                simData.u8ReleyState[unpackMsg.Bits.srcAddr-1] = (msg.data[6]&0x02);
                simData.dbCellTemperature[unpackMsg.Bits.srcAddr-1] = msg.data[7];

            break;

            default:break;

            }
        }
        else if(unpackMsg.Bits.cmdPage == CMD_PAGE_LOG)
        {
            switch(unpackMsg.Bits.cmdCode)
            {
            case CMD_CODE_LOG_OK:

            break;

            case CMD_CODE_LOG_WARNING:
                canLogWarning++;
            break;

            case CMD_CODE_LOG_ERROR:
                canLogError++;
            break;
                default:break;
            }
        }



    }
    else
    {

    }


    return true;
}


void Delay_MSec(unsigned int msec)
{
    QEventLoop loop;//定义一个新的事件循环
    QTimer::singleShot(msec, &loop, SLOT(quit()));  //创建单次定时器，槽函数为事件循环的退出函数
    loop.exec();  //事件循环开始执行，程序会卡在这里，直到定时时间到，本循环被退出
}

void MainWindow::SearchDevices()
{
    can_message_t msg;

    devicSearching = true;
    timerForUI.stop();
    simData.u8DeviceNum = 0;

    ui->tableWidget_CellInfo->setRowCount(0);
    ui->devices->setValue(simData.u8DeviceNum);
    ui->devices->repaint();

    ui->searchDevice->setText(QString("查找中..."));
    ui->searchDevice->repaint();
    for(int idx = 0; idx < CELL_NUM_MAX; idx++)
    {
        simData.simIdValid[idx] = SimId_Invalid;

        ClearMsgData(msg.data);
        msg = MakeUpCanPacketId( CMD_CODE_GEN_READ_PARA , CMD_PAGE_GEN , UPPER_PC_ADDR , (uint8_t)idx , CMD_MODE_R );
        (void)sendCANData(msg);
        Delay_MSec(50);
    }

    ui->searchDevice->setText(QString("查找设备"));


    ClearMsgData(msg.data);
    msg.data[0] = 1;
    msg.data[1] = 60;
    msg = MakeUpCanPacketId( CMD_CODE_GEN_SEL_ADDR , CMD_PAGE_GEN , UPPER_PC_ADDR , (uint8_t)BROADCAST_ADDR , CMD_MODE_W );
    (void)sendCANData(msg);



    timerForUI.start();
}



void MainWindow::CellList_UIFreasher()
{
    int rows = 0;
    can_message_t msg;
    int TempDebugRowNum;

    devicSearching = false;


    ui->cntTxRx->setText(QString("TX: %1\nRX: %2\nError: %3\nWarning: %4").arg(canTxCnt).arg(canRxCnt).arg(canLogError).arg(canLogWarning));

    if(canOpenIsOk)
    {
        for(int idx = 0; idx < CELL_NUM_MAX; idx++)
        {
            if(simData.simIdValid[idx] == SimId_Valid)
            {
                ClearMsgData(msg.data);
                msg = MakeUpCanPacketId( CMD_CODE_GEN_READ_PARA , CMD_PAGE_GEN , UPPER_PC_ADDR , (uint8_t)(idx+1) , CMD_MODE_R );
                (void)sendCANData(msg);
                Delay_MSec(5);
            }
        }
    }


    //qDebug() << "RefreshUITimerOut";
    TempDebugRowNum = simData.u8DeviceNum;
    ui->tableWidget_CellInfo->setRowCount(simData.u8DeviceNum);
    //ui->tableWidget_CellInfo->setEditTriggers(QAbstractItemView::CurrentChanged);

    for(int idx = 0 ; idx < CELL_NUM_MAX; idx++)
    {
        if(simData.simIdValid[idx] == SimId_Valid)
        {

            //地址
            ui->tableWidget_CellInfo->setItem(rows,0,new QTableWidgetItem(QString::number(idx+1)));

            //电流配置
            ui->tableWidget_CellInfo->setItem(rows,1,new QTableWidgetItem(QString::number(simData.dbCellCurrSet[idx])));

            //电流
            ui->tableWidget_CellInfo->setItem(rows,2,new QTableWidgetItem(QString::number(simData.dbCellCurr[idx])));
            if(simData.dbCellCurr[idx] >= 0 )
            {
                ui->tableWidget_CellInfo->item(rows,2)->setForeground(Qt::darkBlue);
            }
            else
            {
                ui->tableWidget_CellInfo->item(rows,2)->setForeground(Qt::darkCyan);
            }

            //电压
            ui->tableWidget_CellInfo->setItem(rows,3,new QTableWidgetItem(QString::number(simData.dbCellVolt[idx])));  

            //继电器
            if(simData.u8ReleyState[idx])
            {
                ui->tableWidget_CellInfo->setItem(rows,4,new QTableWidgetItem(QString("ON")));
                ui->tableWidget_CellInfo->item(rows,4)->setForeground(Qt::darkGreen);
            }
            else
            {
                ui->tableWidget_CellInfo->setItem(rows,4,new QTableWidgetItem(QString("OFF")));
                ui->tableWidget_CellInfo->item(rows,4)->setForeground(Qt::darkRed);
            }

            //温度
            ui->tableWidget_CellInfo->setItem(rows,5,new QTableWidgetItem(QString::number(simData.dbCellTemperature[idx])));

            if(rows < TempDebugRowNum)
            {
                //item = ui->tableWidget_CellInfo->item(rows,2);
                //item->setFlags(Qt::ItemIsEnabled);//设置改item不可修改；
                rows++;
            }
            else
            {
                rows--;
            }
        }
    }
}

void MainWindow::CellList_Sellect()
{
//    qDebug() << "xxxxxxxxxxxxxxxxxxxxxx";
//    if(ui->tableWidget_CellInfo->selectedItems().empty())
//    {
//        qDebug() << "empty sel";
//        qDebug() << "empty sel";
//        qDebug() << "empty sel";
//    }
//    else
//    {
//        qDebug() << "items sel";
//        qDebug() << "items sel";
//        qDebug() << "items sel";
//    }
//    qDebug() << "xxxxxxxxxxxxxxxxxxxxxx";
}


void MainWindow::CellList_Modifier()
{
    double setVal = ui->doubleSpinBox->value();
    can_message_t msg;
    uint8_t addrTemp;

    if(cmdRuningStat)
    {
        return;
    }

    timerForUI.stop();

    //非空，按列遍历
    if(!ui->tableWidget_CellInfo->selectedItems().empty())
    {
        for(auto selItem:ui->tableWidget_CellInfo->selectedItems())
        {
            if(selItem->row()<simData.u8DeviceNum)
            {
                //找地址
                addrTemp = ui->tableWidget_CellInfo->item(selItem->row(),0)->text().toInt();
                qDebug() << addrTemp;
                switch(selItem->column())
                {
                    //地址
                    case 0:
                    if(setVal>=0&&setVal<=60)
                    {
                        msg = setAddrDataPacket(addrTemp,setVal);
                        sendCANData(msg);
                    }
                    break;

                    //限流
                    case 1:
                        if(setVal<0)
                        {
                            setVal = 0;
                        }
                        else if(setVal>1100)
                        {
                            setVal = 1100;
                        }

                        simData.dbCellCurrSet[addrTemp-1] = setVal;

                        msg = setCurrentDataPacket(addrTemp,setVal);
                        sendCANData(msg);
                    break;


                    //电压
                    case 3:
                        msg = setVoltageDataPacket(addrTemp,setVal);
                        sendCANData(msg);

                    //继电器
                    case 4:
                        msg = setRelayDataPacket(addrTemp,setVal);
                        sendCANData(msg);

                    //电流--不可修改
                    case 2:
                    //温度--不可修改
                    case 5:
                    default:
                    break;
                }
                qDebug()<<"--------------------------------";


            }
        }

    }

    timerForUI.start();
}


void MainWindow::Single_Cmd_Run( int volt , int idxStart , int idxStop , int time)
{
    double voltGap[CELL_NUM_MAX] = {0};
    double times = time*2;
    can_message_t msg;

    if(times < 1)
    {
        times = 1;//至少要跑一次
    }

    //执行
    for(int t = 0; t < times; t++)
    {
        for(int cell = idxStart; cell <= idxStop; cell++)
        {
            voltGap[cell] = (volt-simData.dbCellVolt[cell-1])/(times-t);//动态更新的间隔
            if(voltGap[cell] > 0)
            {
                if ((simData.dbCellVolt[cell-1] + voltGap[cell]) > volt)
                {
                    //电压匹配了
                    msg = setVoltageDataPacket(cell,volt);
                    sendCANData(msg);
                    qDebug() << "SEND CMD : SET CELL ↑ " << cell <<" voltage to "<< volt <<" mV ";

                }
                else
                {
                    msg = setVoltageDataPacket(cell,simData.dbCellVolt[cell-1] + voltGap[cell]);
                    sendCANData(msg);
                    qDebug() << "SEND CMD : SET CELL ↑ " << cell <<" voltage to "<< simData.dbCellVolt[cell-1] + voltGap[cell] <<" mV ";
                }
            }
            else if(voltGap[cell] < 0)
            {
                if ((simData.dbCellVolt[cell-1] + voltGap[cell]) < volt)
                {
                    //电压匹配了
                    msg = setVoltageDataPacket(cell,volt);
                    sendCANData(msg);
                    qDebug() << "SEND CMD : SET CELL ↓ " << cell <<" voltage to "<< volt <<" mV ";
                }
                else
                {
                    msg = setVoltageDataPacket(cell,simData.dbCellVolt[cell-1] + voltGap[cell]);
                    sendCANData(msg);
                    qDebug() << "SEND CMD : SET CELL ↓ " << cell <<" voltage to "<< simData.dbCellVolt[cell-1] + voltGap[cell] <<" mV ";
                }
            }
            if(cmdRuningStat == false)
            {
                return;
            }
        }
        runProgress+=1;
        ui->progressBar->setValue(runProgress*100/runProgressTotal);

//        ui->progressBar->repaint();
        Delay_MSec(500);
        qDebug() << "------------------------100ms gap------------------------";
    }

}

void MainWindow::CmdList_Add()
{
    int rowTemp = ui->tableWidget_Cmd->rowCount();

    ui->tableWidget_Cmd->setRowCount(rowTemp+1);

    QTableWidgetItem* p_check = new QTableWidgetItem();
    p_check->setCheckState(Qt::Unchecked);
    ui->tableWidget_Cmd->setItem(rowTemp,0,p_check);

    ui->tableWidget_Cmd->repaint();

}

void MainWindow::CmdList_Delete()
{
    for(int row=ui->tableWidget_Cmd->rowCount()-1;row>=0;row--)
    {
        if(ui->tableWidget_Cmd->item(row,0)->checkState() == Qt::Checked)
        {
            ui->tableWidget_Cmd->removeRow(row);
        }
    }
}

void MainWindow::CmdList_Run()
{
    //配置按键和状态
    cmdRuningStat = true;
    ui->valSetBotton->setEnabled(false);
    ui->AddButton->setEnabled(false);
    ui->DelButton->setEnabled(false);
    ui->RunButton->setEnabled(false);
    ui->StopButton->setEnabled(true);

    ui->tableWidget_Cmd->setEditTriggers(QAbstractItemView::NoEditTriggers);

    int cmdTotalNum = 0;

    cmdData = {{0}};
    runProgressTotal = 0;
    runProgress = 0;

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setMinimum(0);
    ui->progressBar->reset();

    //取出内容
    for(int row=0;row<ui->tableWidget_Cmd->rowCount();row++)
    {
        double idxStartRead = 0;
        double idxStopRead = 0;
        double voltRead = 0;
        double timeRead = 0;


        if(ui->tableWidget_Cmd->item (row,1)!=NULL)
        {
            idxStartRead = ui->tableWidget_Cmd->item(row,1)->text().toDouble();
        }
        else
        {
            ui->tableWidget_Cmd->item(row,0)->setText("无效");
        }

        if(ui->tableWidget_Cmd->item (row,2)!=NULL)
        {
            idxStopRead = ui->tableWidget_Cmd->item(row,2)->text().toDouble();
        }
        else
        {
            ui->tableWidget_Cmd->item(row,0)->setText("无效");
        }

        if(ui->tableWidget_Cmd->item (row,3)!=NULL)
        {
            voltRead = ui->tableWidget_Cmd->item(row,3)->text().toDouble();
        }
        else
        {
            ui->tableWidget_Cmd->item(row,0)->setText("无效");
        }

        if(ui->tableWidget_Cmd->item (row,4)!=NULL)
        {
            timeRead = ui->tableWidget_Cmd->item(row,4)->text().toDouble();
        }
        else
        {
            ui->tableWidget_Cmd->item(row,0)->setText("无效");
        }

        qDebug() << idxStartRead<<"~~"<< idxStopRead<<"~~"<< voltRead<<"~~"<< timeRead;

        if( idxStartRead > 0
            && idxStopRead <= 60
            && idxStartRead <= idxStopRead
            && voltRead >= 0
            && voltRead <= 5500
            && timeRead >= 0
           )
        {
            ui->tableWidget_Cmd->item(row,0)->setText(QString::number(row+1));
            qDebug()<<"有效";
            cmdData.idxStart[cmdTotalNum] = idxStartRead;
            cmdData.idxStop[cmdTotalNum]  = idxStopRead;
            cmdData.volt[cmdTotalNum]     = voltRead;
            cmdData.time[cmdTotalNum]     = timeRead;

            cmdData.rowIdx[cmdTotalNum]     = row;
            runProgressTotal += timeRead*2;
            cmdTotalNum++;
        }
        else
        {
            ui->tableWidget_Cmd->item(row,0)->setText("无效");
            qDebug()<<"无效";
        }
    }


    //开始执行
    for(int i = 0; i < cmdTotalNum; i++)
    {
        if(cmdRuningStat == false)
        {
            break;
        }
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],0)->setBackground(QColorConstants::Green);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],1)->setBackground(QColorConstants::Green);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],2)->setBackground(QColorConstants::Green);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],3)->setBackground(QColorConstants::Green);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],4)->setBackground(QColorConstants::Green);
        ui->tableWidget_Cmd->repaint();

        qDebug()<<"run cmd:"<<i+1<<"row:"<<cmdData.rowIdx[i];
        Single_Cmd_Run( cmdData.volt[i], cmdData.idxStart[i], cmdData.idxStop[i], cmdData.time[i] );

        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],0)->setBackground(QColorConstants::DarkGreen);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],1)->setBackground(QColorConstants::DarkGreen);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],2)->setBackground(QColorConstants::DarkGreen);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],3)->setBackground(QColorConstants::DarkGreen);
        ui->tableWidget_Cmd->item(cmdData.rowIdx[i],4)->setBackground(QColorConstants::DarkGreen);
        ui->tableWidget_Cmd->repaint();
    }

    CmdList_Stop();
}

void MainWindow::CmdList_Stop()
{
    cmdRuningStat = false;
    ui->valSetBotton->setEnabled(true);
    ui->AddButton->setEnabled(true);
    ui->DelButton->setEnabled(true);
    ui->RunButton->setEnabled(true);
    ui->StopButton->setEnabled(false);

    ui->tableWidget_Cmd->setEditTriggers(QAbstractItemView::CurrentChanged);

    for(int row=0;row<ui->tableWidget_Cmd->rowCount();row++)
    {
        for(int j=0; j < 5;j++)
        {
            if(ui->tableWidget_Cmd->item(row,j)!=NULL)
            {
                if(ui->tableWidget_Cmd->item(row,j)->background() == QColorConstants::DarkGreen
                    ||ui->tableWidget_Cmd->item(row,j)->background() == QColorConstants::Green)
                {
                    ui->tableWidget_Cmd->item(row,j)->setBackground(QColorConstants::White);
                }
            }

        }
    }

    ui->tableWidget_Cmd->repaint();

    cmdData = {{0}};
    ui->progressBar->setValue(0);

}

void MainWindow::CmdList_Import()
{
    QString strFile = QFileDialog::getOpenFileName(this,QStringLiteral("save"),"",tr("Exel file(*.xls *.xlsx)"));
    qDebug()<<strFile;
    if (strFile.isEmpty())
    {
        return;
    }

    QAxObject excel("Excel.Application"); //加载Excel驱动
    excel.setProperty("Visible", false);//不显示Excel界面，如果为true会看到启动的Excel界面
    QAxObject *work_books = excel.querySubObject("WorkBooks");
    work_books->dynamicCall("Open (const QString&)", strFile); //打开指定文件
    QAxObject *work_book = excel.querySubObject("ActiveWorkBook");
    QAxObject *work_sheets = work_book->querySubObject("Sheets");  //获取工作表
    QString ExcelName;
    static int row_count = 0,column_count = 0;
    int sheet_count = work_sheets->property("Count").toInt();  //获取工作表数目,如下图，有 3 页


    if(sheet_count > 0)
    {
        QAxObject *work_sheet = work_book->querySubObject("Sheets(int)", 1); //设置为 获取第一页 数据
        QAxObject *used_range = work_sheet->querySubObject("UsedRange");
        QAxObject *rows = used_range->querySubObject("Rows");
        row_count = rows->property("Count").toInt();  //获取行数

        QAxObject *column = used_range->querySubObject("Columns");
        column_count = column->property("Count").toInt();  //获取列数
        qDebug() << row_count<<"-"<< column_count;
        //获取第一行第一列数据
        ExcelName = work_sheet->querySubObject("Cells(int,int)", 1,1)->property("Value").toString();
        qDebug() << ExcelName;
        //获取表格中需要的数据
        ui->tableWidget_Cmd->setRowCount(row_count - 1);
        for (int i =2; i <= row_count; i++)
        {
            for (int j = 1; j <= column_count;j++)
            {
                qDebug() << "reading";
                //QString txt = work_sheet->querySubObject("Cells(int,int)", i,j)->property("Value2()").toString(); //获取单元格内容
                QString txt = work_sheet->querySubObject("Cells(int,int)", i,j)->dynamicCall("Value2()").toString();

                QTableWidgetItem* p_item = new QTableWidgetItem(txt);
                if(j-1 == 0)
                {
                    p_item->setCheckState(Qt::Unchecked);
                }

                ui->tableWidget_Cmd->setItem(i-2,j-1,p_item );
                qDebug() << "row "<<i-2<<",col "<<j-1<<" set to :"<<txt;


            }
        }
        ui->tableWidget_Cmd->repaint();

        work_book->dynamicCall("Close(Boolean)", false);  //关闭文件
        excel.dynamicCall("Quit(void)");  //退出
    }



}

void MainWindow::CmdList_Export()
{
    //获取保存路径
       QString filepath=QFileDialog::getSaveFileName(this,tr("Save"),".",tr(" (*.xlsx)"));
       if(!filepath.isEmpty()){
           QAxObject *excel = new QAxObject(this);
           //连接Excel控件
           excel->setControl("Excel.Application");
           //不显示窗体
           excel->dynamicCall("SetVisible (bool Visible)","false");
           //不显示任何警告信息。如果为true那么在关闭是会出现类似“文件已修改，是否保存”的提示
           excel->setProperty("DisplayAlerts", false);
           //获取工作簿集合
           QAxObject *workbooks = excel->querySubObject("WorkBooks");
           //新建一个工作簿
           workbooks->dynamicCall("Add");
           //获取当前工作簿
           QAxObject *workbook = excel->querySubObject("ActiveWorkBook");
           //获取工作表集合
           QAxObject *worksheets = workbook->querySubObject("Sheets");
           //获取工作表集合的工作表1，即sheet1
           QAxObject *worksheet = worksheets->querySubObject("Item(int)",1);
           //设置表头值
           for(int i=1;i<ui->tableWidget_Cmd->columnCount()+1;i++)
           {
               //设置设置某行某列
               QAxObject *Range = worksheet->querySubObject("Cells(int,int)", 1, i);
               Range->dynamicCall("SetValue(const QString &)",ui->tableWidget_Cmd->horizontalHeaderItem(i-1)->text());
           }
           //设置表格数据
           for(int i = 1;i<ui->tableWidget_Cmd->rowCount()+1;i++)
           {
               for(int j = 1;j<ui->tableWidget_Cmd->columnCount()+1;j++)
               {
                   QAxObject *Range = worksheet->querySubObject("Cells(int,int)", i+1, j);
                   Range->dynamicCall("SetValue(const QString &)",ui->tableWidget_Cmd->item(i-1,j-1)->data(Qt::DisplayRole).toString());
               }
           }
           workbook->dynamicCall("SaveAs(const QString&)",QDir::toNativeSeparators(filepath));//保存至filepath
           workbook->dynamicCall("Close()");//关闭工作簿
           excel->dynamicCall("Quit()");//关闭excel
           delete excel;
           excel=NULL;
           qDebug() << "\n导出成功！！！";
       }

}

