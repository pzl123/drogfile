#ifndef WIDGET_H
#define WIDGET_H

#include "dcdc.h"

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
    void set_output_voL(float32_t value);
    Ui::Widget* getui() { return ui; }
    QTimer* gettimer() { return m_timer; }
    QSqlTableModel* getmodel() { return m_model; }
    QSqlDatabase& getdatabase() { return db; }
    int *getcurRecNo(){ return &curRecNo; }
    dcdc* getdcdcArray() { return dcdc_m; }
    dcdc& getdcdcAt(int index) {
        Q_ASSERT(index >= 0 && index < DCDC_NUM);
        return dcdc_m[index];
    }


private:
    Ui::Widget *ui;
    QTimer *m_timer;
    QSqlTableModel *m_model;
    QSqlDatabase db;
    int curRecNo;
    dcdc dcdc_m[DCDC_NUM];

signals:
    void outputVoltageChanged(int index, float voltage);  // 发射这个信号
    void outputPowerChanged(int index, float power);

private slots:
    void on_startpushButton_clicked();
    void onOutputVoltageChanged(int index, float voltage);
    void timerstart();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
#endif // WIDGET_H
