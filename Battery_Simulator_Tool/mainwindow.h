#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <qthread>
#include <qdebug>
#include <QMutex>
#include <frmSaveLog.h>
#include <QTimer>
#include <qtablewidget.h>

#include "drivers/ECan/ECanClass.h"
#include "can_protocol.h"
#include "help.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define CMD_MAX_NUM 100

class CanReadThread : public QThread
{
    Q_OBJECT
private:
    ECanClass *can;
    QMutex m_lock;
    bool m_isCanRun = true;


public:
    bool reciveStatus = true;


    void run() {
        qDebug() << "Recive THread is run";
        m_isCanRun = true;
        do {
            {
                if (!m_isCanRun)  //在每次循环判断是否可以运行，如果不行就退出循环
                {
                    QMutexLocker locker(&m_lock);
                    qDebug() << this->currentThreadId() << "exit by user.";
                    m_isCanRun = true;
                    return;
                }
            }
            if (!can)
            {
                qDebug() << this->currentThreadId() << "exit by can.";
                m_isCanRun = true;
                return;
            }
            can_message_t msg;
            if (can->Read(msg, 2000) != 1)
            {
                ERR_INFO err_info;
                can->GetErrorText(&err_info);
                if (err_info.ErrCode == 0) continue;
                //qDebug() << err_info.ErrCode << err_info.ArLost_ErrData << err_info.Passive_ErrData;
                reciveStatus = false;
                //qWarning() << tr("若此告警信息一直存在请重新进行CAN连接")<<err_info.ErrCode << err_info.ArLost_ErrData << err_info.Passive_ErrData;
                //qDebug() << this->currentThreadId() << "exit on err.";
                //break;
            }
            else
            {
                reciveStatus = true;
                emit receive_data(msg);
            }
        } while (true);
    }
    void stopImmediately()
    {
        QMutexLocker locker(&m_lock);
        can->Uninitialize();
        qDebug() << "stopImmediately";
        m_isCanRun = false;
    }
    void setCan(ECanClass *port)
    {
        QMutexLocker locker(&m_lock);
        this->can = port;
    }
private:
    signals:
    void receive_data(can_message_t msg);//接收数据
};






class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QMainWindow *parent = nullptr);
    ~MainWindow();

    ECanClass *m_can;
    CanReadThread *m_thread;

public slots:
    void deal_msg(const QString &msg);
    void CellList_UIFreasher(void);
    void CellList_Sellect();
    void CellList_Modifier();

    void CmdList_Import();
    void CmdList_Export();

    void Single_Cmd_Run( int volt , int idxStart , int idxStop , int time);
    void CmdList_Add();
    void CmdList_Delete();
    void CmdList_Run();
    void CmdList_Stop();

    void btnClick();




private:
    Ui::MainWindow *ui;
    frmSaveLog savelog;
    help helps;

    bool canOpenIsOk;
    int  canTxCnt;
    int  canRxCnt;
    int  canLogError;
    int  canLogWarning;

    QTimer timerForUI;
    simulatorData_T simData;

    bool cmdRuningStat;
    QTimer timerForCMD;

    int runProgressTotal;
    int runProgress;

    struct
    {
        double idxStart[CMD_MAX_NUM];
        double idxStop[CMD_MAX_NUM];
        double volt[CMD_MAX_NUM];
        double time[CMD_MAX_NUM];
        double rowIdx[CMD_MAX_NUM];
    }cmdData;

    bool devicSearching;
private:
    bool recvieCANData(can_message_t &msg);
    bool sendCANData(can_message_t &msg);
    void SearchDevices();

    signals:

};
#endif // MAINWINDOW_H
