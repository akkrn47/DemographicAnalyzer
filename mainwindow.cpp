#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->chooseFilePushButton, &QPushButton::clicked, this, &MainWindow::onFileClicked);
    connect(ui->uploadPushButton, &QPushButton::clicked, this, &MainWindow::onUploadClicked);
    connect(ui->metricsPushButton, &QPushButton::clicked, this, &MainWindow::onMetricsClicked);
    connect(ui->exitButton, &QPushButton::clicked, this, &MainWindow::onExitClicked);
    connect(ui->regionField, &QLineEdit::textEdited, this, &MainWindow::filterTable);
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->horizontalHeader()->setMinimumSectionSize(80);
    ui->columnNumber->setDisabled(true);
    ui->regionField->setDisabled(true);

}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onFileClicked() {
    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    ui->pathLabel->clear();
    QString path = QFileDialog::getOpenFileName(this, "Выбрать файл", "D:/", "Файлы csv (*.csv);");
    if (!path.isEmpty()) {
        ui->pathLabel->setText(path);
    }
}

void MainWindow::onUploadClicked() {
    if (ui->pathLabel->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Сначала выберите файл");
        return;
    }
    doOperation(Init, &context);
    strcpy(context.currentFilePath, ui->pathLabel->text().toStdString().c_str());
    doOperation(LoadData, &context);

    if (!context.fileLoaded) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные из файла");
        return;
    }

    ui->columnNumber->clear();
    ui->tableWidget->setRowCount(context.tableData.validRowsCount);
    ui->tableWidget->setColumnCount(context.tableData.cols);

    int checkHeaders = 0;
    checkHeaders = setupTableHeaders(context);
    setupTableCells(context);
    setupRegionCellWidth(context);
    filterTable(ui->regionField->text().trimmed());

    QString stats = QString(
                        "Статистика загрузки:\n"
                        "▸ Всего строк в файле: %1\n"
                        "▸ Корректных строк: %2\n"
                        "▸ Некорректных строк: %3")
                        .arg(context.tableData.rows + context.tableData.invalidRows + checkHeaders)
                        .arg(context.tableData.rows)
                        .arg(context.tableData.invalidRows);

    QMessageBox::information(this, "Результаты загрузки", stats);
    ui->columnNumber->setDisabled(false);
    ui->regionField->setDisabled(false);

}

void MainWindow::filterTable(QString textInRegionField)
{
    if (!context.fileLoaded) return;
    QString selectedRegion = textInRegionField.trimmed();
    bool showAllRegions = (selectedRegion.isEmpty());

    for (int row = 0; row < context.tableData.validRowsCount; ++row) {
        QString rowRegion = QString(context.tableData.validRows[row].region).trimmed();
        bool regionMatches = showAllRegions || (rowRegion == selectedRegion);
        ui->tableWidget->setRowHidden(row, !(regionMatches));
    }
}


void MainWindow::onMetricsClicked() {
    if (!context.fileLoaded) {
        QMessageBox::warning(this, "Ошибка", "Сначала загрузите файл и данные из него");
        return;
    }

    QString region = ui->regionField->text().trimmed();
    if (region.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название региона");
        return;
    }

    bool textTransformedToInt;
    int column = ui->columnNumber->text().toInt(&textTransformedToInt);
    if (!textTransformedToInt || column <= 0 || column > context.tableData.cols || column == (COL_REGION + 1)) {
        QMessageBox::warning(this, "Ошибка", "Некорректный номер колонки \nЕсли поле пустое - введите значение");
        return;
    }
    context.filtredParams.selectedColumn = column;
    context.filtredParams.regionFilter = region.trimmed().toUtf8().constData();

    doOperation(FilterData, &context);

    if (!context.dataFiltered || context.metrics.count == 0) {
        QMessageBox::warning(this, "Ошибка", "Нет данных для выбранного региона");
        return;
    }

    QString metricsText = QString(
                              "Статистика для региона %1 (колонка %2):\n"
                              "▸ Минимум: %3\n"
                              "▸ Максимум: %4\n"
                              "▸ Медиана: %5\n")
                              .arg(region)
                              .arg(column)
                              .arg(context.metrics.min, 0, 'f', 2)
                              .arg(context.metrics.max, 0, 'f', 2)
                              .arg(context.metrics.median, 0, 'f', 2);

    QMessageBox::information(this, "Результаты анализа", metricsText);
}



void MainWindow::onExitClicked() {
    doOperation(Clear, &context);
    qApp->quit();
}


int MainWindow::setupTableHeaders(const AppContext &context){
    QStringList headers;
    for (int i = 0; i < context.tableData.cols; i++) {
        headers << QString(context.tableData.headers[i]);
    }
    int chekHeaders = 0;
    if(!headers.isEmpty())
        chekHeaders = 1;
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    return chekHeaders;
}

void MainWindow::setupTableCells(const AppContext &context){
    for (int row = 0; row < context.tableData.validRowsCount; row++) {
        for (int col = 0; col < context.tableData.cols; col++) {
            QTableWidgetItem* item = new QTableWidgetItem(
                QString(context.tableData.matrix[row][col]));
            ui->tableWidget->setItem(row, col, item);
        }
    }
}

void MainWindow::setupRegionCellWidth(const AppContext &context){
    for(int i = 0; i < ui->tableWidget->columnCount(); i++){
        ui->tableWidget->setColumnWidth(i, 90);
        if(i == REGION_COLUMN) {
            ui->tableWidget->setColumnWidth(i, 260);
        }
    }

}
