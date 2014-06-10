// flip.cpp : Defines the entry point for the console application.
//
// g++ -lGLU -lglut -lhighgui -I/usr/include/nvidia -I/usr/include -I/usr/include/opencv flip-compare.cpp -o flip-compare

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <GL/glut.h>
#include <opencv/highgui.h>
#include <nvidia/GL/gl.h>
#include <nvidia/GL/glx.h>
#include <nvidia/GL/glext.h>
#ifndef GLX_SGI_swap_control
typedef int ( * PFNGLXSWAPINTERVALSGIPROC) (int interval);
#endif

GLuint *textures;
const unsigned int MAX_IMAGES=1200;
const unsigned int MAX_STRLEN=256;
char *mask_image_fns[MAX_IMAGES];
char *screen_image_fns[MAX_IMAGES];
char *mask_image_comp_fns[MAX_IMAGES];
char *screen_image_comp_fns[MAX_IMAGES];
unsigned int n_mask_images;
unsigned int n_screen_images;
unsigned int n_mask_images_comp;
unsigned int n_screen_images_comp;
GLfloat m1v=0.0f;
GLfloat m1h=0.0f;
GLfloat m2v=0.0f;
GLfloat m2h=0.0f;
char swap=1;
unsigned int ntex=0;
float immul=0.0f;
int texture_height=0;
int texture_width=0;
bool comp=false;

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
	static int which_mask=0;
	static int which_screen=0;
	static int switch_interval;
	static int a=0;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_RECTANGLE_NV);
	
	if(!a) {
	  glBindTexture(GL_TEXTURE_RECTANGLE_NV, textures[which_mask]);
	  //printf("%d\n",which_mask);
	} else {
	  glBindTexture(GL_TEXTURE_RECTANGLE_NV, textures[n_mask_images + n_screen_images + which_mask]);
	  //printf("%d\n",n_mask_images + n_screen_images + which_mask);
	}

	glBegin(GL_QUADS);

	glTexCoord2i(0, texture_height);
	glVertex2f(-1.0f+m1h+(float)swap, -1.0f+m1v); 

	glTexCoord2i(texture_width,texture_height);
	glVertex2f( 0.0f+m1h+(float)swap, -1.0f+m1v);
	
	glTexCoord2i(texture_width,0); 
	glVertex2f( 0.0f+m1h+(float)swap, 1.0f+m1v); 

	glTexCoord2i(0, 0); 
	glVertex2f(-1.0f+m1h+(float)swap, 1.0f+m1v);

	glEnd();
	if(!a) {
	  glBindTexture(GL_TEXTURE_RECTANGLE_NV, textures[which_screen+n_mask_images]);
	  //printf("%d\n",which_screen+n_mask_images);
	} else {
	  glBindTexture(GL_TEXTURE_RECTANGLE_NV, textures[which_screen+n_mask_images+n_screen_images+n_mask_images_comp]);
	  //printf("%d\n",which_screen+n_mask_images+n_screen_images+n_mask_images_comp);
	}

	glBegin(GL_QUADS);
	
	glTexCoord2i(0, texture_height);
	glVertex2f(0.0f+m2h-(float)swap, -1.0f+m2v); 
	
	glTexCoord2i(texture_width, texture_height);
	glVertex2f( 1.0f+m2h-(float)swap, -1.0f+m2v); 
	
	glTexCoord2i(texture_width, 0); 
	glVertex2f( 1.0f+m2h-(float)swap, 1.0f+m2v); 
	
	glTexCoord2i(0, 0); 
	glVertex2f(0.0f+m2h-(float)swap, 1.0f+m2v);
	
	glEnd();

	glDisable(GL_TEXTURE_RECTANGLE_NV);

	glFlush();
	glutSwapBuffers();

	if(comp) {
	  if(switch_interval++>60) {
	    a=!a;
	    switch_interval = 0;
	  }	  
	}
	if(a) {
	  if((++which_mask)   >= (n_mask_images_comp)) which_mask = 0;
	  if((++which_screen) >= (n_screen_images_comp)) which_screen = 0;
	} else {
	  if((++which_mask)   >= (n_mask_images)) which_mask = 0;
	  if((++which_screen) >= (n_screen_images)) which_screen = 0;
	}

	//printf("mask: %d, screen: %d\n",which_mask, which_screen);

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

int dotfilter(const struct dirent *ent) {
  if(ent->d_name[0] == '.') return 0;
  return 1;
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

  IplImage* input;
  GLenum err;
  int i;
  char *ret=0;
  FILE *settings;
  struct dirent **dp;

  char flip_dir[MAX_STRLEN];
  char flip_comp[MAX_STRLEN];

  for(i=0;i<argc;i++) {
    if(!strcmp(argv[i],"-dir")) {
      strncpy(flip_dir,argv[i+1],MAX_STRLEN);
    }

    if(!strcmp(argv[i],"-comp")) {
      strncpy(flip_comp,argv[i+1],MAX_STRLEN);
      comp = true;
    }
    

  }

  if(!strcmp(flip_dir,"")) {
    snprintf(flip_dir,MAX_STRLEN,"NMF");
  }

  if(!strcmp(flip_comp,"")) {
    comp = false;
  }

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background

  memset(mask_image_fns,0,MAX_IMAGES);
  memset(screen_image_fns,0,MAX_IMAGES);

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

  /* read in texture file names from NMF directory */

  char thisdir[MAX_STRLEN];

  snprintf(thisdir,MAX_STRLEN,"%s/H",flip_dir);
  printf("Reading directory: %s\n",thisdir);

  i = scandir(thisdir,&dp,dotfilter,alphasort);
  if (i<0) {
    perror("scandir");
    exit(1);
  } else if (i>MAX_IMAGES) {
    fprintf(stderr,"Too many images in %s",thisdir);
    exit(1);
  } else {
    n_mask_images = i;
    while(i--) {
      int len = strlen(dp[i]->d_name) + strlen(thisdir) + 2;
      mask_image_fns[i] = (char*)malloc(sizeof(char)*len);
      if(mask_image_fns[i] == NULL) {
	fprintf(stderr,"malloc failed\n");
	exit(1);
      }
      snprintf(mask_image_fns[i],len,"%s/%s",thisdir,dp[i]->d_name);
      free(dp[i]);
    }
  }
  free(dp);

  snprintf(thisdir,MAX_STRLEN,"%s/W",flip_dir);
  printf("Reading directory: %s\n",thisdir);

  i = scandir(thisdir,&dp,dotfilter,alphasort);
  if (i<0) {
    perror("scandir");
    exit(1);
  } else if (i>MAX_IMAGES) {
    fprintf(stderr,"Too many images in %s",thisdir);
    exit(1);
  } else {
    n_screen_images = i;
    while(i--) {
      int len = strlen(dp[i]->d_name) + strlen(thisdir) + 2;
      screen_image_fns[i] = (char*)malloc(sizeof(char)*len);
      if(screen_image_fns[i] == NULL) {
	fprintf(stderr,"malloc failed\n");
	exit(1);
      }
      snprintf(screen_image_fns[i],len,"%s/%s",thisdir,dp[i]->d_name);
      free(dp[i]);
    }
  }
  free(dp);

  if(comp) {
    snprintf(thisdir,MAX_STRLEN,"%s/H",flip_comp);
    printf("Reading directory: %s\n",thisdir);
    
    i = scandir(thisdir,&dp,dotfilter,alphasort);
    if (i<0) {
      perror("scandir");
      exit(1);
    } else if (i>MAX_IMAGES) {
      fprintf(stderr,"Too many images in %s",thisdir);
      exit(1);
    } else {
      n_mask_images_comp = i;
      while(i--) {
	int len = strlen(dp[i]->d_name) + strlen(thisdir) + 2;
	mask_image_comp_fns[i] = (char*)malloc(sizeof(char)*len);
	if(mask_image_comp_fns[i] == NULL) {
	  fprintf(stderr,"malloc failed\n");
	  exit(1);
	}
	snprintf(mask_image_comp_fns[i],len,"%s/%s",thisdir,dp[i]->d_name);
	free(dp[i]);
      }
    }
    free(dp);

    snprintf(thisdir,MAX_STRLEN,"%s/W",flip_comp);
    printf("Reading directory: %s\n",thisdir);

    i = scandir(thisdir,&dp,dotfilter,alphasort);
    if (i<0) {
      perror("scandir");
      exit(1);
    } else if (i>MAX_IMAGES) {
      fprintf(stderr,"Too many images in %s",thisdir);
      exit(1);
    } else {
      n_screen_images_comp = i;
      while(i--) {
	int len = strlen(dp[i]->d_name) + strlen(thisdir) + 2;
	screen_image_comp_fns[i] = (char*)malloc(sizeof(char)*len);
	if(screen_image_comp_fns[i] == NULL) {
	  fprintf(stderr,"malloc failed\n");
	  exit(1);
	}
	snprintf(screen_image_comp_fns[i],len,"%s/%s",thisdir,dp[i]->d_name);
	free(dp[i]);
      }
    }
    free(dp);
    
  }

  printf("%d mask images\n",n_mask_images);
  for(i = 0; i < n_mask_images; i++){
    printf("\t%s\n",mask_image_fns[i]);
  }
  printf("%d screen images\n",n_screen_images);
  for(i = 0; i < n_screen_images; i++){
    printf("\t%s\n",screen_image_fns[i]);
  }
  if(comp) {
    printf("%d mask images comp\n",n_mask_images_comp);
    for(i = 0; i < n_mask_images_comp; i++){
      printf("\t%s\n",mask_image_comp_fns[i]);
    }
    printf("%d screen images comp\n",n_screen_images_comp);
    for(i = 0; i < n_screen_images_comp; i++){
      printf("\t%s\n",screen_image_comp_fns[i]);
    }
  }


  /* allocate textures */

  if(!comp) {
    textures = (GLuint*)malloc(sizeof(GLint)*(n_screen_images+n_mask_images+1));
    if(textures == NULL) {
      fprintf(stderr,"malloc failed\n");
      exit(1);
    }
    ntex = n_screen_images+n_mask_images;
    printf("%d textures total.\n",ntex);
  } else { /* comp */
    textures = (GLuint*)malloc(sizeof(GLint)*(n_screen_images+n_mask_images+n_mask_images_comp+n_mask_images_comp+1));
    if(textures == NULL) {
      fprintf(stderr,"malloc failed\n");
      exit(1);
    }
    ntex = n_screen_images+n_mask_images+n_screen_images_comp+n_mask_images_comp;
    printf("%d textures total.\n",ntex);
  }


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

  printGLErr();
  
  for(i=0; i<n_mask_images; i++) {
    glBindTexture ( GL_TEXTURE_RECTANGLE_NV, textures[i] );
    input = cvLoadImage(mask_image_fns[i], CV_LOAD_IMAGE_COLOR);
    if(input == NULL) {
      fprintf(stderr,"Cannot load %s\n",mask_image_fns[i]);
      exit(1);
    }
    printf("%s bound to %d\n",mask_image_fns[i],i);
    if(input->width != texture_width || input->height != texture_height) {
	if(texture_width==0 && texture_height==0) {
		printf("Setting texture width/height to: %d x %d\n",input->width,input->height);
	} else {
		printf("Warning: texture width/height changed to: %d x %d\n",input->width, input->height);
        }
	texture_width = input->width;
	texture_height = input->height;
    }
    free(mask_image_fns[i]);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
    printGLErr();
    glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
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
    if(input->width != texture_width || input->height != texture_height) {
	if(texture_width==0 && texture_height==0) {
		printf("Setting texture width/height to: %d x %d\n",input->width,input->height);
	} else {
		printf("Warning: texture width/height changed to: %d x %d\n",input->width, input->height);
        }
	texture_width = input->width;
	texture_height = input->height;
    }
    free(screen_image_fns[i]);
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
    printGLErr();
    glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
    cvReleaseImage(&input);
  }

  int a = n_screen_images + n_mask_images;

  if(comp) {
    for(i=0; i<n_mask_images_comp; i++) {
      glBindTexture ( GL_TEXTURE_RECTANGLE_NV, textures[a+i] );
      input = cvLoadImage(mask_image_comp_fns[i], CV_LOAD_IMAGE_COLOR);
      if(input == NULL) {
	fprintf(stderr,"Cannot load %s\n",mask_image_comp_fns[i]);
	exit(1);
      }
      printf("%s bound to %d\n",mask_image_comp_fns[i],a+i);
      if(input->width != texture_width || input->height != texture_height) {
	if(texture_width==0 && texture_height==0) {
	  printf("Setting texture width/height to: %d x %d\n",input->width,input->height);
	} else {
	  printf("Warning: texture width/height changed to: %d x %d\n",input->width, input->height);
        }
	texture_width = input->width;
	texture_height = input->height;
      }
      free(mask_image_comp_fns[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
      printGLErr();
      glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
      cvReleaseImage(&input);
    }
    
    for(i=0; i<n_screen_images_comp; i++) {
      glBindTexture ( GL_TEXTURE_RECTANGLE_NV, textures[a+i+n_mask_images_comp] );
      input = cvLoadImage(screen_image_comp_fns[i], CV_LOAD_IMAGE_COLOR);
      if(input == NULL) {
	fprintf(stderr,"Cannot load %s\n",screen_image_comp_fns[i]);
	exit(1);
      }
      for(unsigned int h=0;h<(input->width)*(input->height)*3;h++) {
	double blah = (unsigned char)input->imageData[h];
	blah *= immul;
	if(blah>255.0) blah=255.0;
	if(blah<0.0) blah=0.0;
	input->imageData[h]=(unsigned char)blah;
      }
      printf("%s bound to %d\n",screen_image_comp_fns[i],a+i+n_mask_images_comp);
      if(input->width != texture_width || input->height != texture_height) {
	if(texture_width==0 && texture_height==0) {
	  printf("Setting texture width/height to: %d x %d\n",input->width,input->height);
	} else {
	  printf("Warning: texture width/height changed to: %d x %d\n",input->width, input->height);
        }
	texture_width = input->width;
	texture_height = input->height;
      }
      free(screen_image_comp_fns[i]);
      glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB4, input->width, input->height, 0, GL_BGR, GL_UNSIGNED_BYTE, input->imageData);
      printGLErr();
      glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
      glTexParameteri ( GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
      glTexEnvi ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
      cvReleaseImage(&input);
    }
  }

  glMatrixMode(GL_PROJECTION); 
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW); 
  glOrtho(0,glutGameModeGet(GLUT_GAME_MODE_WIDTH), 0, glutGameModeGet(GLUT_GAME_MODE_HEIGHT), -0.1, 0.1);
  glLoadIdentity(); 
  
  glutDisplayFunc(onRender);
  glutTimerFunc(8,videoTimer,0);
  glutKeyboardFunc(onKeyDown);
  
//  requestSynchornizedSwapBuffers();
  
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
