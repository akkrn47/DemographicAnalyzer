#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include "appcontext.h"
#include "entrypoint.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onFileClicked();
    void onUploadClicked();
    void onMetricsClicked();
    void onExitClicked();
    void filterTable(QString textInRegionField);

private:
    Ui::MainWindow *ui;
    AppContext context;
    int setupTableHeaders(const AppContext& context);
    void setupTableCells(const AppContext& context);
    void setupRegionCellWidth(const AppContext& context);
};

#endif // MAINWINDOW_H
