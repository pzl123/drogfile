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

#include <linux/can.h>

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
        qDebug() << "年份:" << ymd;
        QDate date = QDate::fromString(ymd, "yyyy-MM-dd");
        QTime time = QTime::fromString(timestr);

        struct can_frame frame = {0};
        frame.can_id = canIdStr.toUInt(nullptr, 16);
        QStringList dataList = dataStr.split(' ', Qt::SkipEmptyParts);
        for (int i = 0; i < dataList.size() && i < 8; ++i)
        {
            frame.data[i] = static_cast<quint8>(dataList[i].toInt(nullptr, 16));
        }
        qDebug() << date.toString("yyyy-MM-dd");
        qDebug() << time.toString("hh:mm:ss.zzzz");
        qDebug() << QString("CAN ID: %1").arg(frame.can_id, 8, 16, QChar('0'));
        for (int i = 0; i < 8; i++)
        {
            qDebug() <<QString("DATA%1: %2").arg(i).arg(frame.data[i], 2, 16, QChar('0'));
        }

    }
    else
    {
        qDebug() << "no match";
    }

}


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setAcceptDrops(true);
    ui->showplainTextEdit->setAcceptDrops(false);
    ui->showplainTextEdit->setReadOnly(true);
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
    // ui->showplainTextEdit->setPlainText(fullpath);
    ui->showplainTextEdit->appendPlainText(fullpath);
    ui->showplainTextEdit->ensureCursorVisible();
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
    QSize sz = ui->showplainTextEdit->size();
    ui->showplainTextEdit->move(5, 5);
    ui->showplainTextEdit->resize(this->width() - 10, this->height() - 10);
}
