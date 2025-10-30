#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QItemSelectionModel>
#include <QDebug>
#include <QSqlRecord>
#include <QMessageBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;
    QTimer *m_timer;
    QSqlTableModel *m_model;
    int curRecNo;
private slots:
    void timerstart();
    void on_startpushButton_clicked();
    void on_currentRowChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
#endif // WIDGET_H
