#ifndef SINGLE_AREA_H
#define SINGLE_AREA_H

#include <QVector>
#include <QPoint>
#include <iostream>
#include <QBitmap>
struct S_area
{
    std::size_t id;
    QVector<QPoint> Points;
    QVector<QPoint> CPoints;
    S_area() = default;

    explicit S_area(std::size_t number): id(number), Points(0), CPoints(0){}

    S_area& operator=(const S_area& obj)
    {
        id = obj.id;
        std::copy(obj.Points.begin(),obj.Points.end(),Points.begin());
        std::copy(obj.CPoints.begin(),obj.CPoints.end(),CPoints.begin());
        return *this;
    }

    void print()const
    {
        std::cout<<"id : "<<id<<" ";
        for(const QPoint& p : Points)
        std::cout<<"("<<p.x()<<","<<p.y()<<") ";
        std::cout<<" cont : ";
        for(const QPoint& p : CPoints)
        std::cout<<"("<<p.x()<<","<<p.y()<<") ";
        std::cout<<std::endl;
    }
    void add_main(QPoint p)
    {
        Points.push_back(p);
    }

    void add_cont(QPoint p)
    {
        CPoints.push_back(p);
    }
};

#endif // SINGLE_AREA_H
