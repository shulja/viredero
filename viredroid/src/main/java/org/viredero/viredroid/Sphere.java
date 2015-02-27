package org.viredero.viredroid;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.nio.ShortBuffer;

import android.opengl.GLES20;
import android.util.Log;


public class Sphere {


    private FloatBuffer vertexBuffer;
    private ShortBuffer indexBuffer;

    private final static float PI = 3.1415926535f; //FIXME: there is pi in math?
    private final static float PI_2 = 1.57079632679f;

    float[] points;
    float[] normals;
    float[] texcoords;
    short[] indices;    // should probably be short

    // Set color with red, green, blue and alpha (opacity) values
    float color[] = { 1f, 0f, 0f, 1.0f };

    private final int mProgram;
    private int mPositionHandle;
    private int mColorHandle;
    private int mMVPMatrixHandle;

    String vertexShaderCode =
	"uniform mat4 uMVPMatrix;" +
	"attribute vec4 vPosition;" +
	"void main() {" +
	"  gl_Position = vPosition * uMVPMatrix;" +
	"}";

    private final String fragmentShaderCode =
	"precision mediump float;" +
	"uniform vec4 vColor;" +
	"void main() {" +
	"  gl_FragColor = vColor;" +
	"}";

    Sphere(float radius, int rings, int sectors)
    {
	float R = 1f/(float)(rings-1);
	float S = 1f/(float)(sectors-1);
	short r, s;
	float x, y, z;

	points = new float[rings * sectors * 3];
	normals = new float[rings * sectors * 3];
	texcoords = new float[rings * sectors * 2];

	int t = 0, v = 0, n = 0;
	for(r = 0; r < rings; r++) {
	    for(s = 0; s < sectors; s++) {
		y = (float) Math.sin( -PI_2 + PI * r * R );
		x = (float) (Math.cos(2*PI * s * S) * Math.sin( PI * r * R ));
		z = (float) (Math.sin(2*PI * s * S) * Math.sin( PI * r * R ));

		texcoords[t++] = s*S;
		texcoords[t++] = r*R;

		points[v++] = x * radius;
		points[v++] = y * radius;
		points[v++] = z * radius;

		//Log.i("Sphere","Added point (" + x +", " + y + ", " +z +")");

		normals[n++] = x;
		normals[n++] = y;
		normals[n++] = z;
	    }


	}
	int counter = 0;
	indices = new short[rings * sectors * 6];
	for(r = 0; r < rings - 1; r++){
	    for(s = 0; s < sectors-1; s++) {
		indices[counter++] = (short) (r * sectors + s);       //(a)
		indices[counter++] = (short) (r * sectors + (s+1));    //(b)
		indices[counter++] = (short) ((r+1) * sectors + (s+1));  // (c)
		indices[counter++] = (short) ((r+1) * sectors + (s+1));  // (c)
		indices[counter++] = (short) (r * sectors + (s+1));    //(b)
		indices[counter++] = (short) ((r+1) * sectors + s);     //(d)
	    }
	}
	// initialize vertex byte buffer for shape coordinates
	ByteBuffer bb = ByteBuffer.allocateDirect(
						  // (# of coordinate values * 4 bytes per float)
						  points.length * 4);
	bb.order(ByteOrder.nativeOrder());
	vertexBuffer = bb.asFloatBuffer();
	vertexBuffer.put(points);
	vertexBuffer.position(0);

	// initialize byte buffer for the draw list
	ByteBuffer dlb = ByteBuffer.allocateDirect(
						   // (# of coordinate values * 2 bytes per short)
						   indices.length * 2);
	dlb.order(ByteOrder.nativeOrder());
	indexBuffer = dlb.asShortBuffer();
	indexBuffer.put(indices);
	indexBuffer.position(0);

	// prepare shaders and OpenGL program
	int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER,
				      vertexShaderCode);
	int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER,
					fragmentShaderCode);

	mProgram = GLES20.glCreateProgram();             // create empty OpenGL Program
	GLES20.glAttachShader(mProgram, vertexShader);   // add the vertex shader to program
	GLES20.glAttachShader(mProgram, fragmentShader); // add the fragment shader to program
	GLES20.glLinkProgram(mProgram);   

    }
    
    public static int loadShader(int type, String shaderCode){
	
	// create a vertex shader type (GLES20.GL_VERTEX_SHADER)
	// or a fragment shader type (GLES20.GL_FRAGMENT_SHADER)
	int shader = GLES20.glCreateShader(type);

	// add the source code to the shader and compile it
	GLES20.glShaderSource(shader, shaderCode);
	GLES20.glCompileShader(shader);

	return shader;
    }

    public void draw(float[] modelView) {
	// Add program to OpenGL environment
	GLES20.glUseProgram(mProgram);

	// get handle to vertex shader's vPosition member
	mPositionHandle = GLES20.glGetAttribLocation(mProgram, "vPosition");

	// Enable a handle to the triangle vertices
	GLES20.glEnableVertexAttribArray(mPositionHandle);

	// Prepare the triangle coordinate data
	GLES20.glVertexAttribPointer(mPositionHandle, 3,
				     GLES20.GL_FLOAT, false,
				     3*4, vertexBuffer);

	// get handle to fragment shader's vColor member
	mColorHandle = GLES20.glGetUniformLocation(mProgram, "vColor");

	// Set color for drawing the triangle
	GLES20.glUniform4fv(mColorHandle, 1, color, 0);

	// get handle to shape's transformation matrix
	mMVPMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uMVPMatrix");

	// Apply the projection and view transformation
	GLES20.glUniformMatrix4fv(mMVPMatrixHandle, 1, false, modelView, 0);

	// Draw the triangle
	GLES20.glDrawElements(GLES20.GL_TRIANGLES, indices.length, GLES20.GL_UNSIGNED_SHORT, indexBuffer);

	// Disable vertex array
	GLES20.glDisableVertexAttribArray(mPositionHandle);
    }

}
