#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mgraphics.h"
#include "worker.h"

#include <QDebug>
#include <QThread>
#include <QString>
#include <QFileDialog>
#include <QColorDialog>
#include <QColor>
#include <QAction>
#include <QMenu>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    View = new MGraphics;
    ui->gridLayout->addWidget(View);
    connect(this,&MainWindow::send_filePath,
            View,&MGraphics::load_from_file);
    connect(this,&MainWindow::send_Cursor_mode,
            View,&MGraphics::setCursor_mode);
    connect(this,&MainWindow::send_Obj_color,
            View,&MGraphics::ObjectsColorChange);
    connect(this,&MainWindow::send_colorize_obj,
            View,&MGraphics::RandomColorize);
}

MainWindow::~MainWindow()
{
    delete View;
    delete ui;
}

void MainWindow::on_actionLoad_file_triggered()//main load
{
    QString file_name = QFileDialog::getOpenFileName(this, "Open Dialog", "", "*.jpg *.jpeg *.bmp *.png");
    qDebug()<<file_name;
   // QString file_name("C:/Users/relaxes/Documents/46_KAF/primery_izobrazheniy_dlya_UIR/костный мозг  F0000055.bmp");
    emit send_filePath(file_name);
}

void MainWindow::on_actionStart_algo_triggered()
{
     if (View->get_bin_img().isNull()) return;

     QThread *thread = new QThread();
     Worker    *task = new Worker(View->get_bin_img());
//--------------------------------------------------------------------------------------------------------
     task -> moveToThread(thread);
//--------------------------------------------------------------------------------------------------------
     View->setEnabled(false);
     connect(task,SIGNAL(EnableView(bool)),
             View,SLOT(setEnabled(bool)));
     connect(thread,SIGNAL(started()),
             task, SLOT(doWork()),Qt::DirectConnection);
     connect(task,SIGNAL(workFinished()),
             thread,SLOT(quit()),Qt::DirectConnection);
     //automatically delete thread and task object when work is done:
     connect(thread,SIGNAL(finished()),
             task,SLOT(deleteLater()),Qt::DirectConnection);
     connect(thread,SIGNAL(finished()),
             thread,SLOT(deleteLater()),Qt::DirectConnection);
//--------------------------------------------------------------------------------------------------------
     thread->start();
}

void MainWindow::on_action1_triggered()//zoom_in
{
  View->scale(1.1,1.1);
}

void MainWindow::on_action2_triggered()//zoom out
{
  View->scale(1/(1.1),1/(1.1));
}

void MainWindow::on_action3_triggered()//DRAW MODE
{
    bool isActive = ui->action3->isChecked();
    ui->action3->setChecked(isActive);

    if (ui->action4->isChecked() && isActive)
        ui->action4->setChecked(false);

    char mode = isActive ? 1 : 0;
    emit send_Cursor_mode(mode);
}

void MainWindow::on_action4_triggered()
{
    bool isActive = ui->action4->isChecked();
    ui->action4->setChecked(isActive);
    if (ui->action3->isChecked() && isActive)
        ui->action3->setChecked(false);

    char mode = isActive ? 2 : 0;
    emit send_Cursor_mode(mode);
}

void MainWindow::on_actionColor_of_objects_triggered()
{
    QColor col = QColorDialog::getColor(Qt::white,this,"Choose the objects color");
    emit send_Obj_color(col);
}

void MainWindow::on_actionColorize_all_triggered()
{
    emit send_colorize_obj();
}
