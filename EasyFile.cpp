#include "EasyFile.h"
#pragma execution_character_set("utf-8")

EasyFile::EasyFile(QWidget* parent)
    : QMainWindow(parent),
    stop_message_(false),
    fileDst(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)),
    p_tcp_(new TcpService)
{
    ui.setupUi(this);

    p_tcp_->set_file_dst(fileDst);

    QMenu* menu1 = new QMenu(QStringLiteral("设置"), this);
    ui.menuBar->addMenu(menu1);
    
    QAction* action1 = new QAction(QStringLiteral("文件保存位置"), this);
    action1->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    menu1->addAction(action1);
    connect(action1, &QAction::triggered, this, &EasyFile::on_click_select_file_dst);

    connect(ui.pushButton_addFile, &QPushButton::clicked, this, &EasyFile::on_click_add_file);
    connect(ui.pushButton_deleteFile, &QPushButton::clicked, this, &EasyFile::on_click_delete_file);
    connect(ui.pushButton_clearFile, &QPushButton::clicked, this, &EasyFile::on_click_clear_file);

    connect(ui.pushButton_listen, &QPushButton::clicked, this, &EasyFile::on_click_accept_start);
    connect(ui.pushButton_close, &QPushButton::clicked, this, &EasyFile::on_click_accpet_stop);
    connect(ui.pushButton_send, &QPushButton::clicked, this, &EasyFile::on_click_connect_service);

    connect(p_tcp_, &TcpService::progressBarValueUpdate, ui.progressBar, &QProgressBar::setValue, Qt::AutoConnection);
    connect(this, &EasyFile::newMessage, this, &EasyFile::text_browser_append_new_message, Qt::AutoConnection);

    start_message_service();
}

EasyFile::~EasyFile()
{
    delete p_tcp_;
    stop_message_.store(true);
}

void EasyFile::on_click_accept_start()
{
    int port = ui.spinBox_recvPort->value();
    std::thread t(&TcpService::accept_start, p_tcp_, port);
    t.detach();
}

void EasyFile::on_click_accpet_stop()
{
    std::thread t(&TcpService::accept_stop, p_tcp_);
    t.detach();
}

void EasyFile::on_click_connect_service()
{
    QString q_ip = ui.lineEdit_ip->text();
    std::string ip = q_ip.toStdString();
    
    int port = ui.spinBox_sendPort->value();

    std::vector<QString> fileList; 
    int count = ui.listWidget->count();
    for (int i = 0; i < count; i++) {
        QString fileName = ui.listWidget->item(i)->text();
        fileList.emplace_back(fileName);
    }

    for (auto it : fileList) {
        auto bound_func = std::bind(&TcpService::connect_server,
            p_tcp_,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3);

        std::function<void(std::string, int, QString)> func = bound_func;
        ThreadPool::instance().commit(func, ip, port, it);    
    }
}

void EasyFile::on_click_add_file()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
                                                QStringLiteral("选择文件"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    
    if (files.isEmpty())
        return;
    ui.listWidget->addItems(files);
}

void EasyFile::on_click_delete_file()
{
    int row = ui.listWidget->currentRow();
    if (row < 0)
        return;

    delete ui.listWidget->takeItem(row);
}

void EasyFile::on_click_clear_file()
{
    int count = ui.listWidget->count();
    if (count == 0)
        return;

    int ret = QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("确认清空?"),
                                    QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
    if (ret != QMessageBox::Ok)
        return;

    for (int i = 0; i < count; i++) {
        delete ui.listWidget->takeItem(0);
    }
}

void EasyFile::on_click_select_file_dst()
{
    fileDst = QFileDialog::getExistingDirectory(this, 
                                                QStringLiteral("选择文件保存位置"),
                                                QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                                                QFileDialog::ShowDirsOnly);

    if (fileDst.isEmpty())
        return;

    p_tcp_->set_file_dst(fileDst);
}

void EasyFile::start_message_service()
{
    std::thread t([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        EDEBUG("start message service");

        while (!stop_message_.load()) {
            std::wstring mess;
            MessageQueue::GetInstance().get(mess);
            
            if (mess.size() == 0)
                continue;

            QString qstr = QString::fromStdWString(mess);
            emit newMessage(qstr);
        }
        });

    t.detach();
}

void EasyFile::text_browser_append_new_message(QString mess)
{
    ui.textBrowser->append(mess);
}

