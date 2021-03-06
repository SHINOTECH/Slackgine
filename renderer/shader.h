/////////////////////////////////////////////////////////////
//
// Slackgine - Copyright (C) 2010-2011
// The Slackgine development team
//
// See the LICENSE file in the top-level directory.
//
// FILE:        shader.h
// PURPOSE:     Interface for the renderer shaders.
// AUTHORS:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#include <iostream>
#include <fstream>

namespace Renderer
{
    
class IShader
{
public:
    enum Type
    {
        VERTEX_SHADER,
        FRAGMENT_SHADER,
        GEOMETRY_SHADER
    };

public:
    virtual             ~IShader        () {}
    
    bool                load            ( const char* file )
    {
        std::ifstream fp ( file, std::ios::in | std::ios::binary );
        if ( fp.fail() )
            return false;
        return load ( fp );
    }
    virtual bool        load            ( std::istream& fp ) = 0;
    virtual bool        ok              () const = 0;
    virtual void        getError        ( char* dest ) const = 0;
};

}

#include "renderer.h"
