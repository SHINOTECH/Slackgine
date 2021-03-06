/////////////////////////////////////////////////////////////
//
// Slackgine - Copyright (C) 2010-2011
// The Slackgine development team
//
// See the LICENSE file in the top-level directory.
//
// FILE:        shader.h
// PURPOSE:     OpenGL3 shaders.
// AUTHORS:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#include <cstring>
#include <string>
#include "opengl3.h"

namespace Renderer
{
    
class OpenGL3_Shader : public IShader
{
    bool        m_loaded;
    GLuint      m_handler;
    char        m_error [ 512 ];
    
public:
                        OpenGL3_Shader          ( IShader::Type type );
                        ~OpenGL3_Shader         ();
    
    bool                load                    ( std::istream& fp );
    bool                ok                      () const { return m_handler > 0 && m_loaded; }
    void                getError                ( char* dest ) const { strcpy(dest, m_error); }
    
    GLuint&             handler                 () { return m_handler; }
    const GLuint&       handler                 () const { return m_handler; }
};

}
