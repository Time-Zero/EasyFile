#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_EasyFile.h"
#include "TcpService.h"
#include "CommonTools.h"
#include "ThreadPool.h"
#include <thread>
#include <qstringlist.h>
#include <qfiledialog.h>
#include <qstandardpaths.h>
#include <qmessagebox.h>
#include <MessageQueue.h>
#include <atomic>

#pragma comment (lib, "ws2_32.lib") 

class EasyFile : public QMainWindow
{
    Q_OBJECT

public:
    EasyFile(QWidget *parent = nullptr);
    ~EasyFile();

public:
    void on_click_accept_start();
    void on_click_accpet_stop();
    void on_click_connect_service();

    void on_click_add_file();
    void on_click_delete_file();
    void on_click_clear_file();

    void on_click_select_file_dst();
    
signals:
    void newMessage(QString mess);

private:
    void start_message_service();
    void text_browser_append_new_message(QString mess);
private:
    Ui::EasyFileClass ui;
    TcpService* p_tcp_;
    QString fileDst;
    std::atomic_bool stop_message_;
};
