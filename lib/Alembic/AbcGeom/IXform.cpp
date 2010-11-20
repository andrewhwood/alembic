//-*****************************************************************************
//
// Copyright (c) 2009-2010,
//  Sony Pictures Imageworks Inc. and
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
// Industrial Light & Magic, nor the names of their contributors may be used
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

#include <Alembic/AbcGeom/IXform.h>

namespace Alembic {
namespace AbcGeom {

//-*****************************************************************************
void IXformSchema::init( const Abc::IArgument &iArg0,
                            const Abc::IArgument &iArg1 )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "IXformTrait::init()" );

    // It seems like I'm not using kNoMatching correctly.
    // What I really want to do is to have an invalid object, if
    // the array properties don't exist.
    // Instead exceptions are being thrown.

    Abc::IUInt32ArrayProperty ops( *this, ".ops", kNoMatching );

    if (ops.valid())
    {
        Abc::UInt32ArraySamplePtr opSamp;
        ops.get(opSamp);
        if (opSamp)
        {
            size_t numOps = opSamp->size();
            m_ops.resize(numOps);
            for (size_t i = 0; i < numOps; ++i)
            {
                XformOp op;
                op.setEncodedValue( (*opSamp)[i] );
                m_ops[i] =  op;
            }
        }
    }

    Abc::IDoubleArrayProperty staticData( *this, ".static",
        args.getSchemaInterpMatching() );
    if (staticData.valid())
    {
        staticData.get(m_static);
    }

    m_anim = Abc::IDoubleArrayProperty( *this, ".anim", kNoMatching );
    m_inherits = Abc::IBoolProperty( *this, ".inherits", kNoMatching );

    ALEMBIC_ABC_SAFE_CALL_END_RESET();
}

Abc::M44d IXformSchema::getMatrix( const Abc::ISampleSelector &iSS )
{
    Abc::M44d ret;
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "IXformTrait::getMatrix()" );

    Abc::DoubleArraySamplePtr anim;
    m_anim.get(anim, iSS);

    size_t staticIndex = 0;
    size_t animIndex = 0;

    size_t numOps = m_ops.size();
    for (size_t i = 0; i < numOps; ++i)
    {
        Abc::M44d m;
        XformOperationType type = m_ops[i].getType();
        if (type == cMatrixOperation)
        {
            for (size_t j = 0; j < 4; ++j)
            {
                for (size_t k = 0; k < 4; ++k)
                {
                    if (m_ops[i].isIndexAnimated(j))
                    {
                        m.x[j][k] = (*m_static)[staticIndex];
                        staticIndex ++;
                    }
                    else
                    {
                        m.x[j][k] = (*anim)[animIndex];
                        animIndex ++;
                    }
                }
            }
        }
        else
        {
            double x, y, z;
            if (m_ops[i].isXAnimated())
            {
                x = (*m_static)[staticIndex];
                staticIndex ++;
            }
            else
            {
                x = (*anim)[animIndex];
                animIndex ++;
            }

            if (m_ops[i].isYAnimated())
            {
                y = (*m_static)[staticIndex];
                staticIndex ++;
            }
            else
            {
                y = (*anim)[animIndex];
                animIndex ++;
            }

            if (m_ops[i].isZAnimated())
            {
                z = (*m_static)[staticIndex];
                staticIndex ++;
            }
            else
            {
                z = (*anim)[animIndex];
                animIndex ++;
            }

            if (type == cScaleOperation)
            {
                m.setScale( V3d(x,y,z) );
            }
            else if (type == cTranslateOperation)
            {
                m.setTranslation( V3d(x, y, z) );
            }
            else if (type == cRotateOperation)
            {
                double angle;
                if (m_ops[i].isAngleAnimated())
                {
                    angle = (*m_static)[staticIndex];
                    staticIndex ++;
                }
                else
                {
                    angle = (*anim)[animIndex];
                    animIndex ++;
                }

                m.setAxisAngle( V3d(x,y,z), angle );
            }
        }

        ret = m * ret;
    }
    ALEMBIC_ABC_SAFE_CALL_END();

    return ret;
}

} // End namespace AbcGeom
} // End namespace Alembic