#include "mgraphics.h"
#include "single_area.h"
#include <QDebug>
#include <QList>
#include <QToolTip>
#include <QApplication>
#include <QImage>
#include <QLineF>
#include <QPointF>
#include <QVector2D>
#include <cmath>
#include <random>

const int SLIDER_X_POS = 8;
const int SLIDER_Y_POS = 8;
const int SLIDER_WIDTH = 200;
const int SLIDER_HEIGHT = 20;
const uint BIN_BLACK = 0x0;
const uint BIN_WHITE = 0xFFFFFF;
const qreal EPS = 0.5;
const uint ARGB_A = 0xFF000000;
//const int NUM_OF_WIDGETS = 4;

class GenColor
{
    std::random_device rd;
    std::default_random_engine re;
    std::uniform_int_distribution<uint> dist;
    std::uniform_int_distribution<uint> a;
public:
    GenColor():re(rd()), dist(0x1,0xFFFFFF){}
    uint operator()()
    {
      // uint ARGB = 0xFF000000; //0xAARRGGBB;
       return ARGB_A + dist(re);
    }
};


MGraphics::~MGraphics()
{

}

MGraphics::MGraphics(): cursor_mode(0),drawingFlag(false),ColorObj(qRgb(0, 145, 218))
{
    this->setAlignment(Qt::AlignCenter);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setMinimumHeight(100);
    this->setMinimumWidth(100);
    this->setCursor(Qt::ArrowCursor);
    this->setMouseTracking(true);
    QPainter p;
    this->drawBackground(&p,this->rect());
    this->setScene(&scene);

    Slider = std::make_unique<QSlider> (Qt::Horizontal,this);
    OSlider = std::make_unique<QSlider> (Qt::Horizontal,this);
    txt = std::make_unique<QLabel>("threshold level",this);
    otxt = std::make_unique<QLabel> ("Opacity level",this);

    OSlider->setMaximum(100);
    OSlider->setMinimum(0);
    Slider->setMaximum(255);
    Slider->setMinimum(0);

    scene.addWidget(Slider.get());
    scene.addWidget(OSlider.get());
    scene.addWidget(txt.get());
    scene.addWidget(otxt.get());

    Slider->setGeometry(SLIDER_X_POS,SLIDER_Y_POS,SLIDER_WIDTH,SLIDER_HEIGHT);
    OSlider->setGeometry(SLIDER_X_POS,SLIDER_Y_POS + SLIDER_HEIGHT,SLIDER_WIDTH,SLIDER_HEIGHT);
    txt->setGeometry(SLIDER_X_POS + SLIDER_WIDTH + 3,SLIDER_Y_POS - 3,SLIDER_WIDTH - 50,SLIDER_HEIGHT);
    otxt->setGeometry(SLIDER_X_POS + SLIDER_WIDTH + 3,SLIDER_Y_POS - 3 + SLIDER_HEIGHT,SLIDER_WIDTH - 50,SLIDER_HEIGHT);
    txt->setStyleSheet("QLabel{color: rgba(0, 0, 0, 200); background-color : rgba(0, 0, 0, 50);}");
    otxt->setStyleSheet("QLabel{color: rgba(0, 0, 0, 200); background-color : rgba(0, 0, 0, 50);}");

    connect(Slider.get(),SIGNAL(valueChanged(int)),this,SLOT(Slider_Change(int)));
    connect(OSlider.get(),SIGNAL(valueChanged(int)),this,SLOT(OSlider_Change(int)));

    draw_pix.load(QCoreApplication::applicationDirPath() + "/res/draw_cur.png");
    erase_pix.load(QCoreApplication::applicationDirPath() + "/res/erase_cur.png");
}

void MGraphics::setCursor_mode(char mode)
{
    QCursor cur1(draw_pix,0,draw_pix.height());
    QCursor cur2(erase_pix,0,erase_pix.height());

    switch (mode) {
    case 0:
        this->setCursor(Qt::ArrowCursor);
        break;
    case 1:
        this->setCursor(cur1);
        break;
    case 2:
        this->setCursor(cur2);
        break;
    default:
        break;
    }
    cursor_mode = mode;
}

void MGraphics::load_from_file(const QString& path)//FINAL
{
    if (titem) scene.removeItem(titem.get());
    if (track_item)scene.removeItem(track_item.get());
    if (randItem) scene.removeItem(randItem.get());

    pm.load(path);
    sourceItem_from_image = std::make_unique<QGraphicsPixmapItem> (pm);
    scene.addItem(sourceItem_from_image.get());
    MGraphics::centerOn(sourceItem_from_image.get());
    MGraphics::fitInView(sourceItem_from_image.get(),Qt::KeepAspectRatio);
    update();
}

void MGraphics::OSlider_Change(int value)//FINAL
{
    if (titem && titem->scene() == &scene)
    {
        otxt->setText("Opacity level is " + QString::number(value));
        titem->setOpacity((qreal)value/100);
    }
}

void MGraphics::Slider_Change(int value)//threshold - FINAL
{// item in titem, threshold image in b_img
    if (pm.isNull()) return;

    Ctrl_Z.clear();
    Ctrl_Y.clear();

    txt->setText("threshold level is " + QString::number(value));

    QImage source_img = pm.toImage();

    const int h = source_img.height();
    const int w = source_img.width();

    for (int i = 0;i < w; ++i)
      for(int j = 0;j < h; ++j)
      {
         QRgb _P = source_img.pixel(i,j);
         if (qRed(_P) + qGreen(_P) + qBlue(_P) >= value * 3)
          {
              source_img.setPixel(i,j,BIN_WHITE);
          }else{
              source_img.setPixel(i,j,BIN_BLACK);
          }
      }
    b_img = source_img;
    titem = std::make_unique<QGraphicsPixmapItem> (QPixmap::fromImage(std::move(source_img)));
    scene.addItem(titem.get());
    update();
}

inline bool MGraphics::on_img(int x,int y)//return true if (x,y) is pixmap point
{
    bool res = (x < 0)||(y < 0)||(x + 1 > (int) pm.width())||(y + 1 > (int) pm.height());
    return !res;
}

void MGraphics::ObjectsColorChange(QColor col)
{
    ColorObj = col;
}

size_t ID_onDraw;
QPoint prevPoint;
QPoint StartPoint;
QPoint EndPoint;
QStack<QLineF> lines;
QStack<QGraphicsLineItem*> line_items;

void MGraphics::mouseMoveEvent(QMouseEvent *event)
{//test
    QPoint p = event->pos();
    QPointF l = mapToScene(p.x(),p.y()); // coordinate transformation
    int x = static_cast<int> (l.x());
    int y = static_cast<int> (l.y());
    p.setX(x); p.setY(y);
    if (!on_img(x,y)){ return; }
//highlight an object----------------------------------------------------------------------------------------------------
    QToolTip::showText(event->globalPos(),QString("(" + QString::number((int) l.x()) +
                                                  "," + QString::number((int) l.y()) + ")"));
    if (!(data_01.empty()) && (data_01.size() == pm.height()) && cursor_mode != 2)
    {
        size_t id = data_01[y][x];
        if (id){
            QImage track(pm.width(),pm.height(),QImage::Format_ARGB32);
            track.fill(qRgba(0, 0, 0, 0));
            for (const auto& p : data_obj[id - 1].Points) {
               track.setPixel(p,ColorObj.rgb());
             }
            track_item = std::make_unique<QGraphicsPixmapItem> (QPixmap::fromImage(std::move(track)));
            scene.addItem(track_item.get());
            update();
            QToolTip::showText(event->globalPos(),QString ("id = " + QString::number(data_01[y][x])));
        }
    }  
//----------------------------------------------------------------------------------------------------------------------
    if (drawingFlag && cursor_mode == 1)//draw
    {
       QLineF line(prevPoint.x(),prevPoint.y(),p.x(),p.y());
       line_items.push(scene.addLine(line,QPen(QBrush(ColorObj),2,Qt::SolidLine,Qt::RoundCap)));
       lines.push(line);
       prevPoint = p;
    }
    if (drawingFlag && cursor_mode == 2)//erase
    {
        QLineF line(prevPoint.x(),prevPoint.y(),p.x(),p.y());
        line_items.push(scene.addLine(line,QPen(QBrush(QColor(Qt::white)),2,Qt::SolidLine,Qt::RoundCap)));
        lines.push(line);
        prevPoint = p;
    }
}

void MGraphics::mousePressEvent(QMouseEvent *event)
{//test
    QPoint p = event->pos();
    QPointF l = mapToScene(p.x(),p.y()); // coordinate transformation
    int x = static_cast<int> (l.x());
    int y = static_cast<int> (l.y());
    p.setX(x); p.setY(y);
    if (!on_img(x,y) || data_01.empty() || data_01.size() != pm.height()) { return; }
//----------------------------------------------------------------------------------------------------------------------
    lines.clear();
    line_items.clear();

    switch (cursor_mode) {
    case 1:
        drawingFlag = data_01[y][x] > 0 ? true : false;
        break;
    case 2:
        drawingFlag = data_01[y][x] > 0 ? false : true;
        break;
    default:
        drawingFlag = false;
        break;
    }
    ID_onDraw = data_01[y][x];
    StartPoint = QPoint(x,y);

    if (drawingFlag)
    {
    //scene.addEllipse((qreal) p.x(),(qreal) p.y(),3,3,QPen(Qt::NoPen),QBrush(ColorObj));
    prevPoint = p;
    }
}

void drawLineOnQImage(QImage& img,QPointF p1,QPointF p2, const uint color)
{
    QVector2D n(p2 - p1);
   // int len = (int) n.length();
    int len = static_cast<int> (n.length());
    n.normalize();
    QVector2D v1(p1);
    QVector2D v2(p2);

    while (len--)
    {
        v1 += n;
        int x = v1.toPoint().x();
        int y = v1.toPoint().y();
        img.setPixel(x,y,color);
        img.setPixel(x+1,y, color);
        img.setPixel(x-1,y, color);
        img.setPixel(x,y+1, color);
        img.setPixel(x,y-1, color);
    }
}

const QRgb black = qRgb(0,0,0);

inline bool isBlack(int x, int y, const QImage& im)
{
    return (im.pixel(x,y) == black);
}

void fill_area(QImage& img, QPoint Start_point)
{
  QStack<QPoint> depth;
  depth.push(Start_point);
  const int w = img.width();
  const int h = img.height();

  while (!depth.empty())
  {
    QPoint t = depth.top();
    depth.pop();
    int x = t.x();
    int y = t.y();
    img.setPixel(t,BIN_BLACK);

    if((x + 1 < w)&&(!isBlack(x+1,y,img)))
    {
        depth.push(QPoint(x+1,y));
    }
    if((x - 1> -1)&&(!isBlack(x-1,y,img)))
    {
        depth.push(QPoint(x-1,y));
    }
    if((y + 1< h)&&(!isBlack(x,y+1,img)))
    {
        depth.push(QPoint(x,y+1));
    }
    if((y - 1> -1)&&(!isBlack(x,y-1,img)))
    {
        depth.push(QPoint(x,y-1));
    }
  }
}

void fill_erase_area(QImage& img, QPoint begin, QPoint end, QPoint Start_point)
{
    drawLineOnQImage(img,begin,end,1);
    QStack<QPoint> depth;
    depth.push(Start_point);
    const int w = img.width();
    const int h = img.height();

    while (!depth.empty())
    {
      QPoint t = depth.top();
      depth.pop();
      int x = t.x();
      int y = t.y();
      img.setPixel(t,BIN_WHITE);

      if((x + 1 < w)&&(!isBlack(x+1,y,img)))
      {
          depth.push(QPoint(x+1,y));
      }
      if((x - 1> -1)&&(!isBlack(x-1,y,img)))
      {
          depth.push(QPoint(x-1,y));
      }
      if((y + 1< h)&&(!isBlack(x,y+1,img)))
      {
          depth.push(QPoint(x,y+1));
      }
      if((y - 1> -1)&&(!isBlack(x,y-1,img)))
      {
          depth.push(QPoint(x,y-1));
      }
    }
}

QPoint getStartPoint(const QImage& img, QPoint p)
{
   return p;
}

void MGraphics::mouseReleaseEvent(QMouseEvent *event)
{//test
    drawingFlag = false;
    QPoint p = event->pos();
    QPointF l = mapToScene(p.x(),p.y()); // coordinate transformation
    int x = static_cast<int> (l.x());
    int y = static_cast<int> (l.y());
    p.setX(x); p.setY(y);
    EndPoint = p;
    if (!on_img(x,y) || data_01.empty() || data_01.size() != pm.height()) {
        while (!line_items.empty())
        {
          scene.removeItem(line_items.top());
          line_items.pop();
        }

        return;
    }
    if (cursor_mode == 1 && ID_onDraw != data_01[y][x])
    {
        while (!line_items.empty())
        {
          scene.removeItem(line_items.top());
          line_items.pop();
        }

        return;
    }else if(cursor_mode == 2)
    {

    }
//----------------------------------------------------------------------------------------------------------------------
    QImage red(b_img);
    Ctrl_Z.push(red);//push before drawing
    int N = 0;
    qreal avgX = 0.0;
    qreal avgY = 0.0;
    while (!line_items.empty())//size line_items == size lines
    {
        //reWrite bin image
        drawLineOnQImage(red,lines.top().p1(),lines.top().p2(),
                         cursor_mode == 1 ? BIN_BLACK : BIN_WHITE);
        avgX += lines.top().p1().x();
        avgY += lines.top().p1().y();
        //remove temp lines from scene
       scene.removeItem(line_items.top());
       lines.pop(); line_items.pop();  
       ++N;
    }
    N = N > 0 ? N : 1;
    avgX /= N;
    avgY /= N;
    QPoint m(static_cast<int>(avgX),static_cast<int>(avgY));

    if (cursor_mode == 1){
    fill_area(red,getStartPoint(red,m));
    }

    b_img = red;
    titem = std::make_unique<QGraphicsPixmapItem> (QPixmap::fromImage(std::move(red)));
    scene.addItem(titem.get());
    update();
}

void MGraphics::drawBackground(QPainter *painter, const QRectF &rect)
{
    //119'136'153	#778899	Светлый синевато-серый	Lightslategray
    //230'230'250	#e6e6fa	Бледно-лиловый	Lavender
    //211'211'211	#d3d3d3	Светло-серый	Lightgray
   QColor col(211,211,211);
   painter->fillRect(rect,col);
}

void MGraphics::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Z &&
             e->modifiers() & Qt::ControlModifier && !Ctrl_Z.empty())
    {
      Ctrl_Y.push(b_img);
      b_img = Ctrl_Z.pop();
      titem = std::make_unique<QGraphicsPixmapItem> (QPixmap::fromImage(b_img));
      scene.addItem(titem.get());
      update();
     // qDebug()<<"Z = " << Ctrl_Z.size() << "Y = " << Ctrl_Y.size();
    }
    if (e->key() == Qt::Key_Y &&
            e->modifiers() & Qt::ControlModifier && !Ctrl_Y.empty())
    {      
      Ctrl_Z.push(Ctrl_Y.top());
      b_img = Ctrl_Y.pop();
      titem = std::make_unique<QGraphicsPixmapItem> (QPixmap::fromImage(b_img));
      scene.addItem(titem.get());
      update();
     // qDebug()<<"Z = " << Ctrl_Z.size() << "Y = " << Ctrl_Y.size();
    }

}

void MGraphics::RandomColorize()
{
    QImage mask(pm.width(),pm.height(),QImage::Format_ARGB32);
    mask.fill(qRgba(0, 0, 0, 0));
    GenColor gen;

    for (const S_area& obj : data_obj)
    {
        uint objColor = gen();
        for (const QPoint& p : obj.Points)
        {
            mask.setPixel(p,objColor);
        }
        objColor = BIN_BLACK;
        for (const QPoint& p : obj.CPoints)
        {
            mask.setPixel(p,objColor);
        }
    }
    randItem = std::make_unique<QGraphicsPixmapItem>
            (QPixmap::fromImage(std::move(mask)));
    scene.addItem(randItem.get());
    update();
}









