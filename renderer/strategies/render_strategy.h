/////////////////////////////////////////////////////////////
//
// Slackgine - Copyright (C) 2010-2011
// The Slackgine development team
//
// See the LICENSE file in the top-level directory.
//
// FILE:        render_strategy.h
// PURPOSE:     Interface for rendering strategies.
// AUTHORS:     Alberto Alonso <rydencillo@gmail.com>
//

#pragma once

#include <vector>
#include "core/slackgine.h"
#include "core/entity.h"
#include "core/camera.h"
#include "renderer/shader.h"

namespace Core { class Slackgine; }

namespace Renderer
{

class RenderStrategy
{
    typedef Renderer::IRenderer::MeshRenderFn MeshRenderFn;
public:
    //--------------------------------------------------------------------------
    // Constructor / Destructor
                    RenderStrategy          ();
    virtual         ~RenderStrategy         ();
    
public:
    //--------------------------------------------------------------------------
    // Abstract methods to handle initialization/finalization
    virtual bool    beginScene              ( Core::Slackgine* sg, Core::Camera* cam ) = 0;
    virtual bool    endScene                ( Core::Slackgine* sg ) = 0;
    
    //--------------------------------------------------------------------------
    // Method to execute this strategy
    virtual bool    execute                 ( Core::Slackgine* sg, Core::Entity* startAt );
    
    //--------------------------------------------------------------------------
    // Error management
    void            getError                ( char* dest ) const;
    bool            isOk                    () const { return ( m_error[0] == 0 ); }
protected:
    void            setError                ( const char* err );
    
private:
    //--------------------------------------------------------------------------
    // Default handler for mesh render events
    static bool     defaultMeshHandler      ( Mesh* ) { return true; }
public:
    //--------------------------------------------------------------------------
    // Setter and getter for the mesh render events
    void            setMeshHandler          ( MeshRenderFn handler );
    void            resetMeshHandler        ();
protected:
    MeshRenderFn&   getMeshHandler          () { return m_meshDelegate; }
    
private:
    //--------------------------------------------------------------------------
    // Private class attributes
    char            m_error [ 256 ];
    MeshRenderFn    m_meshDelegate;
};

}
