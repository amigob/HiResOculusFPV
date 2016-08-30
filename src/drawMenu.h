#include <GL/gl.h>
#include <string>

class drawMenu 
{        
    private:
       GLfloat fontSize;       
       GLfloat fontColor[3];
       GLfloat startX,startY;
       int     line;   
       void    drawString(const char *str);    
    public: 
       drawMenu();
       ~drawMenu(); 
       void startPos(GLfloat x, GLfloat y);
       void setColor(GLfloat r, GLfloat g, GLfloat b);
       void setSize(GLfloat s);
       void newMenu(std::string);
       void addLine(std::string);       
};
