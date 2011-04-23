package es.lautech.slackgine.test;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import es.lautech.slackgine.Slackgine;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

public class Renderer implements GLSurfaceView.Renderer
{
	public Renderer ( Context context )
	{
	}
	
	public void onSurfaceCreated(GL10 glUnused, EGLConfig config)
	{
	}
	
	public void onDrawFrame ( GL10 glUnused )
	{
		GLES20.glViewport(0, 0, m_width, m_height);
		GLES20.glClearColor(0.35f, 0.35f, 0.35f, 1.0f );
		Slackgine.Instance().Render ();
	}
	
	public void onSurfaceChanged(GL10 glUnused, int width, int height)
	{
		m_width = width;
		m_height = height;
	}
	
	private int m_width;
	private int m_height;
}