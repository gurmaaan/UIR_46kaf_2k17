#include "worker.h"
#include "single_area.h"
#include "mgraphics.h"

#include <QVector>
#include <QImage>
#include <QStack>
#include <QPoint>
#include <QDebug>

QVector<QVector<size_t>> MGraphics::data_01;
QVector<S_area> MGraphics::data_obj;

inline bool inner(size_t x, size_t y, QVector<QVector<size_t>>& V)//return true if d is inner point
{
    bool res = (V[y][x + 1])&(V[y][x - 1])&(V[y + 1][x])&(V[y - 1][x]);
    return res;
}

const QRgb black = qRgb(0,0,0);

inline bool isBlack(int x, int y, const QImage& im)
{
    return im.pixel(x,y) == black;
}

void fill(const QImage& img, QVector<QVector<size_t>>& V, int _x, int _y, int L)//FINAL
{
  QPoint t;
  QStack<QPoint> depth;
  depth.push(QPoint(_x,_y));
  const int w = img.width();
  const int h = img.height();

  while (!depth.empty())
  {
    t = depth.top();
    depth.pop();
    int x = t.rx();
    int y = t.ry();
    V[y][x] = L; // filling.

    if((x + 1 < w)&&(isBlack(x+1,y,img))&&(V[y][x + 1] == 0))
    {
        depth.push(QPoint(x+1,y));
    }
    if((x - 1> -1)&&(isBlack(x-1,y,img))&&(V[y][x - 1] == 0))
    {
        depth.push(QPoint(x-1,y));
    }
    if((y + 1< h)&&(isBlack(x,y+1,img))&&(V[y + 1][x] == 0))
    {
        depth.push(QPoint(x,y+1));
    }
    if((y - 1> -1)&&(isBlack(x,y-1,img))&&(V[y - 1][x] == 0))
    {
        depth.push(QPoint(x,y-1));
    }
  }
}

void Worker::doWork()//heavy function
{
    const size_t _h = bin.height();
    const size_t _w = bin.width();
    size_t L = 1; // starting id value

    QVector<QVector<size_t>> Labels(_h, QVector<size_t>(_w,0));

//labeling__________________________________________________________________________
    for(size_t y = 0; y < _h; ++y)
      for(size_t x = 0; x < _w; ++x)
      {
       if((Labels[y][x] == 0)&&(isBlack(x,y,bin)))
       {
         fill(bin,Labels,x,y,L++);//very fast!
       }
      }

//_________________________________________________________________________________
    const size_t size = --L; // size == num of objects
    QVector<S_area> V(size);

    for(size_t i = 0;i < size; ++i)
     {
       V[i] = S_area{i};
     }
//-----------------------------------------------------------------------------

    if ((size > 0)&&(_h > 2)&&(_w > 2))
    {
    for(size_t y = 1; y < _h - 1; ++y)//general case
      for(size_t x = 1; x < _w - 1; ++x)//general case
      {
          size_t id = Labels[y][x];
          if ((id > 0)&&(id < size + 1))
          {
            QPoint t(x,y);
            V[id - 1].add_main(t);

            if (!inner(x,y,Labels))
            {
                V[id - 1].add_cont(t);
            }
          }
      }
    for(size_t x = 1; x < _w - 1; ++x)//top case
     {
        size_t id = Labels[0][x];
        if ((id > 0)&&(id < size + 1))
        {
           QPoint t(x,0);
           V[id - 1].add_main(t);
           V[id - 1].add_cont(t);//
        }
     }
    for(size_t x = 1; x < _w - 1; ++x)//bottom case
     {
        size_t id = Labels[_h - 1][x];
        if ((id > 0)&&(id < size + 1))
                {
                   QPoint t(x,_h - 1);
                   V[id - 1].add_main(t);
                   V[id - 1].add_cont(t);
                }
     }
    for(size_t y = 0; y < _h; ++y)//left case
     {
         size_t id = Labels[y][0];
         if ((id > 0)&&(id < size + 1))
         {
            QPoint t(0,y);
            V[id - 1].add_main(t);
            V[id - 1].add_cont(t);
         }
     }
    for(size_t y = 0; y < _h; ++y)//right case
     {
         size_t id = Labels[y][_w - 1];
         if ((id > 0)&&(id < size + 1))
         {
            QPoint t(_w - 1,y);
            V[id - 1].add_main(t);
            V[id - 1].add_cont(t);
         }
     }
   }

    MGraphics::data_01 = std::move(Labels);
    MGraphics::data_obj = std::move(V);

    emit EnableView(true);
    emit workFinished();
}
