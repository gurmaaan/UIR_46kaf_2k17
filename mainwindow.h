#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mgraphics.h"
#include <QMainWindow>
#include <QImage>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QSlider>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
signals:

    void send_filePath(const QString&);

    void send_Cursor_mode(char);

    void send_Obj_color(QColor);

    void send_colorize_obj();

    void send_thickness(int);

private slots:
    void on_actionLoad_file_triggered();

    void on_actionStart_algo_triggered();

    void on_action1_triggered();

    void on_action2_triggered();

    void on_action3_triggered();

    void on_action4_triggered();

    void on_actionColor_of_objects_triggered();

    void on_actionColorize_all_triggered();

    void on_lineEdit_selectionChanged();

    void on_lineEdit_editingFinished();

    void on_horizontalSlider_valueChanged(int value);

private:
    Ui::MainWindow *ui;
    MGraphics *View;
};

#endif // MAINWINDOW_H
