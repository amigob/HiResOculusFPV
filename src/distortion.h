#include <GL/gl.h>

class distortion 
{        
    private:
       GLfloat ctrlpoints[4][4][3];       
       GLfloat barrel_dis;
       GLfloat eye_dis;
       void fill_barrel_distortion();
    public: 
       distortion();
       ~distortion(); 
       void init(GLfloat barrel, GLfloat eye);
       void addBarrelDisto(GLfloat);
       void addEyeDis(GLfloat); 
       GLfloat getBarrelDisto();
       GLfloat getEyeDist();
       GLfloat* getDistortion();
};
