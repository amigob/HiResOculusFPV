//////////////////////////////////////////////////////////////////////////////
// main.cpp
// ========
// testing Pixel Buffer Object for unpacking (uploading) pixel data to PBO
// using GL_ARB_pixel_buffer_object extension
// It uses 2 PBOs to optimize uploading pipeline; application to PBO, and PBO to
// texture object.
//
// CREATED: 2007-10-22
// UPDATED: 2014-04-24
///////////////////////////////////////////////////////////////////////////////
// Thermal Conductivit 3M  8940

// in order to get function prototypes from glext.h, define GL_GLEXT_PROTOTYPES before including glext.h
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h> 

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <errno.h>
#include <math.h>

#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include "glInfo.h"		// glInfo struct
#include "Timer.h"
#include "distortion.h"
#include "drawMenu.h"
#include "settings_i.h"


// GLUT CALLBACK functions ////////////////////////////////////////////////////
void displayCB ();
void reshapeCB (int w, int h);
void idleCB ();
void keyboardCB (unsigned char key, int x, int y);
void mouseCB (int button, int stat, int x, int y);
void mouseMotionCB (int x, int y);

// CALLBACK function when exit() called ///////////////////////////////////////
void exitCB ();
void initGL ();
int initGLUT (int argc, char **argv);
bool initSharedMem ();
void clearSharedMem ();
void updatePixels (GLubyte * scr, GLubyte * dst);
void drawText ();

// constants
const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 300;
const float CAMERA_DISTANCE = 3.34f;

//const float  CAMERA_DISTANCE = 4.0f;
const int TEXT_WIDTH = 8;
const int TEXT_HEIGHT = 13;
const int PAL_IMAGE_WIDTH = 720; 
const int PAL_IMAGE_HEIGHT = 576;	
const int NTSC_IMAGE_WIDTH = 720; 
const int NTSC_IMAGE_HEIGHT = 480;	


const GLenum PIXEL_FORMAT = GL_RGB;

int IMAGE_WIDTH = PAL_IMAGE_WIDTH; 
int IMAGE_HEIGHT = PAL_IMAGE_HEIGHT;
int DATA_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * 3;

GLuint pboIds[2];		// IDs of PBO
GLuint textureId[2];		// ID of texture
GLuint vertex_id;
GLubyte * imageData = 0;	// pointer to texture buffer
GLubyte *interBuffer = 0;	// pointer to texture buffer

int screenWidth;
int screenHeight;
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;

bool pboSupported;
int pboMode;
int drawMode = 0;

Timer timer, t1, t2;
float copyTime, updateTime;


float Rx = -10.0;
float Ry = 10.0;

double dx1 = -0.20;
double dy1 = -0.20;
double dx2 = dx1 * 2.0;
double dy2 = dy1 * 2.0;

double image_position = 0.22;
float  image_zoom;

bool viewMenu = false;
distortion video_disto[2];

GLfloat texpts[2][2][2] ={{{0.0, 0.0},{0.0, 1.0}},{{1.0, 0.0},{1.0, 1.0}}};


#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer 
{
   void *start;
   size_t length;
};

struct buffer *buffers;

unsigned int n_buffers = 2;
int fd = -1;


///////////////////////////////////////////////////////////////////////////////
int main (int argc, char **argv) 
{
   GLubyte * ptr[2];
   
   IMAGE_WIDTH = PAL_IMAGE_WIDTH; 
   IMAGE_HEIGHT = PAL_IMAGE_HEIGHT;      
   DATA_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * 3;
   
  
   
   initSharedMem ();

   // register exit callback
   atexit (exitCB);

   // init GLUT and GL
   initGLUT (argc, argv);
   initGL ();

   // get OpenGL info
   glInfo glInfo;
   glInfo.getInfo ();

   //glInfo.printSelf();
   image_zoom = Settings.getImageZoom ();
   image_position = Settings.getImagePosition ();

   // init 2 texture objects
   for (vertex_id = 0; vertex_id < 2; vertex_id++)
   {
      if ( vertex_id )
         video_disto[vertex_id].init (Settings.getBarrelDisto(), Settings.getEyeDisto());
      else
         video_disto[vertex_id].init (Settings.getBarrelDisto(), -Settings.getEyeDisto());
         
      glGenTextures (1, &textureId[vertex_id]);
      glBindTexture (GL_TEXTURE_2D, textureId[vertex_id]);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, IMAGE_WIDTH, IMAGE_HEIGHT, 0, 
                    PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid *) imageData);
      glBindTexture (GL_TEXTURE_2D, 0);
   }

   if (glInfo.isExtensionSupported ("GL_ARB_pixel_buffer_object"))
   {
      pboSupported = true;
      pboMode = 1;
      std::cout << "Video card supports GL_ARB_pixel_buffer_object." << std::endl;
   }
   else
   {
      pboSupported = false;
      pboMode = 0;
      std::cout << "Video card does NOT support GL_ARB_pixel_buffer_object." << std::endl;
      exit (EXIT_FAILURE);
   }

   // create 2 pixel buffer objects, you need to delete them when program exits.
   // glBufferDataARB with NULL pointer reserves only memory space.
   glGenBuffersARB (2, pboIds);
   glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[0]);
   glBufferDataARB (GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
   ptr[0] = (GLubyte *) glMapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
   glUnmapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB);	// release pointer to mapping buffer
   glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[1]);
   glBufferDataARB (GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
   glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, 0);
   ptr[1] = (GLubyte *) glMapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
   glUnmapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB);	// release pointer to mapping buffer
   Initialize_video (ptr);

   // start timer, the elapsed time will be used for updateVertices()
   timer.start ();

   // the last GLUT call (LOOP)
   // window will be shown and display callback is triggered by events
   // NOTE: this call never return main().
   glutFullScreen ();
   glutMainLoop ();		/* Start GLUT event-processing loop */
   uninitialize_video ();
   return 0;
}


///////////////////////////////////////////////////////////////////////////////
// initialize GLUT for windowing
///////////////////////////////////////////////////////////////////////////////
int initGLUT (int argc, char **argv) 
{
   int handle = 0;

   // GLUT stuff for windowing
   // initialization openGL window.
   // it is called before any other GLUT routine
   glutInit (&argc, argv);
   glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE);	// display mode| GLUT_ALPHA GLUT_DOUBLE |
   glutInitWindowSize (SCREEN_WIDTH, SCREEN_HEIGHT);	// window size
   glutInitWindowPosition (100, 100);	// window location

   // finally, create a window with openGL context
   // Window will not displayed until glutMainLoop() is called
   // it returns a unique ID
   handle = glutCreateWindow (argv[0]);	// param is the title of window

   // register GLUT callback functions
t1.start ();
   glutDisplayFunc (displayCB);

   glutIdleFunc (idleCB);	// redraw only every given millisec
   glutReshapeFunc (reshapeCB);
   glutKeyboardFunc (keyboardCB);

   //glutMouseFunc(mouseCB);
   //glutMotionFunc(mouseMotionCB);
   return handle;
}


///////////////////////////////////////////////////////////////////////////////
// initialize OpenGL
// disable unused features
///////////////////////////////////////////////////////////////////////////////
void initGL () 
{

   //@glShadeModel(GL_SMOOTH);                    // shading mathod: GL_SMOOTH or GL_FLAT
   glShadeModel (GL_FLAT);	// shading mathod: GL_SMOOTH or GL_FLAT
   glPixelStorei (GL_UNPACK_ALIGNMENT, 3);	// 3-byte pixel alignment

   // enable /disable features
   glEnable (GL_DEPTH_TEST);
   glDisable (GL_LIGHTING);
   glEnable (GL_TEXTURE_2D);

   // track material ambient and diffuse from surface color, call it before glEnable(GL_COLOR_MATERIAL)
   glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
   glEnable (GL_COLOR_MATERIAL);
   glClearColor (0, 0, 0, 0);	// background color
   glClearStencil (0);		// clear stencil buffer
   glClearDepth (1.0f);		// 0 is near, 1 is far
   glDepthFunc (GL_LEQUAL);
}  

///////////////////////////////////////////////////////////////////////////////
// initialize global variables
///////////////////////////////////////////////////////////////////////////////
bool initSharedMem () 
{
   screenWidth = SCREEN_WIDTH;
   screenHeight = SCREEN_HEIGHT;
   mouseLeftDown = mouseRightDown = false;
   mouseX = mouseY = 0;
   cameraAngleX = 0;		//180;
   cameraAngleY = 0;		//-180;
   image_zoom = CAMERA_DISTANCE;
   drawMode = 0;		// 0:fill, 1: wireframe, 2:points

   // allocate texture buffer
   imageData = new GLubyte[DATA_SIZE];
   memset (imageData, 0, DATA_SIZE);
   return true;
}


///////////////////////////////////////////////////////////////////////////////
// clean up shared memory
///////////////////////////////////////////////////////////////////////////////
void clearSharedMem () 
{
   // deallocate texture buffer
   delete[]imageData;
   imageData = 0;

   // clean up texture
   for (vertex_id = 0; vertex_id < 2; vertex_id++)
   {
      glDeleteTextures (1, &textureId[vertex_id]);
   }

   // clean up PBOs
   if (pboSupported)
   {
      glDeleteBuffersARB (2, pboIds);
   }
}


///////////////////////////////////////////////////////////////////////////////
// copy an image data to texture buffer
///////////////////////////////////////////////////////////////////////////////
void updatePixels (GLubyte * src, GLubyte * dst) 
{
   int* dst_int = NULL;
   int* src_int = NULL;
   if(!dst)
      return;
   
   src_int = (int*)src;
   dst_int = (int*)dst;
   
   for(int i = 0; i < ((IMAGE_HEIGHT*IMAGE_WIDTH*3)/4); ++i)
   {
      *dst_int = *src_int;
      dst_int++;
      src_int++;
   } 
}    

void getFrame (GLubyte * dst) 
{

}   

///////////////////////////////////////////////////////////////////////////////
// set the projection matrix as perspective
///////////////////////////////////////////////////////////////////////////////
void toPerspective () 
{
   // set viewport to be the entire window
   glViewport (0, 0, (GLsizei) screenWidth, (GLsizei) screenHeight);

   // set perspective viewing frustum
   glMatrixMode (GL_PROJECTION);
   glLoadIdentity ();
   gluPerspective (60.0f, (float) (screenWidth) / screenHeight, 1.0f, 1000.0f);	// FOV, AspectRatio, NearClip, FarClip

   // switch to modelview matrix in order to set scene
   glMatrixMode (GL_MODELVIEW);
   glLoadIdentity ();
}   

//=============================================================================
// CALLBACKS
//=============================================================================
void displayCB () 
{
   static int index = 0;
   int nextIndex = 0;
t1.stop ();
updateTime = t1.getElapsedTimeInMilliSec ();
std::cout << "Updating Time: " << updateTime << " ms\n" << std::ends;
t1.start ();
   

   // clear buffer
   glFlush ();
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   // "index" is used to copy pixels from a PBO to a texture object
   // "nextIndex" is used to update pixels in a PBO
   if (pboMode == 1)
   {
      // In single PBO mode, the index and nextIndex are set to 0
      index = nextIndex = 0;
   }
   else 
   {
      // In dual PBO mode, increment current index first then get the next index
      index = (index + 1) % 2;
      nextIndex = (index + 1) % 2;
   }

   glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[nextIndex]);
   glBufferDataARB (GL_PIXEL_UNPACK_BUFFER_ARB, DATA_SIZE, 0, GL_STREAM_DRAW_ARB);
   GLubyte * ptr = (GLubyte *) glMapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
   
   if (ptr)
   {  // update data directly on the mapped buffer       
      getFrame(ptr);
	  
      glUnmapBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB);	// release pointer to mapping buffer
   }

   // it is good idea to release PBOs with ID 0 after use.
   // Once bound with 0, all pixel operations behave normal ways.
   glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, 0);

   // start to copy from PBO to texture object ///////

   // bind the texture and PBO
   for (vertex_id = 0; vertex_id < 2; vertex_id++)
   {
      glBindTexture (GL_TEXTURE_2D, textureId[vertex_id]);
      glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_ARB, pboIds[index]);
      glEnable (GL_MAP2_VERTEX_3);
      glEnable (GL_DEPTH_TEST);
      glEnable (GL_TEXTURE_2D);
      glEnable (GL_MAP2_TEXTURE_COORD_2);

      glColor3f (1.0f, 1.0f, 1.0f);
      
      glMap2f (GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4, video_disto[vertex_id].getDistortion ());
      glMap2f (GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2, &texpts[0][0][0]);
      glMapGrid2f (20, 0.0, 1.0, 20, 0.0, 1.0);
      
      glShadeModel (GL_FLAT);

      // copy pixels from PBO to texture object
      // Use offset instead of ponter.
      glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT,
         PIXEL_FORMAT, GL_UNSIGNED_BYTE, 0);

      // start to modify pixel values ///////////////////    
      glPushMatrix ();

      // transform camera
      if (vertex_id == 0)
      {
         glTranslatef ((-1.78 + image_position), 0, -image_zoom);
      }
      else
      {
         glTranslatef ((+1.78 - image_position), 0, -image_zoom);
      }
      
      glRotatef (cameraAngleX, 1, 0, 0);	// RX
      glRotatef (cameraAngleY, 0, 1, 0);	// RY
      glRotatef (-90, 0, 0, 1);	// RZ

      // draw a point with texture          
      glEvalMesh2 (GL_FILL, 0, 20, 0, 20);
      
      if (viewMenu)
         drawText ();
      
      glPopMatrix ();
      glBindTexture (GL_TEXTURE_2D, 0);
   }
   glutSwapBuffers ();
   glFinish ();
}


void reshapeCB (int width, int height) 
{
   screenWidth = width;
   screenHeight = height;
   toPerspective ();
}

void idleCB () 
{
   glutPostRedisplay ();
}   


/*------------------- Inter action-----------*/ 
void keyboardCB (unsigned char key, int x, int y) 
{
   switch (key)
   {
      case 27:			// ESCAPE
         exit (0);
         break;
      case 'a':
         video_disto[0].addEyeDis (0.02);
         video_disto[1].addEyeDis (-0.02);
         break;
      case 'A':
         video_disto[0].addEyeDis (-0.02);
         video_disto[1].addEyeDis (0.02);
         break;
      case 'q':
         video_disto[0].addBarrelDisto (0.02);
         video_disto[1].addBarrelDisto (0.02);
         break;
      case 'Q':
         video_disto[0].addBarrelDisto (-0.02);
         video_disto[1].addBarrelDisto (-0.02);
         break;
      case 'w':
         image_zoom = image_zoom + 0.02;
         break;
      case 'W':
         image_zoom = image_zoom - 0.02;
         break;
      case 's':
         image_position = image_position + 0.005;
         break;
      case 'S':
         image_position = image_position - 0.005;
         break;
      case 'f':
         glutPositionWindow (100, 100);
         break;
      case 'F':
         glutFullScreen ();
         break;
      case 'h':
         Ry = Ry - 2;
         break;
      case 'k':
         Ry = Ry + 2;
         break;
      case 'j':
         Rx = Rx - 2;
         break;
      case 'u':
         Rx = Rx + 2;
         break;
      case 'n':
      case 'N':
         Settings.SetImagePosition (image_position);
         Settings.SetImageZoom (image_zoom);
         Settings.SetBarrelDisto (video_disto[0].getBarrelDisto ());
         Settings.SetEyeDisto (video_disto[0].getEyeDist ());
         Settings.save ();
         break;
      case 'm':
      case 'M':
         if (viewMenu)
            viewMenu = false;

         else
            viewMenu = true;
         break;
      default:;
   }

   printf ("image_zoom : %f\n", image_zoom);
   printf ("image_position      : %f\n", image_position);
   fflush (stdout);
}


void mouseCB (int button, int state, int x, int y) 
{
   mouseX = x;
   mouseY = y;
   if (button == GLUT_LEFT_BUTTON)

   {
      if (state == GLUT_DOWN)

      {
         mouseLeftDown = true;
      }

      else if (state == GLUT_UP)
         mouseLeftDown = false;
   }

   else if (button == GLUT_RIGHT_BUTTON)

   {
      if (state == GLUT_DOWN)

      {
         mouseRightDown = true;
      }

      else if (state == GLUT_UP)
         mouseRightDown = false;
   }
   fflush (stdout);
}

void mouseMotionCB (int x, int y) 
{
   if (mouseLeftDown)

   {
      cameraAngleY += (x - mouseX);
      cameraAngleX += (y - mouseY);
      mouseX = x;
      mouseY = y;
   }
   if (mouseRightDown)

   {
      image_zoom -= (y - mouseY) * 0.2f;
      if (image_zoom < 2.0f)
         mouseY = y;
   }
}


void exitCB () 
{
   clearSharedMem ();
}  

void drawText () 
{
   drawMenu test;
   test.startPos (-1500, 800);
   test.setSize (7.0);
   std::stringstream text;
   text << "Barrel Distorion (Q): " << video_disto[0].getBarrelDisto ();
   test.newMenu (text.str ());
   text.str (std::string ());
   text << "Eye distortion   (A): " << video_disto[0].getEyeDist ();
   test.addLine (text.str ());
   text.str (std::string ());
   text << "Zoom             (W): " << image_zoom;
   test.addLine (text.str ());
   text.str (std::string ());
   text << "eye position     (S): " << image_position;
   test.addLine (text.str ());
} 
