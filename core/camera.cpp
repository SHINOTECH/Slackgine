/////////////////////////////////////////////////////////////
//
// Slackgine - Copyright (C) 2010-2012
// The Slackgine development team
//
// See the LICENSE file in the top-level directory.
//
// FILE:        camera.cpp
// PURPOSE:     Camera base class.
// AUTHORS:     Alberto Alonso <rydencillo@gmail.com>
//

#include "camera.h"

using namespace Core;

Camera::Camera ( const l3m::Camera* cam )
{
    setOrthographic ( -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f );
    if ( cam != 0 )
        create ( cam );
}

Camera::~Camera ()
{
}

void Camera::create ( const l3m::Camera* cam )
{
    if ( cam == 0 )
        return;

    switch ( cam->type() )
    {
        case l3m::Camera::CAMERA_ORTHOGRAPHIC:
            setOrthographic( cam->orthographicData().left,
                             cam->orthographicData().right,
                             cam->orthographicData().top,
                             cam->orthographicData().bottom,
                             cam->orthographicData().near,
                             cam->orthographicData().far );
            break;
        case l3m::Camera::CAMERA_PERSPECTIVE:
            setPerspective( cam->perspectiveData().fov,
                            cam->perspectiveData().aspect,
                            cam->perspectiveData().near,
                            cam->perspectiveData().far );
            break;
        default:
            return;
    }

    transform() = cam->transform();
}

void Camera::setOrthographic ( const float& left, const float& right,
                               const float& top, const float& bottom,
                               const float& near, const float& far )
{
    m_type = CAMERA_ORTHOGRAPHIC;
    m_orthographic.left = left;
    m_orthographic.right = right;
    m_orthographic.top = top;
    m_orthographic.bottom = bottom;
    m_orthographic.near = near;
    m_orthographic.far = far;
    makeOrthographicMatrix ();
}

void Camera::setPerspective ( const float& fovy, const float& aspect,
                              const float& near, const float& far )
{
    m_type = CAMERA_PERSPECTIVE;
    m_perspective.fovy = fovy;
    m_perspective.aspect = aspect;
    m_perspective.near = near;
    m_perspective.far = far;
    makePerspectiveMatrix ();
}

void Camera::setCorners (const f32& left, const f32& right, const f32& top, const f32& bottom)
{
    if ( m_type == CAMERA_ORTHOGRAPHIC )
    {
        m_orthographic.left = left;
        m_orthographic.right = right;
        m_orthographic.top = top;
        m_orthographic.bottom = bottom;
        makeOrthographicMatrix ();
    }
}

void Camera::setNearFar (const f32& near, const f32& far)
{
    if ( m_type == CAMERA_ORTHOGRAPHIC )
    {
        m_orthographic.near = near;
        m_orthographic.far = far;
        makeOrthographicMatrix ();
    }
    else
    {
        m_perspective.near = near;
        m_perspective.far = far;
        makePerspectiveMatrix ();
    }
}

void Camera::setFovy (const f32& fovy)
{
    if ( m_type == CAMERA_PERSPECTIVE )
    {
        m_perspective.fovy = fovy;
        makePerspectiveMatrix ();
    }
}

void Camera::setAspect (const f32& aspect)
{
    if ( m_type == CAMERA_PERSPECTIVE )
    {
        m_perspective.aspect = aspect;
        makePerspectiveMatrix ();
    }
}

void Camera::makeOrthographicMatrix ()
{
    m_projection = OrthographicMatrix ( m_orthographic.left,
                                        m_orthographic.right,
                                        m_orthographic.top,
                                        m_orthographic.bottom,
                                        m_orthographic.near,
                                        m_orthographic.far );
}

void Camera::makePerspectiveMatrix ()
{
    m_projection = PerspectiveMatrix ( m_perspective.aspect,
                                       m_perspective.fovy,
                                       m_perspective.near,
                                       m_perspective.far );
}
