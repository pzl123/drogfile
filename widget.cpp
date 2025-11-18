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

typedef enum
{
    FORWARD = 0,
    BACK_OFF = 1
} E_DIRECTION;

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

int Widget::get_total_row(const QString& table_name)
{
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT COUNT(*) FROM %1").arg(table_name));
    if (query.exec() && query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        qWarning() << "Failed to get total row count!";
        return 0;
    }

}

void read_a_row_by_id(Widget *widget_a, int& last_id, E_DIRECTION direction)
{
    QString direction_str = (direction == FORWARD) ? ">" : "<";
    QSqlQuery query(widget_a->getdatabase());
    QString table_name = "dcdc_frame";
    QString prepare_str = QString("SELECT * FROM dcdc_frame WHERE id %1 %2 ORDER BY id LIMIT 1").arg(direction_str).arg(last_id);
    // qDebug() << prepare_str;
    query.prepare(prepare_str);

    QVariantMap tmp_map;
    if (query.exec() && query.next())
    {
        QSqlRecord r = query.record();
        for (int i = 0; i < r.count(); ++i)
        {
            tmp_map[r.fieldName(i)] = r.value(i);
        }

        last_id = tmp_map["id"].toInt();
        // qDebug() << tmp_map;

        struct can_frame frame{};
        frame.can_id = tmp_map["can_id"].toUInt();
        frame.can_dlc = tmp_map["can_len"].toUInt();
        for (int i = 0; i < frame.can_dlc; i++)
        {
            frame.data[i] = tmp_map[QString("can_data%1").arg(i)].toUInt();
        }

        // frame.can_id = 0x06083780;
        // frame.data[0] = 0x03;
        // frame.data[1] = 0x00;
        // frame.data[2] = 0x00;
        // frame.data[3] = 0x21;
        // frame.data[4] = 0x44;
        // frame.data[5] = 0x48;
        // frame.data[6] = 0x00;
        // frame.data[7] = 0x00;
        // print_can_frame(frame);
        deal_with_frame(widget_a, frame);
    }
    else
    {
        qDebug() << "error read_a_row";
    }

}

void Widget::timerstart()
{
    QString table_name = "dcdc_frame";
    int all_row_num = get_total_row(table_name);
    if (m_curRecNo >= all_row_num)
    {
        qDebug() << "read end";
        QMessageBox::information(this, "data end", "data read end");
        gettimer()->stop();
        getui()->startpushButton->setText("statr");
        return;
    }

    read_a_row_by_id(this, m_curRecNo, FORWARD);

}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_manager()
    , m_dcdc{}
{
    ui->setupUi(this);
    m_curRecNo = 0;
    ui->timelineEdit->setText("1000");

    connect(&m_manager, &dcdc_manager::outputVolChanged, this, &Widget::onOutputVoltageChanged);
    connect(&m_manager, &dcdc_manager::setoutputVolChanged, this, &Widget::onsetOutputVoltageChanged);
    connect(&m_manager, &dcdc_manager::outputCurChanged, this, &Widget::onOutputCurrentChanged);
    connect(&m_manager, &dcdc_manager::setoutputCurChanged, this, &Widget::onsetOutputCurrentChanged);
    connect(&m_manager, &dcdc_manager::switchStateChanged, this, &Widget::onswitchStateChanged);
    connect(&m_manager, &dcdc_manager::setswitchStateChanged, this, &Widget::onsetswitchStateChanged);

    for (int i = 0; i < DCDC_NUM; i++)
    {
        struct can_frame frame{};
        m_dcdc[i].set_dcdc_id(frame, i + 1);
        m_dcdc[i].attach(m_manager.get_observer());
    }

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
        // drop_table_in_db(table_name);
        // create_table_in_db(create_str ,table_name);
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

    m_dcdc_model = new QStandardItemModel(12, 7, this);
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

    // connect(this, &Widget::setoutputVolChanged, this, &Widget::onOutputVoltageChanged);


    this->setAcceptDrops(true);
    ui->showplainTextEdit->setAcceptDrops(false);
    ui->showplainTextEdit->setReadOnly(true);
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
        // std::cout << "data:" << match[1] << std::endl;
        // std::cout << "time:" << match[2] << std::endl;
        // std::cout << "can_itf:" << match[3] << std::endl;
        // std::cout << "can_id:" << match[4] << std::endl;
        // std::cout << "can_len:" << match[5] << std::endl;
        // std::cout << "can_data:" << match[6] << std::endl;
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




void Widget::onOutputVoltageChanged(int index, double voltage)
{
    int column = 1;
    qDebug() << "slot voltage change";
    show_msg_2_model(index, column, voltage);
}

void Widget::onsetOutputVoltageChanged(int index, double voltage)
{
    qDebug() << "slot setvoltage change";
    int column = 2;
    show_msg_2_model(index, column, voltage);
}

void Widget::onOutputCurrentChanged(int index, double current)
{
    int column = 3;
    qDebug() << "slot current change";
    show_msg_2_model(index, column, current);
}

void Widget::onsetOutputCurrentChanged(int index, double current)
{
    int column = 4;
    qDebug() << "slot setcurrent change";
    show_msg_2_model(index, column, current);
}

void Widget::onswitchStateChanged(int index, int switstate)
{
    qDebug() << "slot switchstate change";
    int column = 5;
    QString switch_state = (DCDC_TURN_ON == switstate) ? "ON" : "OFF";
    show_msg_2_model(index, column, switch_state);
}

void Widget::onsetswitchStateChanged(int index, int switchstate)
{
    qDebug() << "slot setswitchstate change";
    int column = 6;
    QString switch_state = (DCDC_TURN_ON == switchstate) ? "ON" : "OFF";
    show_msg_2_model(index, column, switch_state);
}
