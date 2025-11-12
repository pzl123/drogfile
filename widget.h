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
#include <QListView>
#include <QStandardItemModel>

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
    bool open_database(const QString& data_path, const QString& table_name, const QString& conn_name);
    void close_database();
    void drop_table_in_db(const QString& table_name);
    void create_table_in_db(QString create_str, const QString &table_name);
    void insert_msg_to_db(linestr_2_dbinfo_t& msg, const QString& table_name);
    void display_all_db_class();
    void set_output_voL(float32_t value);
    Ui::Widget* getui() { return ui; }
    QTimer* gettimer() { return m_timer; }
    QSqlTableModel* getmodel() { return m_model; }
    QSqlDatabase& getdatabase() { return m_db; }
    int *getcurRecNo(){ return &m_curRecNo; }
    dcdc* getdcdcArray() { return m_dcdc; }
    dcdc& getdcdcAt(int index) {
        Q_ASSERT(index >= 0 && index < DCDC_NUM);
        return m_dcdc[index];
    }


private:
    Ui::Widget *ui;
    QTimer *m_timer;
    QSqlTableModel *m_model;
    QStandardItemModel *m_dcdc_model;
    QSqlDatabase m_db;
    int m_curRecNo;
    dcdc m_dcdc[DCDC_NUM];

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
