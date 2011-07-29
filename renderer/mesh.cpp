#include <cstring>
#include "mesh.h"

using namespace Renderer;

Mesh::Mesh()
: m_indices ( 0 )
, m_numIndices ( 0 )
, m_polyType ( TRIANGLES )
{}

Mesh::~Mesh ()
{
    FreeIndices ();
}

void Mesh::FreeIndices()
{
    if ( m_indices != 0 )
        free ( m_indices );
    m_indices = 0;
    m_numIndices = 0;
}

void Mesh::Load(const unsigned int* pIndices, unsigned int indexCount, PolygonType polyType)
{
    FreeIndices ();
    m_indices = ( unsigned int * )malloc(sizeof(unsigned int) * indexCount);
    memcpy ( m_indices, pIndices, sizeof(unsigned int) * indexCount );
    m_numIndices = indexCount;
    m_polyType = polyType;
}

void Mesh::Set(unsigned int* pIndices, unsigned int indexCount, PolygonType polyType)
{
    if ( pIndices != m_indices )
    {
        FreeIndices ();
        m_indices = pIndices;
    }
    m_numIndices = indexCount;
    m_polyType = polyType;
}
