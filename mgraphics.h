#ifndef MGRAPHICS_H
#define MGRAPHICS_H

#include "single_area.h"

#include <memory>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QVector>
#include <QStack>
#include <QSlider>
#include <QLabel>
#include <QPoint>
#include <QString>
#include <QColor>

class MGraphics : public QGraphicsView
{
    Q_OBJECT
public:
   MGraphics();
   ~MGraphics();

   static QVector<QVector<std::size_t>> data_01;
   static QVector<S_area> data_obj;

   const QImage& get_bin_img()
   {
       return b_img;
   }

protected:
  void mouseMoveEvent(QMouseEvent *event)override;
  void mousePressEvent(QMouseEvent *event)override;
  void mouseReleaseEvent(QMouseEvent *event)override;
  void drawBackground(QPainter *painter, const QRectF &rect)override;
  void keyPressEvent(QKeyEvent *event)override;

public slots:

  void load_from_file(const QString&);
  void setCursor_mode(char);
  void ObjectsColorChange(QColor);
  void RandomColorize();

private slots:

  void Slider_Change(int value);
  void OSlider_Change(int value);

private:
  bool on_img(int,int);
  char cursor_mode; //0 - view(nothing), 1 - draw, 2 - erase
  bool drawingFlag;

  QColor ColorObj;

  std::unique_ptr<QLabel> txt;
  std::unique_ptr<QLabel> otxt;
  std::unique_ptr<QSlider> Slider;
  std::unique_ptr<QSlider> OSlider;
  QPixmap draw_pix;
  QPixmap erase_pix;

  QGraphicsScene scene;
  QPixmap pm;
  QImage b_img;
  QStack<QImage> Ctrl_Z, Ctrl_Y;

  std::unique_ptr<QGraphicsPixmapItem> sourceItem_from_image;
  std::unique_ptr<QGraphicsPixmapItem> titem;
  std::unique_ptr<QGraphicsPixmapItem> track_item;
  std::unique_ptr<QGraphicsPixmapItem> randItem;
};

#endif // MGRAPHICS_H
