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

MGraphics::MGraphics():thickness_pen(5), cursor_mode(0),drawingFlag(false),ColorObj(qRgb(0, 145, 218))
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
    switch (mode) {
    case 0:
        this->setCursor(Qt::ArrowCursor);
        break;
    case 1:
        this->setCursor(QCursor(draw_pix,0,draw_pix.height()));
        break;
    case 2:
        this->setCursor(QCursor(erase_pix,0,erase_pix.height()));
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

QImage threshold_img(const QImage& source_img, int threshold_value)
{
    int h = source_img.height();
    int w = source_img.width();
    QImage ret_img(w,h,source_img.format());

    for (int i = 0;i < w; ++i)
      for(int j = 0;j < h; ++j)
      {
         QRgb _P = source_img.pixel(i,j);
         if (qRed(_P) + qGreen(_P) + qBlue(_P) >= threshold_value * 3)
          {
              ret_img.setPixel(i,j,BIN_WHITE);
          }else{
              ret_img.setPixel(i,j,BIN_BLACK);
          }
      }
    return ret_img;
}

void MGraphics::Slider_Change(int value)//threshold
{// item in titem, threshold image in b_img
    if (pm.isNull()) return;

    Ctrl_Z.clear();
    Ctrl_Y.clear();

    txt->setText("threshold level is " + QString::number(value));

    b_img = threshold_img(pm.toImage(),value);
    titem = std::make_unique<QGraphicsPixmapItem> (QPixmap::fromImage(b_img));
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

inline bool MGraphics::dataIsReady()
{
   return !data_01.empty() && !data_obj.empty() && data_01.size() == pm.height();
}

void MGraphics::ShowObjectUnderCursor(QMouseEvent *event)
{
    QPoint p = transform(event->pos());
    int x = p.x();
    int y = p.y();
    if (dataIsReady() && cursor_mode != 2)
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
        }else QToolTip::showText(event->globalPos(),QString("(" + QString::number(x) +
                                                            "," + QString::number(y) + ")"));
    }
}

QPoint MGraphics::transform(QPoint pos)
{
    QPointF l = mapToScene(pos.x(),pos.y());
    int x = static_cast<int> (l.x());
    int y = static_cast<int> (l.y());
    return QPoint(x,y);
}

void MGraphics::mouseMoveEvent(QMouseEvent *event)
{
    QPoint p = transform(event->pos());
    if (!on_img(p.x(),p.y())){ return; }
    ShowObjectUnderCursor(event);
//----------------------------------------------------------------------------------------------------------------------
    if (drawingFlag && cursor_mode == 1)//draw
    {
       QLineF line(prevPoint.x(),prevPoint.y(),p.x(),p.y());
       line_items.push(scene.addLine(line,QPen(QBrush(ColorObj),
                       2*static_cast<qreal>(thickness_pen),Qt::SolidLine,Qt::RoundCap)));
       lines.push(line);
       prevPoint = p;
    }
    if (drawingFlag && cursor_mode == 2)//erase
    {
        QLineF line(prevPoint.x(),prevPoint.y(),p.x(),p.y());
        line_items.push(scene.addLine(line,
         QPen(QBrush(QColor(Qt::white)),2*static_cast<qreal>(thickness_pen),Qt::SolidLine,Qt::RoundCap)));
        lines.push(line);
        prevPoint = p;
    }
}

bool MGraphics::decide_to_draw(QPoint p)
{
    bool res;
    switch (cursor_mode) {
    case 1:
        res = data_01[p.y()][p.x()] > 0 ? true : false;
        break;
    case 2:
        res = true; //data_01[p.y()][p.x()] > 0 ? false : true;
        break;
    default:
        res = false;
        break;
    }
    return res;
}

void MGraphics::mousePressEvent(QMouseEvent *event)
{
    QPoint p = transform(event->pos());
    if (!on_img(p.x(),p.y()) || !dataIsReady()) { return; }
//----------------------------------------------------------------------------------------------------------------------
    lines.clear();
    line_items.clear();
    drawingFlag = decide_to_draw(p);

    if (drawingFlag)
    {
        ID_onDraw = data_01[p.y()][p.x()];
        StartPoint = prevPoint = p;
    }
}

QVector<QPoint> get_prime(int R)
{
   QVector<QPoint> res;
   for (int y = -R; y < R; ++y)
       for (int x = -R; x < R; ++x)
       {
           if (std::pow(x,2) + std::pow(y,2) <= std::pow(R,2))
           {
               res.push_back(QPoint(x,y));
           }
       }
   return res;
}

class SecureDrawing
{
    int w;
    int h;
    QPoint pt;
    uint clr;
    QImage& ref;
    bool Valid(QPoint point)
    {
        return point.x() >= 0 && point.y() >= 0 &&
               point.x() < w && point.y() < h;
    }
public:
    explicit SecureDrawing(QImage& image, QPoint draw, uint color):
       w(image.width()), h(image.height()),pt(draw),clr(color), ref(image){}
    SecureDrawing() = default;
    ~SecureDrawing()
    {
        if (Valid(pt))
          ref.setPixel(pt,clr);
    }
};

void drawLineOnQImage(QImage& img,QPointF p1,QPointF p2, const uint color, int thickness = 2)
{
    QVector2D n(p2 - p1);
    int len = static_cast<int> (n.length());
    n.normalize();
    QVector2D v(p1);
    auto prime = get_prime(thickness);
    while (len--)
    {
        v += n;
        for (const QPoint& p : prime)
        {
           SecureDrawing pixel
                   (img,p + v.toPoint(),color);
        }
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

QPoint getStartPoint(const QImage& img, QPoint CenterMass)
{
   return CenterMass;
}

QPoint MGraphics::drawCurve_andGetCenter(QImage& img)
{
    int N = 0;
    qreal avgX = 0.0;
    qreal avgY = 0.0;
    while (!line_items.empty())//size line_items == size lines
    {
        //reWrite bin image
        drawLineOnQImage(img,lines.top().p1(),lines.top().p2(),
                         cursor_mode == 1 ? BIN_BLACK : BIN_WHITE,thickness_pen);
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
   return QPoint(static_cast<int>(avgX),static_cast<int>(avgY));
}

void MGraphics::mouseReleaseEvent(QMouseEvent *event)
{
    drawingFlag = false;
    QPoint p = transform(event->pos());
    int x = p.x(); int y = p.y();

    EndPoint = p;
    if (!on_img(x,y) || !dataIsReady()) {
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
    }
//----------------------------------------------------------------------------------------------------------------------
    QImage red(b_img);
    Ctrl_Z.push(red);//push before drawing

    QPoint centerMass = drawCurve_andGetCenter(red);

    if (cursor_mode == 1){
    fill_area(red,getStartPoint(red,centerMass));
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

int wheel = 0;

void MGraphics::wheelEvent(QWheelEvent *event)
{
    if (pm.isNull() || scene.items().empty())
        return;
    QPoint numDegrees = event->angleDelta() / 8;
    if (numDegrees.y() > 0)
    {
        this->scale(1.05,1.05);
      //  QToolTip::showText(event->globalPos(),"Current thickness = " +
     //                       QString::number(++wheel) + QString("px"),this);
    }else
    {
        this->scale(1/1.05,1/1.05);
       // QToolTip::showText(event->globalPos(),"Current thickness = " +
        //                    QString::number(--wheel) + QString("px"),this);
    }
}

void MGraphics::setThickness(int number)
{
   thickness_pen = number;
}









