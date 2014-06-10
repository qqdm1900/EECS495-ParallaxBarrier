// flip.cpp : Defines the entry point for the console application.
//
// g++ -lGLU -lglut -lhighgui -I/usr/include/nvidia -I/usr/include -I/usr/include/opencv flip.cpp -o flip

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <opencv/highgui.h>
#include <nvidia/GL/gl.h>
#include <nvidia/GL/glx.h>
#include <nvidia/GL/glext.h>
#ifndef GLX_SGI_swap_control
typedef int ( * PFNGLXSWAPINTERVALSGIPROC) (int interval);
#endif

GLuint *textures;
const unsigned int MAX_IMAGES=100;
const unsigned int MAX_STRLEN=200;
char *mask_image_fns[MAX_IMAGES];
char *screen_image_fns[MAX_IMAGES];
char *mask_image_pinhole_fns[MAX_IMAGES];
char *screen_image_pinhole_fns[MAX_IMAGES];
unsigned int n_mask_images;
unsigned int n_screen_images;
unsigned int n_mask_images_pinhole;
unsigned int n_screen_images_pinhole;
char pinhole = 0;
GLfloat m1v=0.0f;
GLfloat m1h=0.0f;
GLfloat m2v=0.0f;
GLfloat m2h=0.0f;
char swap=1;
unsigned int ntex=0;
float immul=0.0f;

static void requestSynchornizedSwapBuffers(void);

void gammaadj(double adj) {
  static double gamma=1.0;
  gamma += adj;

  char cmd[200];

  snprintf(cmd,200,"nvidia-settings --assign RedGamma=%f --assign BlueGamma=%f --assign GreenGamma=%f",gamma,gamma,gamma);

  system(cmd);
}

void gammareset() {
  system("nvidia-settings --assign RedGamma=1 --assign BlueGamma=1 --assign GreenGamma=1");
}

void videoTimer(int i) {
	glutTimerFunc(8,videoTimer,0);
	glutPostRedisplay();
}

void onRender() {
	static char which_mask=0;
	static char which_screen=0;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, textures[which_mask+pinhole*(n_mask_images+n_screen_images)]);

	//printf("display mask %d\n",which_mask+pinhole*(n_mask_images+n_screen_images));

	glBegin(GL_QUADS);

	glTexCoord2i(0, 1050);
	glVertex2f(-1.0f+m1h+(float)swap, -1.0f+m1v); 

	glTexCoord2i(1680, 1050);
	glVertex2f( 0.0f+m1h+(float)swap, -1.0f+m1v);
	
	glTexCoord2i(1680, 0); 
	glVertex2f( 0.0f+m1h+(float)swap, 1.0f+m1v); 

	glTexCoord2i(0, 0); 
	glVertex2f(-1.0f+m1h+(float)swap, 1.0f+m1v);

	glEnd();
	
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, textures[which_screen+n_mask_images+pinhole*(n_screen_images+n_mask_images_pinhole)]);
	//printf("display screen %d\n",which_screen+n_mask_images+pinhole*(n_screen_images+n_mask_images_pinhole));

	glBegin(GL_QUADS);
	
	glTexCoord2i(0, 1050);
	glVertex2f(0.0f+m2h-(float)swap, -1.0f+m2v); 
	
	glTexCoord2i(1680, 1050);
	glVertex2f( 1.0f+m2h-(float)swap, -1.0f+m2v); 
	
	glTexCoord2i(1680, 0); 
	glVertex2f( 1.0f+m2h-(float)swap, 1.0f+m2v); 
	
	glTexCoord2i(0, 0); 
	glVertex2f(0.0f+m2h-(float)swap, 1.0f+m2v);
	
	glEnd();

	glDisable(GL_TEXTURE_RECTANGLE_NV);

	glFlush();
	glutSwapBuffers();

	if(pinhole) {
	  if(++which_mask>=n_mask_images_pinhole) which_mask = 0;
	  if(++which_screen>=n_screen_images_pinhole) which_screen = 0;
	} else {
	  if(++which_mask>=n_mask_images) which_mask = 0;
	  if(++which_screen>=n_screen_images) which_screen = 0;
	}

	glFinish();
}

void onKeyDown(unsigned char key, int x, int y) {
  FILE *save;

  switch (key) {
  case 27:
  case 'q':
    printf("exiting...\n");
    gammareset();
    exit(0);
    break;
  case ' ':
    onRender();
    break;
  case 'm':
    pinhole=!pinhole;
    break;
  case 'a':
    m1h-=2.0/1680.0;
    break;
  case 'w':
    m1v+=2.0/1050.0;
    break;
  case 'd':
    m1h+=2.0/1680.0;
    break;
  case 's':
    m1v-=2.0/1050.0;
    break;
  case 'k':
    m2h-=2.0/1680.0;
    break;
  case 'o':
    m2v+=2.0/1050.0;
    break;
  case ';':
    m2h+=2.0/1680.0;
    break;
  case 'l':
    m2v-=2.0/1050.0;
    break;
  case '`':
    save = fopen("shifts.txt","w");
    if(save != NULL) {
      fprintf(save,"%f %f\n%f %f\n",m1h,m1v,m2h,m2v);
      printf("saved shifts.txt: %f %f %f %f\n",m1h, m1v, m2h, m2v);
      fclose(save);
    }
    break;
  case 'f':
    swap=!swap;
    break;
  case 't':
    gammaadj(0.05);
    break;
  case 'g':
    gammaadj(-0.05);
    break;
  }
}

void printGLErr() {

  GLenum err;

  err = glGetError();
  switch(err) {
  case GL_NO_ERROR:
    return;
  case GL_INVALID_ENUM:
    fprintf(stderr,"Error %d: Invalid Enum\n",err);
    return;
  case GL_INVALID_VALUE:
    fprintf(stderr,"Error %d: Invalid Value\n",err);
    return;
  case GL_INVALID_OPERATION:
    fprintf(stderr,"Error %d: Invalid Operation\n",err);
    return;
  case GL_STACK_OVERFLOW:
    fprintf(stderr,"Error %d: Stack Overflow\n",err);
    return;
  case GL_OUT_OF_MEMORY:
    fprintf(stderr,"Error %d: Out of Memory\n",err);
    return;
  case GL_TABLE_TOO_LARGE:
    fprintf(stderr,"Error %d: Table Too Large\n",err);
    return;
  default:
    fprintf(stderr,"Error %d: Unknown Error\n",err);
  }
}

int main(int argc, char* argv[])
{
  char mode[MAX_STRLEN];

  FILE *settings;
  IplImage* input;
  GLenum err;
  int i;
  char *ret=0;

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background

  memset(mask_image_fns,0,MAX_IMAGES);
  memset(screen_image_fns,0,MAX_IMAGES);
  memset(mask_image_pinhole_fns,0,MAX_IMAGES);
  memset(screen_image_pinhole_fns,0,MAX_IMAGES);

  /* read saved offsets from file */
  settings = fopen("shifts.txt","r");
  if(settings != NULL) {
    fscanf(settings,"%f %f\n%f %f\n",&m1h, &m1v, &m2h, &m2v);
    printf("read shifts.txt: %f %f %f %f\n",m1h, m1v, m2h, m2v);
    fclose(settings);
  }

  settings = fopen("mul.txt","r");
  if(settings != NULL) {
    fscanf(settings,"%f",&immul);
    printf("read mul.txt: %f\n",immul);
    fclose(settings);
  }

  /* read display settings from file */

  settings = fopen("size.dat","r");
  if(settings == NULL) {
    fprintf(stderr,"error reading size.dat\n");
    exit(1);
  }
  fread(mode,MAX_STRLEN,1,settings);
  fclose(settings);

  /* read in texture file names */
  settings = fopen("mask_H_NMF.txt","r");
  if(settings == NULL) {
    fprintf(stderr,"error reading mask_H_NMF.txt\n");
    exit(1);
  }
  i = 0;
  do {
    mask_image_fns[i] = (char*)malloc(sizeof(char)*MAX_STRLEN);
    if(mask_image_fns[i] == NULL) {
      fprintf(stderr,"malloc failed\n");
      exit(1);
    }
    ret = fgets(mask_image_fns[i],MAX_STRLEN,settings);
    i+=(ret != NULL);
  } while(ret != NULL && i<MAX_IMAGES);
  free(mask_image_fns[i+1]);
  mask_image_fns[i+1]=0;
  fclose(settings);

  n_mask_images = i;

  settings = fopen("screen_W_NMF.txt","r");
  if(settings == NULL) {
    fprintf(stderr,"error reading screen_W_NMF.txt\n");
    exit(1);
  }
  i = 0;
  do {
    screen_image_fns[i] = (char*)malloc(sizeof(char)*MAX_STRLEN);
    if(screen_image_fns[i] == NULL) {
      fprintf(stderr,"malloc failed\n");
      exit(1);
    }
    ret = fgets(screen_image_fns[i],MAX_STRLEN,settings);
    i+=(ret != NULL);
  } while(ret != NULL && i<MAX_IMAGES);
  free(screen_image_fns[i+1]);
  screen_image_fns[i+1]=0;
  fclose(settings);

  n_screen_images = i;

  printf("%d mask images\n",n_mask_images);
  for(i = 0; i < n_mask_images; i++){
    ret = strchr(mask_image_fns[i],'\n');
    *ret=0;
    printf("\t%s\n",mask_image_fns[i]);
  }
  printf("%d screen images\n",n_screen_images);
  for(i = 0; i < n_screen_images; i++){
    ret = strchr(screen_image_fns[i],'\n');
    *ret=0;
    printf("\t%s\n",screen_image_fns[i]);
  }

  /* read in pinhole texture file names */
  settings = fopen("mask_H_pinhole.txt","r");
  if(settings == NULL) {
    fprintf(stderr,"error reading mask_H_pinhole.txt\n");
    exit(1);
  }
  i = 0;
  do {
    mask_image_pinhole_fns[i] = (char*)malloc(sizeof(char)*MAX_STRLEN);
    if(mask_image_pinhole_fns[i] == NULL) {
      fprintf(stderr,"malloc failed\n");
      exit(1);
    }
    ret = fgets(mask_image_pinhole_fns[i],MAX_STRLEN,settings);
    i+=(ret != NULL);
  } while(ret != NULL && i<MAX_IMAGES);
  free(mask_image_pinhole_fns[i+1]);
  mask_image_pinhole_fns[i+1]=0;
  fclose(settings);

  n_mask_images_pinhole = i;

  settings = fopen("screen_W_pinhole.txt","r");
  if(settings == NULL) {
    fprintf(stderr,"error reading screen_W_pinhole.txt\n");
    exit(1);
  }
  i = 0;
  do {
    screen_image_pinhole_fns[i] = (char*)malloc(sizeof(char)*MAX_STRLEN);
    if(screen_image_pinhole_fns[i] == NULL) {
      fprintf(stderr,"malloc failed\n");
      exit(1);
    }
    ret = fgets(screen_image_pinhole_fns[i],MAX_STRLEN,settings);
    i+=(ret != NULL);
  } while(ret != NULL && i<MAX_IMAGES);
  free(screen_image_pinhole_fns[i+1]);
  screen_image_pinhole_fns[i+1]=0;
  fclose(settings);

  n_screen_images_pinhole = i;

  printf("%d mask images pinhole\n",n_mask_images_pinhole);
  for(i = 0; i < n_mask_images_pinhole; i++){
    ret = strchr(mask_image_pinhole_fns[i],'\n');
    *ret=0;
    printf("\t%s\n",mask_image_pinhole_fns[i]);
  }
  printf("%d screen images pinhole\n",n_screen_images_pinhole);
  for(i = 0; i < n_screen_images_pinhole; i++){
    ret = strchr(screen_image_pinhole_fns[i],'\n');
    *ret=0;
    printf("\t%s\n",screen_image_pinhole_fns[i]);
  }


  /* allocate textures */

  textures = (GLuint*)malloc(sizeof(GLint)*(n_screen_images+n_mask_images+n_screen_images_pinhole+n_mask_images_pinhole+1));
  if(textures == NULL) {
    fprintf(stderr,"malloc failed\n");
    exit(1);
  }

  ntex = n_screen_images+n_mask_images+n_screen_images_pinhole+n_mask_images_pinhole+1;

  /* enter game mode (fullscreen) */

  glutGameModeString(mode);
  
  if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) {
    glutEnterGameMode();
  } else {
    printf("no suitable game mode resolution found: %s\n",mode);
    exit(1);
  }
  
  glutSetCursor(GLUT_CURSOR_NONE);
  
  if (glutGameModeGet(GLUT_GAME_MODE_ACTIVE) == 0)
    printf("Current Mode: Window\n");
  else
    printf("Current Mode: Game Mode %dx%d at %d hertz, %d bpp\n",
	   glutGameModeGet(GLUT_GAME_MODE_WIDTH),
	   glutGameModeGet(GLUT_GAME_MODE_HEIGHT),
	   glutGameModeGet(GLUT_GAME_MODE_REFRESH_RATE),
	   glutGameModeGet(GLUT_GAME_MODE_PIXEL_DEPTH));


  /* generate textures */

  glGenTextures( ntex, textures );

  for(i=0; i<n_mask_images; i++) {
    glBindTexture ( GL_TEXTURE_RECTANGLE_NV, textures[i] );
    input = cvLoadImage(mask_image_fns[i], CV_LOAD_IMAGE_COLOR);
    if(input == NULL) {
      fprintf(stderr,"Cannot load %s\n",mask_image_fns[i]);
      exit(1);
    }
    printf("%s bound to %d\n",mask_image_fns[i],i);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
    printGLErr();
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    cvReleaseImage(&input);
  }
  
  for(i=0; i<n_screen_images; i++) {
    glBindTexture ( GL_TEXTURE_RECTANGLE_NV, textures[i+n_mask_images] );
    input = cvLoadImage(screen_image_fns[i], CV_LOAD_IMAGE_COLOR);
    if(input == NULL) {
      fprintf(stderr,"Cannot load %s\n",screen_image_fns[i]);
      exit(1);
    }
    for(unsigned int h=0;h<(input->width)*(input->height)*3;h++) {
      double blah = (unsigned char)input->imageData[h];
      blah *= immul;
      if(blah>255.0) blah=255.0;
      if(blah<0.0) blah=0.0;
      input->imageData[h]=(unsigned char)blah;
    }
    printf("%s bound to %d\n",screen_image_fns[i],i+n_mask_images);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
    printGLErr();
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    cvReleaseImage(&input);
  }
  /**************************************/
  for(i=0; i<n_mask_images_pinhole; i++) {
    glBindTexture ( GL_TEXTURE_RECTANGLE_NV, textures[i]+n_mask_images+n_screen_images );
    input = cvLoadImage(mask_image_pinhole_fns[i], CV_LOAD_IMAGE_COLOR);
    if(input == NULL) {
      fprintf(stderr,"Cannot load %s\n",mask_image_pinhole_fns[i]);
      exit(1);
    }
    printf("%s bound to %d\n",mask_image_pinhole_fns[i],i+n_mask_images+n_screen_images);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
    printGLErr();
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    cvReleaseImage(&input);
  }
  
  for(i=0; i<n_screen_images_pinhole; i++) {
    glBindTexture ( GL_TEXTURE_RECTANGLE_NV, textures[i+n_mask_images+n_screen_images+n_mask_images_pinhole] );
    input = cvLoadImage(screen_image_pinhole_fns[i], CV_LOAD_IMAGE_COLOR);
    if(input == NULL) {
      fprintf(stderr,"Cannot load %s\n",screen_image_pinhole_fns[i]);
      exit(1);
    }
    printf("%s bound to %d\n",screen_image_pinhole_fns[i],i+n_mask_images+n_screen_images+n_mask_images_pinhole);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
    printGLErr();
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    cvReleaseImage(&input);
  }

  /* make white texture */
  char tex[4] = {0,0,0,0};
  glBindTexture(GL_TEXTURE_2D,textures[ntex-1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB4, 2, 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, &tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glMatrixMode(GL_PROJECTION); 
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW); 
  glOrtho(0,glutGameModeGet(GLUT_GAME_MODE_WIDTH), 0, glutGameModeGet(GLUT_GAME_MODE_HEIGHT), -0.1, 0.1);
  glLoadIdentity(); 
  
  glutDisplayFunc(onRender);
  glutTimerFunc(8,videoTimer,0);
  glutKeyboardFunc(onKeyDown);
  
  requestSynchornizedSwapBuffers();
  
  glutMainLoop();
  
  return 0;
}

static void requestSynchornizedSwapBuffers(void)
{
  PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI =
    (PFNGLXSWAPINTERVALSGIPROC) glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI");
  if (glXSwapIntervalSGI) {
    glXSwapIntervalSGI(1);
  } else {
    fprintf(stderr,"Failed to set vsync\n");
    exit(1);
  }
}
