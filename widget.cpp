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

#include <random>

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


static void model_init(QSqlDatabase &db, QSqlTableModel *model)
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

    model->setTable("student");

    if (!model->select())
    {
        qDebug() << "select() 失败:" << model->lastError().text();
    }

    int idCol = model->fieldIndex("id");
    int nameCol = model->fieldIndex("name");
    int gradesCol = model->fieldIndex("grades");

    model->setHeaderData(idCol, Qt::Horizontal, "id");
    model->setHeaderData(nameCol, Qt::Horizontal, "name");
    model->setHeaderData(gradesCol, Qt::Horizontal, "grades");
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
    curRecNo = 0;
    ui->timelineEdit->setText("1000");
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/home/zlgmcu/Desktop/qt_project/drogfile/drogfile.db");
    if (db.open())
    {
        m_model = new QSqlTableModel(this, db);
        model_init(db, m_model);
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
    QSqlQuery query(db);

    // 开启事务
    db.transaction();

    int lineNumber = 1;
    while (!in.atEnd())
    {
        QString line = in.readLine();

        // 正确写法：插入到 logstr 字段
        query.prepare("INSERT INTO student (logstr) VALUES (?)");
        query.addBindValue(line);  // 绑定 line 变量的内容

        if (!query.exec())
        {
            // qDebug() << "第" << lineNumber << "行插入失败:" << query.lastError().text();
        } else {
            // qDebug() << "第" << lineNumber << "行插入成功:" << line;
        }
        lineNumber++;
    }

    file.close();

    // 提交事务
    if (!db.commit())
    {
        qDebug() << "transaction commit error:" << db.lastError().text();
        db.rollback();  // 回滚
    } else {
        qDebug() << "all data insert success, commit transaction";
    }
    m_model->select();

    ui->showplainTextEdit->appendPlainText(QString("import success， %1 row").arg(lineNumber - 1));
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

    this->dcdc_m[index].set_output_vol(voltage);
    ui->set_output_vol_lineEdit->setText(QString::number(this->dcdc_m[index].get_output_vol()));

}
