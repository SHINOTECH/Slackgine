/////////////////////////////////////////////////////////////
//
// Slackgine - Copyright (C) 2010-2011
// The Slackgine development team
//
// See the LICENSE file in the top-level directory.
//
// FILE:        renderer.h
// PURPOSE:     OpenGL3 renderer.
// AUTHORS:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#include <cstring>
#include "opengl3.h"
#include "math/matrix.h"
#include "math/transform.h"
#include "../../renderer.h"

namespace Renderer
{

class OpenGL3_Renderer : public IRenderer
{
private:
    bool                m_initialized;
    IProgram*           m_program;
    char                m_error [ 512 ];
    Matrix              m_matProjection;
    Matrix              m_matLookat;
    Vector3             m_viewVector;
    TextureLookupFn     m_texLookup;

public:
                OpenGL3_Renderer        ();
                ~OpenGL3_Renderer       ();
    
    bool        initialize              ();

    bool        beginScene              ( const Matrix& projection, const Matrix& lookAt, TextureLookupFn fn );
    void        setProgram              ( IProgram* program );
    void        pushState               ();
    bool        render                  ( Geometry* geometry, const Transform& transform = IdentityTransform(), bool includeTransparent = false, MeshRenderFn fn = 0 );
    bool        renderGeometryMesh      ( Geometry* geometry, Mesh* mesh, const Transform& transform = IdentityTransform(), MeshRenderFn fn = 0 );
    void        popState                ();
    bool        endScene                ();
    
    void        getError                ( char* dest ) const { strcpy(dest, m_error); }
    
private:
    void        setupLighting           ();
};

}
