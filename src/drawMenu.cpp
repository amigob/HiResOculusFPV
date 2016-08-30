#include <GL/gl.h>
#include <GL/glut.h>

#include "drawMenu.h"

drawMenu::drawMenu()
{
    fontSize     = 8.0;
    fontColor[0] = 1.0;
    fontColor[1] = 1.0;
    fontColor[2] = 1.0;
    startX       = 0.0;
    startY       = 0.0;
    line         = 0;
}


drawMenu::~drawMenu()
{
} 


void drawMenu::drawString(const char *str)
{
    int index = 0;
    
	glPushMatrix();	
	glPushAttrib(GL_ALL_ATTRIB_BITS);
   glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
//	glDisable(GL_MAP2_VERTEX_3);
//	glDisable(GL_MAP2_TEXTURE_COORD_2);
	glLineWidth(3.0);
	glScalef(0.0001f*fontSize, 0.0001f * fontSize, 0.0f); 
	glColor3f(fontColor[0], fontColor[1], fontColor[2]);		
	glRotatef(90.0f, 0, 0, 1 );				
	glTranslatef( startX, startY - line*fontSize*20, 0.0f);
	
	
	while( str[index] !='\0')	
	{
	    glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, str[index]);
	    index++;
	}
	
	glPopAttrib();
	glPopMatrix();
}


void drawMenu::startPos(GLfloat x, GLfloat y)
{
    startX = x;
    startY = y;
}

   
void drawMenu::setSize(GLfloat s)
{
    fontSize = s;
}


void drawMenu::setColor(GLfloat r, GLfloat g, GLfloat b)    
{
    fontColor[0] = r;
    fontColor[1] = g;
    fontColor[2] = b;    
}


void drawMenu::newMenu(std::string text)
{
    drawString(text.c_str());
    line = 1;
}


void drawMenu::addLine(std::string text)
{
    drawString(text.c_str());
    line++;
}
