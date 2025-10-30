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

void adjustLineEditWidth(QLineEdit* lineEdit, const QString& text = QString())
{
    QString actualText = text.isEmpty() ? lineEdit->text() : text;

    QFontMetrics fm(lineEdit->font());
    int textWidth = fm.horizontalAdvance(actualText); // Qt 5.11+

    // 获取样式中的默认边框宽度（包括 padding）
    int frameWidth = lineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

    // 总宽度 = 文本宽度 + 左右边距（2 * frameWidth）
    int totalWidth = textWidth + 2 * frameWidth + 10; // +10 是额外内边距或缓冲

    lineEdit->setFixedWidth(totalWidth);
}

static void processLine(const QString &line)
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
        deal_with_frame(frame);
    }
    else
    {
        qDebug() << "no match";
    }

}


static void model_init(QSqlDatabase &db, QSqlTableModel *model)
{
    QSqlQuery query(db);
    if (!query.exec("DROP TABLE IF EXISTS student"))
    {
        qDebug() << "DROP TABLE 失败:" << query.lastError().text();
    }

    if (!query.exec("CREATE TABLE student (id INTEGER PRIMARY KEY, name TEXT, grades REAL)"))
    {
        qDebug() << "CREATE TABLE 失败:" << query.lastError().text();
    }

    bool success = query.exec(R"(
    INSERT INTO student (id, name, grades)
    VALUES
        (1, 'pzl', 1000.0),
        (2, 'a', 2000.0),
        (3, 'b', 3000.0),
        (4, 'c', 4000.0),
        (5, 'd', 6000.0),
        (6, 'e', 7000.0),
        (7, 'f', 8000.0),
        (8, 'g', 9000.0),
        (9, 'h', 10000.0)
)");
    if (!success)
    {
        qDebug() << "INSERT 失败:" << query.lastError().text();
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

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    curRecNo = 0;
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/home/zlgmcu/Desktop/qt_project/drogfile/drogfile.db");
    if (db.open())
    {
        m_model = new QSqlTableModel(this, db);
        model_init(db, m_model);


/*
 * 通过创建 QItemSelectionModel 对象 theSelection 并关联到 tabModel模型，将数据模型和选择模型关联到 ui->tableView，并设置选择模式为行选择模式。
 */
        QItemSelectionModel *theSelection = new QItemSelectionModel(m_model);
        connect(theSelection, &QItemSelectionModel::currentRowChanged,
                this, &Widget::on_currentRowChanged);

        ui->m_tableView->setModel(m_model);
        ui->m_tableView->setSelectionModel(theSelection);
        ui->m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    }
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Widget::timerstart);

    this->setAcceptDrops(true);
    ui->showplainTextEdit->setAcceptDrops(false);
    ui->showplainTextEdit->setReadOnly(true);
    ui->dcdc_id_lineEdit->setText("7");
    qDebug() << "waitting for drag";
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
    qDebug() << fullpath;
    QFileInfo fileInfo(fullpath);
    if (false == fileInfo.exists())
    {
        ui->showplainTextEdit->appendPlainText("文件不存在: " + fullpath);
        return;
    }
    ui->showplainTextEdit->setPlainText(fullpath);
    // ui->showplainTextEdit->appendPlainText(fullpath);
    // ui->showplainTextEdit->ensureCursorVisible();
    QFile file(fullpath);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        int lineNumber = 1;
        while (!in.atEnd())
        {
            QString line = in.readLine();
            // qDebug() << "第" << lineNumber << "行:" << line;
            processLine(line);

            lineNumber++;
        }
    }
}

void Widget::resizeEvent(QResizeEvent *event)
{
    // QSize sz = ui->showplainTextEdit->size();
    // ui->showplainTextEdit->move(5, 5);
    // ui->showplainTextEdit->resize(this->width() - 10, this->height() - 10);
}

void Widget::timerstart()
{
    if (curRecNo >= m_model->rowCount())
    {
        qDebug() << "read end";
        QMessageBox::information(this, "data end", "data read end");
        m_timer->stop();
        ui->startpushButton->setText("statr");
        return;
    }
    QSqlRecord  curRec = m_model->record(curRecNo);
    QString valuestr = curRec.value("grades").toString();
    ui->set_output_vol_lineEdit->setText(valuestr);
    qDebug() << "valuestr: " << valuestr << " curRecNo: " << curRecNo;
    curRecNo += 1;
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

void Widget::on_currentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
    qDebug() << "previous:" << previous.row() << " current:" << current.row();
    curRecNo = current.row();
}
