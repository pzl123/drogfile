#include "widget.h"
#include "./ui_widget.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QFileDialog>
#include <QFile>
#include <QRegularExpression>
#include <QDate>
#include <QTime>
#include <QTimer>
#include <QStyle>
#include <QSqlDriver>
#include <QStandardItemModel>

#include <random>
#include <regex>

#include <linux/can.h>
#include "dcdc.h"



static void processLine(Widget *widget_m, const QString &line)
{
    // qDebug() << line;
    QRegularExpression re(R"zzz(\((\d{4}-\d{2}-\d{2})\s+(\d{2}:\d{2}:\d{2}\.\d+)\)\s+(\w+)\s+([0-9a-fA-F]+)\s+\[\d\]\s+(([0-9a-fA-F]{2}(?:\s+[0-9a-fA-F]{2})*)?))zzz");
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch())
    {

        QString ymd       = match.captured(1);
        QString timestr   = match.captured(2);
        QString canIdStr  = match.captured(4);
        QString dataStr   = match.captured(5);
        // qDebug() << "年份:" << ymd;
        QDate date = QDate::fromString(ymd, "yyyy-MM-dd");
        QTime time = QTime::fromString(timestr);

        struct can_frame frame = {0};
        frame.can_id = canIdStr.toUInt(nullptr, 16);
        QStringList dataList = dataStr.split(' ', Qt::SkipEmptyParts);
        for (int i = 0; i < dataList.size() && i < 8; ++i)
        {
            frame.data[i] = static_cast<quint8>(dataList[i].toInt(nullptr, 16));
            frame.can_dlc = i + 1;
        }
        print_can_frame(frame);
        // qDebug() << date.toString("yyyy-MM-dd");
        // qDebug() << time.toString("hh:mm:ss.zzzz");
        // qDebug() << QString("CAN ID: %1").arg(frame.can_id, 8, 16, QChar('0'));
        // for (int i = 0; i < 8; i++)
        // {
        //     qDebug() <<QString("DATA%1: %2").arg(i).arg(frame.data[i], 2, 16, QChar('0'));
        // }
        deal_with_frame(widget_m, frame);
    }
    else
    {
        qDebug() << "no match";
    }

}


static void model_init(QSqlDatabase &db, QString& table_name, QSqlTableModel *model)
{
    QSqlQuery query(db);
    // if (!query.exec("DROP TABLE IF EXISTS student"))
    // {
    //     qDebug() << "DROP TABLE 失败:" << query.lastError().text();
    // }

    if (!query.exec("CREATE TABLE IF NOT EXISTS student (id INTEGER PRIMARY KEY, logstr TEXT)"))
    {
        qDebug() << "CREATE TABLE 失败:" << query.lastError().text();
    }

    model->setTable(table_name);

    if (!model->select())
    {
        qDebug() << "select() 失败:" << model->lastError().text();
    }

    // int idCol = model->fieldIndex("id");
    // int nameCol = model->fieldIndex("name");
    // int gradesCol = model->fieldIndex("grades");

    // model->setHeaderData(idCol, Qt::Horizontal, "id");
    // model->setHeaderData(nameCol, Qt::Horizontal, "name");
    // model->setHeaderData(gradesCol, Qt::Horizontal, "grades");
}

void Widget::timerstart()
{
    QSqlQuery query(getdatabase());
    QString table_name = "student";
    query.prepare(QString("SELECT COUNT(*) FROM %1").arg(table_name));
    int all_row_num = 0;
    if (query.exec() && query.next())
    {
        all_row_num = query.value(0).toInt();
    }
    int *currecno = getcurRecNo();
    if (*currecno >= all_row_num)
    {
        qDebug() << "read end";
        QMessageBox::information(this, "data end", "data read end");
        gettimer()->stop();
        getui()->startpushButton->setText("statr");
        return;
    }

    QSqlRecord  curRec = getmodel()->record(*currecno);
    QString valuestr = curRec.value("logstr").toString();
    qDebug() << *currecno << "m_model->rowCount is :" << getmodel()->rowCount();
    processLine(this, valuestr);
    (*currecno) += 1;
}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    m_curRecNo = 0;
    ui->timelineEdit->setText("1000");

    QString data_path = "/home/zlgmcu/Desktop/qt_project/drogfile/drogfile.db";
    QString table_name = "dcdc_frame";
    QString conn_name = "frame";
    if (open_database(data_path, table_name, conn_name))
    {
        QString create_str = "CREATE TABLE IF NOT EXISTS " + table_name + "(id INTEGER PRIMARY KEY, timestamp TEXT, \
            can_id INTEGER, \
            can_len INTEGER,\
            can_data0 INTEGER, \
            can_data1 INTEGER, \
            can_data2 INTEGER, \
            can_data3 INTEGER, \
            can_data4 INTEGER, \
            can_data5 INTEGER, \
            can_data6 INTEGER, \
            can_data7 INTEGER)";
        drop_table_in_db(table_name);
        create_table_in_db(create_str ,table_name);
        m_model = new QSqlTableModel(this, m_db);
        model_init(m_db, table_name,  m_model);
/*
 * 通过创建 QItemSelectionModel 对象 theSelection 并关联到 tabModel模型，将数据模型和选择模型关联到 ui->tableView，并设置选择模式为行选择模式。
 */
        QItemSelectionModel *theSelection = new QItemSelectionModel(m_model);

        ui->m_tableView->setModel(m_model);
        ui->m_tableView->setSelectionModel(theSelection);
        ui->m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->m_tableView->hideColumn(0);
        if (m_model->columnCount() > 1)
        {
            QHeaderView *header = ui->m_tableView->horizontalHeader();
            header->setSectionResizeMode(m_model->columnCount() - 1, QHeaderView::Stretch);
        }
    }

    QStandardItemModel *m_dcdc_model = new QStandardItemModel(12, 7, this);
    m_dcdc_model->setHorizontalHeaderItem(0, new QStandardItem("ID"));
    m_dcdc_model->setHorizontalHeaderItem(1, new QStandardItem("vol"));
    m_dcdc_model->setHorizontalHeaderItem(2, new QStandardItem("set_vol"));
    m_dcdc_model->setHorizontalHeaderItem(3, new QStandardItem("cur"));
    m_dcdc_model->setHorizontalHeaderItem(4, new QStandardItem("set_cur"));
    m_dcdc_model->setHorizontalHeaderItem(5, new QStandardItem("switch"));
    m_dcdc_model->setHorizontalHeaderItem(6, new QStandardItem("set_switch"));

    ui->dcdc_msg_tableView->setModel(m_dcdc_model);
    ui->dcdc_msg_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);


    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Widget::timerstart);

    this->setAcceptDrops(true);
    ui->showplainTextEdit->setAcceptDrops(false);
    ui->showplainTextEdit->setReadOnly(true);
    ui->dcdc_id_lineEdit->setText("7");
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::open_database(const QString& data_path, const QString& table_name, const QString& conn_name)
{
    if (QSqlDatabase::contains(conn_name))
    {
        QSqlDatabase::removeDatabase(conn_name);
        // db = QSqlDatabase::database(conn_name);
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", conn_name);
    // db.setHostName("aaaaa");
    m_db.setDatabaseName(data_path);
    // db.setUserName("dbuse");
    // db.setPassword("dbpassword");

    if (!m_db.open())
    {
        QMessageBox::critical(this, "Error", "Failed to open database. path :" + data_path + "database error: " + m_db.lastError().text());
        return false;
    }
    return true;
}

void Widget::close_database()
{
    if (m_db.isOpen())
    {
        m_db.close();
    }
}

void Widget::drop_table_in_db(const QString& table_name)
{
    QString delete_str = "DROP TABLE IF EXISTS ";
    QString final_str = delete_str + table_name;

    if (!m_db.isValid() || !m_db.isOpen())
    {
        qWarning() << "Database connection is invalid or not open.";
        return;
    }

    QSqlQuery query(m_db);
    if (!query.exec(final_str))
    {
        qWarning() << "Failed to drop table" << table_name
                   << ": " << query.lastError().text();
    }
}

void Widget::create_table_in_db(QString create_str, const QString &table_name)
{
    if (!m_db.isValid() || !m_db.isOpen())
    {
        qWarning() << "Database connection is invalid or not open.";
        return;
    }

    QSqlQuery query(m_db);
    if (!query.exec(create_str))
    {
        qWarning() << "Failed to create table" << table_name
                   << ": " << query.lastError().text();
    }
}

void Widget::display_all_db_class()
{
    QStringList drives = QSqlDatabase::drivers();
    foreach (QString driver, drives)
    {
        qDebug() << driver;
    }
}

void Widget::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "somthing drag";
    if (event->mimeData()->hasUrls())
    {
        QString filename = event->mimeData()->urls().at(0).fileName();
        QFileInfo fileINfo(filename);
        QString ex = fileINfo.suffix().toUpper();
        if (ex == "LOG")
        {
            event->acceptProposedAction();
            qDebug() << filename + ":sufix:" + ex;
        }
        else
        {
            event->ignore();
        }
    }
    else
    {
        event->ignore();
    }
}

static int32_t split_str_2_frame_data(std::string& str, unsigned int *data, unsigned int max_len)
{
    if (23 != str.size())
    {
        return -1;
    }
    std::regex re(R"(^([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})\s+([0-9a-fA-F]{2})$)");
    std::smatch match;
    if (std::regex_match(str, match, re))
    {
        for (size_t i = 1; i <= 8; ++i)
        {
            data[i-1] = static_cast<unsigned int>(std::stoi(match[i].str(), nullptr, 16));
            // data[i-1] = static_cast<unsigned int>(data[i-1]);
            // std::cout << "data[" << (i-1) << "]: "<< std::hex << data[i-1] << " " << std::endl;
        }
    }
    else
    {
        std::cout << "error regex_match:" << str << std::endl;
    }
    return 0;
}

static void string_info_2_dcdc_insert_msg(QString& line_str_, linestr_2_dbinfo_t &insert_msg)
{
    std::string line_str = line_str_.toStdString();
    std::regex re(R"(\((\d{4}-\d{2}-\d{2})\s+(\d{2}:\d{2}:\d{2}\.\d+)\)\s+(\w+)\s+([0-9a-zA-Z]+)\s+\[(\d)\]\s+(([0-9a-zA-Z]){2}(?:\s+[0-9a-zA-Z]{2})*))");
    std::smatch match;
    if (std::regex_search(line_str, match, re))
    {
        std::cout << "data:" << match[1] << std::endl;
        std::cout << "time:" << match[2] << std::endl;
        std::cout << "can_itf:" << match[3] << std::endl;
        std::cout << "can_id:" << match[4] << std::endl;
        std::cout << "can_len:" << match[5] << std::endl;
        std::cout << "can_data:" << match[6] << std::endl;
    }
    else
    {
        std::cout << "regex error:" << line_str << std::endl;
        return;
    }
    insert_msg.can_id = static_cast<unsigned int>(std::stoi(match[4].str(), nullptr, 16));
    insert_msg.can_len =  static_cast<uint8_t>(std::stoi(match[5].str(), NULL, 10));
    insert_msg.timestamp  = match[1].str() + " " + match[2].str();
    std::string can_data_str = match[6].str();
    if (insert_msg.can_len <= MAX_LEN)
    {
        if (-1 == split_str_2_frame_data(can_data_str, insert_msg.can_data, MAX_LEN))
        {
            std::cout << "split str error: " << can_data_str << std::endl;
            return;
        }
    }
}


void Widget::dropEvent(QDropEvent *event)
{
    QString fullpath = event->mimeData()->urls().at(0).path();
    qDebug() << "file path:" << fullpath;

    QFileInfo fileInfo(fullpath);
    if (!fileInfo.exists())
    {
        ui->showplainTextEdit->appendPlainText("file isn't exit: " + fullpath);
        return;
    }

    ui->showplainTextEdit->setPlainText(fullpath);

    QFile file(fullpath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->showplainTextEdit->appendPlainText("can't open: " + fullpath);
        return;
    }

    QTextStream in(&file);


    // 开启事务
    QSqlDriver *drv = m_db.driver();

    if (drv && drv->hasFeature(QSqlDriver::Transactions))
    {
        m_db.transaction();

        QString sql = QString("INSERT INTO %1 (timestamp, can_id, can_len, can_data0, can_data1, can_data2, can_data3, can_data4, can_data5, can_data6, can_data7) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)").arg("dcdc_frame");
        qDebug() << sql;


        int lineNumber = 1;
        while (!in.atEnd())
        {
            QString line = in.readLine();
            linestr_2_dbinfo_t msg{};
            msg.id = lineNumber;
            string_info_2_dcdc_insert_msg(line, msg);

            QSqlQuery query(m_db);
            query.prepare(sql);

            query.addBindValue(QString::fromStdString(msg.timestamp));
            query.addBindValue(msg.can_id);
            query.addBindValue(msg.can_len);

            for (int i = 0; i < 8; ++i)
            {
                if (i < msg.can_len)
                {
                    query.addBindValue(msg.can_data[i]);
                }
                else
                {
                    query.addBindValue(0);
                }
            }


            if (!query.exec())
            {
                // qDebug() << "第" << lineNumber << "行插入失败:" << query.lastError().text();
            } else {
                // qDebug() << "第" << lineNumber << "行插入成功:" << line;
            }
            // query.clear();
            lineNumber++;
        }

        file.close();

        // 提交事务
        if (!m_db.commit())
        {
            qDebug() << "transaction commit error:" << m_db.lastError().text();
            m_db.rollback();  // 回滚
        } else {
            qDebug() << "all data insert success, commit transaction";
        }
        m_model->select();

        ui->showplainTextEdit->appendPlainText(QString("import success, %1 row").arg(lineNumber - 1));
    }
}

void Widget::resizeEvent(QResizeEvent *event)
{
    // QSize sz = ui->showplainTextEdit->size();
    // ui->showplainTextEdit->move(5, 5);
    // ui->showplainTextEdit->resize(this->width() - 10, this->height() - 10);
}


void Widget::on_startpushButton_clicked()
{
    if (m_timer && m_timer->isActive())
    {
        m_timer->stop();
        ui->startpushButton->setText("statr");
    }
    else
    {
        QString time_idel = ui->timelineEdit->text();
        m_timer->start(time_idel.toUInt());
        ui->startpushButton->setText("pause");
    }
}

void Widget::onOutputVoltageChanged(int index, float voltage)
{

    this->m_dcdc[index].set_output_vol(voltage);
    ui->set_output_vol_lineEdit->setText(QString::number(this->m_dcdc[index].get_output_vol()));

}
