//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/Abc/Tests/Assert.h>

using namespace Alembic::AbcGeom;

//-*****************************************************************************
void xformOut()
{
    OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(), "Xform1.abc" );
    OXform a( OObject( archive, kTop ), "a" );
    OXform b( a, "b" );
    OXform c( b, "c" );
    OXform d( c, "d" );

    XformOpVec aVec;
    XformOp op(kTranslateOperation, kTranslateHint);

    std::vector <double> valVec;
    valVec.push_back(12.0);  // translate x
    valVec.push_back(20.0);  // translate z
    Abc::DoubleArraySample data(valVec);

    op.setYAnimated(true);
    aVec.push_back(op);
    a.getSchema().setXform( aVec, data);

    for (size_t i = 0; i < 20; ++i)
    {
        valVec.clear();
        valVec.push_back(i+42.0);
        data = Abc::DoubleArraySample(valVec);
        a.getSchema().set( data, OSampleSelector( i ) );
    }

    for (size_t i = 0; i < 20; ++i)
    {
        b.getSchema().setInherits( i&1, OSampleSelector( i ) );
    }

    // for c we write nothing

    XformOpVec dVec;
    op = XformOp(kScaleOperation, kScaleHint);
    dVec.push_back(op);

    valVec.clear();
    valVec.push_back(3.0);
    valVec.push_back(6.0);
    valVec.push_back(9.0);
    data = Abc::DoubleArraySample(valVec);

    d.getSchema().setXform( dVec, data );
}

//-*****************************************************************************
void xformIn()
{
    IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(),
                      "Xform1.abc" );
    IXform a( IObject( archive, kTop ), "a" );
    XformOpVec ops = a.getSchema().getOps();
    TESTING_ASSERT( ops.size() == a.getSchema().getNumOps() );
    TESTING_ASSERT( ops.size() == 1 );
    TESTING_ASSERT( a.getSchema().isOpStatic(0) == false );

    TESTING_ASSERT( a.getSchema().getNumAnimSamples() == 20 );
    TESTING_ASSERT( a.getSchema().inherits() );
    for ( index_t i = 0; i < 20; ++i )
    {
        XformSample xs;
        a.getSchema().getSample(xs, Abc::ISampleSelector(i));
        TESTING_ASSERT(xs.getNum() == 1);
        TESTING_ASSERT( xs.get(0)->getType() == kTranslateOperation );
        TranslateData t( xs.get(0) );
        TESTING_ASSERT( t.get() == V3d(12.0, i+42.0, 20.0) );
        TESTING_ASSERT( t.getMatrix() ==
            Abc::M44d().setTranslation( V3d(12.0, i+42.0, 20.0)) );
        Abc::M44d mat = a.getSchema().getMatrix(Abc::ISampleSelector(i));
        TESTING_ASSERT( mat ==
            Abc::M44d().setTranslation( V3d(12.0, i+42.0, 20.0)) );
    }

    Abc::M44d identity;

    XformSample xs;

    IXform b( a, "b" );
    b.getSchema().getSample(xs);
    TESTING_ASSERT( xs.getNum() == 0 );
    TESTING_ASSERT( b.getSchema().getOps().size() == 0 );
    TESTING_ASSERT( b.getSchema().getNumOps() == 0 );
    TESTING_ASSERT( b.getSchema().getMatrix() == identity );

    IXform c( b, "c" );
    c.getSchema().getSample(xs);
    TESTING_ASSERT( xs.getNum() == 0 );
    TESTING_ASSERT( c.getSchema().getOps().size() == 0 );
    TESTING_ASSERT( c.getSchema().getNumOps() == 0 );
    TESTING_ASSERT( c.getSchema().getMatrix() == identity );
    TESTING_ASSERT( c.getSchema().inherits() );

    IXform d( c, "d" );
    d.getSchema().getSample(xs);
    TESTING_ASSERT( xs.getNum() == 1 );
    TESTING_ASSERT( d.getSchema().getNumOps() == 1 );
    TESTING_ASSERT( xs.get(0)->getType() == kScaleOperation );
    TESTING_ASSERT( d.getSchema().isOpStatic(0) );
    ScaleData s( xs.get(0) );
    TESTING_ASSERT( s.get() == V3d(3.0, 6.0, 9.0) );
    TESTING_ASSERT( s.getMatrix() ==
        Abc::M44d().setScale( V3d(3.0, 6.0, 9.0)) );
    TESTING_ASSERT( d.getSchema().getOps().size() == 1 );
    TESTING_ASSERT( d.getSchema().inherits() );
    Abc::M44d mat = d.getSchema().getMatrix();
    TESTING_ASSERT( mat ==
        Abc::M44d().setScale( V3d(3.0, 6.0, 9.0)) );
}
//-*****************************************************************************
void someOpsXform()
{
    std::string name = "Xform2.abc";
    {
        OArchive archive( Alembic::AbcCoreHDF5::WriteArchive(), name );
        OXform a( OObject( archive, kTop ), "a" );

        XformOpVec aVec;

        std::vector <double> staticVec;
        std::vector <double> animVec;

        // scale with animated x
        XformOp op(kScaleOperation, kScaleHint);
        op.setXAnimated(true);
        aVec.push_back(op);
        animVec.push_back(2.0);   // x
        staticVec.push_back(1.0); // y
        staticVec.push_back(2.0); // z

        // Maya like shear xy, xz, yz will all be animated
        op = XformOp(kMatrixOperation, kMayaShearHint);
        op.setIndexAnimated(4, true);
        op.setIndexAnimated(8, true);
        op.setIndexAnimated(9, true);
        aVec.push_back(op);
        staticVec.push_back(1.0); // [0][0]
        staticVec.push_back(0.0); // [0][1]
        staticVec.push_back(0.0); // [0][2]
        staticVec.push_back(0.0); // [0][3]
        animVec.push_back(0.0);   // [1][0] xy
        staticVec.push_back(1.0); // [1][1]
        staticVec.push_back(0.0); // [1][2]
        staticVec.push_back(0.0); // [1][3]
        animVec.push_back(0.0);   // [2][0] xz
        animVec.push_back(0.0);   // [2][1] yz
        staticVec.push_back(1.0); // [2][2]
        staticVec.push_back(0.0); // [2][3]
        staticVec.push_back(0.0); // [3][0]
        staticVec.push_back(0.0); // [3][1]
        staticVec.push_back(0.0); // [3][2]
        staticVec.push_back(1.0); // [3][3]

        // rotate x axis, static
        op = XformOp(kRotateOperation, kRotateHint);
        aVec.push_back(op);
        staticVec.push_back(1.0);  // x
        staticVec.push_back(0.0);  // y
        staticVec.push_back(0.0);  // z
        staticVec.push_back(1.57); // angle

        // rotate y axis, angle will be animated
        op.setAngleAnimated(true);
        aVec.push_back(op);
        staticVec.push_back(0.0); // x
        staticVec.push_back(1.0); // y
        staticVec.push_back(0.0); // z
        animVec.push_back(0.125); // angle

        // rotate z axis, angle will be animated, use a different hint for fun
        op = XformOp(kRotateOperation, kRotateOrientationHint);
        op.setAngleAnimated(true);
        aVec.push_back(op);
        staticVec.push_back(0.0); // x
        staticVec.push_back(0.0); // y
        staticVec.push_back(1.0); // z
        animVec.push_back(0.1); // angle

        // translate with animated y and z, different hint for fun
        op = XformOp(kTranslateOperation, kRotatePivotPointHint);
        op.setYAnimated(true);
        op.setZAnimated(true);
        aVec.push_back(op);
        staticVec.push_back(0.0); // x
        animVec.push_back(0.0);   // y
        animVec.push_back(0.0);   // z

        Abc::DoubleArraySample data(staticVec);
        a.getSchema().setXform( aVec, data);

        data = Abc::DoubleArraySample(animVec);
        a.getSchema().set( data, OSampleSelector( 0 ) );

        for (size_t i = 1; i < 5; ++i)
        {
            animVec[0] = 2 * (i + 1); // scale x
            animVec[1] = i;  // shear xy
            animVec[2] = -(double)(i);  // shear xz
            animVec[3] = 0;  // shear yz
            animVec[4] = 0.125 * (i + 1); // rot y angle
            animVec[5] = 0.1 * (i + 1); // rot z angle
            animVec[6] = 3 * i;
            animVec[7] = 4 * i;

            data = Abc::DoubleArraySample(animVec);
            a.getSchema().set( data, OSampleSelector( i ) );
        }

    }

    {
        IArchive archive( Alembic::AbcCoreHDF5::ReadArchive(), name );
        IXform a( IObject( archive, kTop ), "a" );
        XformOpVec ops = a.getSchema().getOps();
        TESTING_ASSERT( ops.size() == 6 );
        TESTING_ASSERT( a.getSchema().getNumOps() == 6 );
        TESTING_ASSERT( ops[0].getType() == kScaleOperation );
        TESTING_ASSERT( ops[0].getHint() == kScaleHint );
        TESTING_ASSERT( ops[0].isXAnimated() );
        TESTING_ASSERT( !ops[0].isYAnimated() );
        TESTING_ASSERT( !ops[0].isZAnimated() );
        TESTING_ASSERT( !a.getSchema().isOpStatic(0) );

        TESTING_ASSERT( ops[1].getType() == kMatrixOperation );
        TESTING_ASSERT( ops[1].getHint() == kMayaShearHint );
        TESTING_ASSERT( !a.getSchema().isOpStatic(1) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(0) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(1) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(2) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(3) );
        TESTING_ASSERT( ops[1].isIndexAnimated(4) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(5) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(6) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(7) );
        TESTING_ASSERT( ops[1].isIndexAnimated(8) );
        TESTING_ASSERT( ops[1].isIndexAnimated(9) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(10) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(11) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(12) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(13) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(14) );
        TESTING_ASSERT( !ops[1].isIndexAnimated(15) );

        TESTING_ASSERT( ops[2].getType() == kRotateOperation );
        TESTING_ASSERT( ops[2].getHint() == kRotateHint );
        TESTING_ASSERT( a.getSchema().isOpStatic(2) );
        TESTING_ASSERT( !ops[2].isXAnimated() );
        TESTING_ASSERT( !ops[2].isYAnimated() );
        TESTING_ASSERT( !ops[2].isZAnimated() );
        TESTING_ASSERT( !ops[2].isAngleAnimated() );

        TESTING_ASSERT( ops[3].getType() == kRotateOperation );
        TESTING_ASSERT( ops[3].getHint() == kRotateHint );
        TESTING_ASSERT( !a.getSchema().isOpStatic(3) );
        TESTING_ASSERT( !ops[3].isXAnimated() );
        TESTING_ASSERT( !ops[3].isYAnimated() );
        TESTING_ASSERT( !ops[3].isZAnimated() );
        TESTING_ASSERT( ops[3].isAngleAnimated() );

        TESTING_ASSERT( ops[4].getType() == kRotateOperation );
        TESTING_ASSERT( ops[4].getHint() == kRotateOrientationHint );
        TESTING_ASSERT( !a.getSchema().isOpStatic(4) );
        TESTING_ASSERT( !ops[4].isXAnimated() );
        TESTING_ASSERT( !ops[4].isYAnimated() );
        TESTING_ASSERT( !ops[4].isZAnimated() );
        TESTING_ASSERT( ops[4].isAngleAnimated() );

        TESTING_ASSERT( ops[5].getType() == kTranslateOperation );
        TESTING_ASSERT( ops[5].getHint() == kRotatePivotPointHint );
        TESTING_ASSERT( !a.getSchema().isOpStatic(5) );
        TESTING_ASSERT( !ops[5].isXAnimated() );
        TESTING_ASSERT( ops[5].isYAnimated() );
        TESTING_ASSERT( ops[5].isZAnimated() );

        Abc::DoubleArraySamplePtr s = a.getSchema().getStaticData();
        TESTING_ASSERT( s->size() == 26 );
        TESTING_ASSERT( (*s)[0] == 1.0 );
        TESTING_ASSERT( (*s)[1] == 2.0 );
        TESTING_ASSERT( (*s)[2] == 1.0 );
        TESTING_ASSERT( (*s)[3] == 0.0 );
        TESTING_ASSERT( (*s)[4] == 0.0 );
        TESTING_ASSERT( (*s)[5] == 0.0 );
        TESTING_ASSERT( (*s)[6] == 1.0 );
        TESTING_ASSERT( (*s)[7] == 0.0 );
        TESTING_ASSERT( (*s)[8] == 0.0 );
        TESTING_ASSERT( (*s)[9] == 1.0 );
        TESTING_ASSERT( (*s)[10] == 0.0 );
        TESTING_ASSERT( (*s)[11] == 0.0 );
        TESTING_ASSERT( (*s)[12] == 0.0 );
        TESTING_ASSERT( (*s)[13] == 0.0 );
        TESTING_ASSERT( (*s)[14] == 1.0 );
        TESTING_ASSERT( (*s)[15] == 1.0 );
        TESTING_ASSERT( (*s)[16] == 0.0 );
        TESTING_ASSERT( (*s)[17] == 0.0 );
        TESTING_ASSERT( (*s)[18] == 1.57 );
        TESTING_ASSERT( (*s)[19] == 0.0 );
        TESTING_ASSERT( (*s)[20] == 1.0 );
        TESTING_ASSERT( (*s)[21] == 0.0 );
        TESTING_ASSERT( (*s)[22] == 0.0 );
        TESTING_ASSERT( (*s)[23] == 0.0 );
        TESTING_ASSERT( (*s)[24] == 1.0 );
        TESTING_ASSERT( (*s)[25] == 0.0 );

        for (index_t i = 0; i < 5; ++i)
        {
            Abc::DoubleArraySamplePtr anim =
                a.getSchema().getAnimData( Abc::ISampleSelector(i) );
            TESTING_ASSERT( anim->size() == 8 );
            TESTING_ASSERT( (*anim)[0] == 2.0 * (i + 1) );
            TESTING_ASSERT( (*anim)[1] == i );
            TESTING_ASSERT( (*anim)[2] == -i );
            TESTING_ASSERT( (*anim)[3] == 0 );
            TESTING_ASSERT( (*anim)[4] == 0.125 * (i+1) );
            TESTING_ASSERT( (*anim)[5] == 0.1 * (i+1) );
            TESTING_ASSERT( (*anim)[6] == 3.0 * i );
            TESTING_ASSERT( (*anim)[7] == 4.0 * i );

            XformSample xs;
            a.getSchema().getSample(xs, Abc::ISampleSelector(i));
            TESTING_ASSERT( xs.getNum() == 6 );

            XformDataPtr xp = xs.get(0);
            TESTING_ASSERT( xp->getType() == kScaleOperation );
            ScaleData s( xp );
            TESTING_ASSERT( s.get() == Abc::V3d(2.0 * (i+1), 1.0, 2.0) );

            xp = xs.get(1);
            TESTING_ASSERT( xp->getType() == kMatrixOperation );
            MatrixData m( xp );
            TESTING_ASSERT( m.get() == Abc::M44d(
                1.0, 0.0, 0.0, 0.0,
                  i, 1.0, 0.0, 0.0,
                 -i, 0.0, 1.0, 0.0,
                0.0, 0.0, 0.0, 1.0) );

            xp = xs.get(2);
            TESTING_ASSERT( xp->getType() == kRotateOperation );
            RotateData rx( xp );
            TESTING_ASSERT( rx.getAxis() == Abc::V3d(1.0, 0.0, 0.0) );
            TESTING_ASSERT( rx.getAngle() == 1.57 );

            xp = xs.get(3);
            TESTING_ASSERT( xp->getType() == kRotateOperation );
            RotateData ry(xp);
            TESTING_ASSERT( ry.getAxis() == Abc::V3d(0.0, 1.0, 0.0) );
            TESTING_ASSERT( ry.getAngle() == 0.125 * (i+1) );

            xp = xs.get(4);
            TESTING_ASSERT( xp->getType() == kRotateOperation );
            RotateData rz( xp );
            TESTING_ASSERT( rz.getAxis() == Abc::V3d(0.0, 0.0, 1.0) );
            TESTING_ASSERT( rz.getAngle() == 0.1 * (i+1) );

            xp = xs.get(5);
            TESTING_ASSERT( xp->getType() == kTranslateOperation );
            TranslateData t( xp );
            TESTING_ASSERT( t.get() == Abc::V3d(0.0, 3.0 * i, 4.0 * i) );
        }
    }
}

//-*****************************************************************************
int main( int argc, char *argv[] )
{
    xformOut();
    xformIn();
    someOpsXform();

    return 0;
}
