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

#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/GeometryScope.h>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace Alembic {
namespace AbcGeom {

//-*****************************************************************************
// minor helper function
void OXformSchema::_setXformOpProps( const XformSample &iSamp,
                                     const OSampleSelector &iSS,
                                     const std::vector<chrono_t> &iTimes )
{
    for ( size_t i = 0 ; i < iSamp.m_ops.size() ; ++i )
    {
        XformOp op = iSamp.m_ops[i];

        for ( size_t j = 0 ; j < op.getNumChannels() ; ++j )
        {
            m_props[i + j].set( op.getValue( j ), iSS, iTimes );
        }
    }
}


//-*****************************************************************************
void OXformSchema::set( const XformSample &iSamp,
                        const Abc::OSampleSelector &iSS  )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::set()" );

    ABCA_ASSERT( !iSamp.m_id.is_nil(), "Sample has been reset!" );

    index_t idx = iSS.getIndex();
    chrono_t time = iSS.getTime();

    if ( iSamp.getChildBounds.hasVolume() )
    { m_childBounds.set( iSamp.getChildBounds(), iSS ); }

    if ( iSS.getIndex() == 0 )
    {
        m_sampID = iSamp.m_id;

        m_ops.set( iSamp.m_ops );

        m_times.push_back( time );

        AbcA::CompoundPropertyWriterPtr cptr = this->getPtr();
        Abc::ErrorHandler::Policy pcy = this->getErrorHandlerPolicy();

        std::string namebase = ".oc";

        // Create our well-named Properties, push them into our propvec,
        // and set them.
        for ( size_t i = 0 ; i < iSamp.m_ops.size() ; ++i )
        {
            XformOp op = iSamp.m_ops[i];

            for ( size_t j = 0 ; j < op.getNumChannels() ; ++j )
            {
                std::string channame = boost::lexical_cast<std::string>( i + j );
                prop = ODefaultedDoubleProperty( cptr, namebase + channame,
                                                 pcy, m_timeSamplingType,
                                                 op.getDefaultValue() );

                prop.set( op.getValue( j ), iSS, m_times );

                m_props.push_back( prop );
            }
        }
    }
    else
    {
        ABCA_ASSERT( m_sampID == iSamp.m_id, "Invalid sample ID!" );

        if ( m_times.size() == idx )
        {
            m_times.push_back( time );
            this->_setXformOpProps( iSamp, iSS, m_times );
        }
        else
        {
            std::vector<chrono_t> empty;
            empty.clear();
            this->_setXformOpProps( iSamp, iSS, empty );
        }
    }

    ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OXformSchema::setIsToWorld( bool iIsToWorld,
                                 const Abc::OSampleSelector &iSS  )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::setIsToWorld()" );

    m_isToWorld.set( iIsToWorld, iSS );

    ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OXformSchema::setFromPrevious( const Abc::OSampleSelector &iSS )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::setFromPrevious" );

    if ( m_isToWorld.getNumSamples() > 0 )
    { m_isToWorld.setFromPrevious( iSS ); }

    if ( m_childBounds.getNumSamples() > 0 )
    { m_childBounds.setFromPrevious( iSS ); }

    ALEMBIC_ABC_SAFE_CALL_END();
}

//-*****************************************************************************
void OXformSchema::init( const AbcA::TimeSamplingType &iTst )
{
    ALEMBIC_ABC_SAFE_CALL_BEGIN( "OXformSchema::init()" );

    m_timeSamplingType = iTst;

    m_childBounds = Abc::OBox3dProperty( this->getPtr(), ".childBnds", iTst );

    // This will hold the shape of the xform
    m_ops = Abc::OUInt32ArrayProperty( this->getPtr(), ".ops" );

    m_isToWorld = Abc::OBoolProperty( this->getPtr(), ".istoworld", iTst );

    m_setWithStack = false;

    boost::uuids::nil_generator ng;
    m_sampID = ng();

    ALEMBIC_ABC_SAFE_CALL_END_RESET();
}

} // End namespace AbcGeom
} // End namespace Alembic
