#ifndef SHAPESTATISTICS_H
#define SHAPESTATISTICS_H

#include <QString>

// shape statistics class, link to ITK
template<typename T>    // T is the class of the shape obj
class ShapeStatistics
{
private:
    const T* shlPtr;

public:
    ShapeStatistics() { }
    ShapeStatistics( const T* ptr )
    {
        shlPtr = ptr;
    }

    QString toString()
    {
        QString s = "<table border=\"1\">";

#define ADD_ROW(x,y)    \
    s += QString("<tr> <td>") + x + QString("</td><td>") + y + QString("</td>")

        ADD_ROW( "Phys size", QString("%1").arg( shlPtr->GetPhysicalSize() ) );


        ADD_ROW( "Feret Diameter", QString("%1").arg( shlPtr->GetFeretDiameter() ) );
        ADD_ROW( "Perimeter", QString("%1").arg( shlPtr->GetPerimeter() ) );
        ADD_ROW( "Roundness", QString("%1").arg( shlPtr->GetRoundness() ) );
        ADD_ROW( "Flatness", QString("%1").arg( shlPtr->GetBinaryFlatness() ) );

        typename T::VectorType moments = shlPtr->GetBinaryPrincipalMoments();
        typename T::MatrixType pAxes = shlPtr->GetBinaryPrincipalAxes();

        ADD_ROW( "Principal Moments", QString("(%1, %2, %3)").arg(moments[0]).arg(moments[1]).arg(moments[2]) );


        ADD_ROW( "Principal Vector 1", QString("(%1, %2, %3)").arg(pAxes[0][0]).arg(pAxes[0][1]).arg(pAxes[0][2]) );
        ADD_ROW( "Principal Vector 2", QString("(%1, %2, %3)").arg(pAxes[1][0]).arg(pAxes[1][1]).arg(pAxes[1][2]) );
        ADD_ROW( "Principal Vector 3", QString("(%1, %2, %3)").arg(pAxes[2][0]).arg(pAxes[2][1]).arg(pAxes[2][2]) );


#undef ADD_ROW

        s += "</table>";

        return s;
    }
};

#endif // SHAPESTATISTICS_H
