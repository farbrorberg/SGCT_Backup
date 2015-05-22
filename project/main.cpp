#include "sgct.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <iterator>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>

//For the time function
#include <time.h>

#include <SpiceUsr.h>
#include <SpiceZfc.h>

#include "model.hpp"
#include "shadow.hpp"
#include "shader.hpp"

sgct::Engine * gEngine;

void myInitOGLFun();
//      |
//      V
void myPreSyncFun();//<---------------------------------┐
//      |                                               |
//      V                                               |
void myPostSyncPreDrawFun(); //                         |
//      |                                               |
//      V                                               |
void myDrawFun();//                                     |
//      |                                               |
//      V                                               |
void myEncodeFun();//                                   |
//      |                                               |
//      V                                               |
void myDecodeFun();//                                   |
//      |                                               |
//      V                                               |
void myCleanUpFun();//                                  |
//      |                                               |
//      V                                               |
void keyCallback(int key, int action);//                |
//      |                                               |
//      V                                               |
void mouseButtonCallback(int button, int action);//     |
//      |                                               ^
//      └-----------------------------------------------┘


/*------------------MOVEMENT------------------*/
float rotationSpeed = 0.1f;
float walkingSpeed = 2.5f;
float runningSpeed = 5.0f;

bool dirButtons[6];
enum directions { FORWARD = 0, BACKWARD, LEFT, RIGHT, UP, DOWN };

bool runningButton = false;
bool mouseLeftButton = false;

double mouseDx = 0.0;
double mouseDy = 0.0;

double mouseXPos[] = { 0.0, 0.0 };
double mouseYPos[] = { 0.0, 0.0 };

glm::vec3 bView(0.0f, 0.0f, 0.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
glm::vec3 pos(0.0f, 0.0f, 0.0f);
/*--------------------------------------------*/

/*------------------REGULAR FUNCTIONS------------------*/
void calcSunPosition(); // Calculates the suns position
void calcSkyColor(float fSunAnglePhi, float &fAmb, glm::vec4 &sColor);
void resetToCurrentTime(); // Used to calculate the time of the current computer
void addSecondToTime();
/*-----------------------------------------------------*/

/*------------------SHADOWMAP------------------*/

//Post Fx shader locations
sgct::PostFX fx;
GLint fxNearLoc = -1;
GLint fxFarLoc = -1;
GLint depthMVP_Loc = -1;
GLint texID_Loc = -1;


void updatePassShadow()
{
//	glActiveTexture(GL_TEXTURE1);
//	glEnable(GL_TEXTURE_2D);
//	glBindTexture(GL_TEXTURE_2D, gEngine->getActiveDepthTexture() );
//	glUniform1i( texID_Loc, 1 );
//	glUniform1f( fxNearLoc, gEngine->getNearClippingPlane() );
//	glUniform1f( fxFarLoc, gEngine->getFarClippingPlane() );
//	glUniformMatrix4fv(depthMVP_Loc, 1, GL_FALSE, glm::value_ptr(nyDepthMVP)); //Hur göra med matriserna (allt ej textur)

}

std::vector<class shadow> buffers;

sgct_core::OffScreenBuffer *myBuffer;
/*---------------------------------------------*/

/*------------------SHADERS------------------*/
//Shader Scene
GLint MVP_Loc = -1;
GLint NM_Loc = -1;
GLint sColor_Loc = -1;
GLint lDir_Loc = -1;
GLint Amb_Loc = -1;
GLint Tex_Loc = -1;

GLint depthBiasMVP_Loc = -1;
GLint shadowmap_Loc = -1;

//Shader Sky
GLint MVP_Loc_S = -1;
GLint NM_Loc_S = -1;
GLint lDir_Loc_S = -1;
GLint Tex_Loc_S = -1;
GLint Glow_Loc_S = -1;
GLint SunColor_Loc_S = -1;
/*------------------------------------------*/

/*------------------SHARED VARIABLES ACROSS THE CLUSTER------------------*/
sgct::SharedDouble curr_time(0.0);
sgct::SharedBool reloadShader(false);
sgct::SharedObject<glm::mat4> xform;
/*-----------------------------------------------------------------------*/

/*------------------GUI------------------*/
void externalControlMessageCallback(const char * receivedChars, int size);
void externalControlStatusCallback(bool connected);

sgct::SharedBool timeIsTicking(true);
sgct::SharedInt timeSpeed = 1;
sgct::SharedString date;
sgct::SharedBool oneSecondPassed(false);
/*---------------------------------------*/

/*---------------OTHER VARIABLES--------------*/
//SUN POSITION
float fSunAnglePhi;
float fSunAngleTheta;
float fAmb = 0.2f; //Initialize to low for debugging purposes
glm::vec4 sColor = glm::vec4(0.4f, 0.4f, 0.4f, 0.4f); //Initialize to low for debugging purposes

//TIME
enum timeVariables{YEAR = 0, MONTH = 1, DAY = 2, HOUR = 3, MINUTE = 4, SECOND = 5};
int currentTime[6];
int timeCount = 0;

//OBJECTS
model landscape;
model box;
model sun;
model skyDome;

std::vector<model> objects;

glm::mat4 transforms;
glm::mat4 nyDepthMVP;
glm::mat4 nyMVP;
/*------------------------------------------------*/

int main( int argc, char* argv[] ){
    gEngine = new sgct::Engine( argc, argv );

    gEngine->setInitOGLFunction( myInitOGLFun );
    gEngine->setPreSyncFunction( myPreSyncFun );
    gEngine->setPostSyncPreDrawFunction( myPostSyncPreDrawFun );
    gEngine->setDrawFunction( myDrawFun );
    gEngine->setCleanUpFunction( myCleanUpFun );
    gEngine->setKeyboardCallbackFunction( keyCallback );
    gEngine->setMouseButtonCallbackFunction( mouseButtonCallback );
    gEngine->setExternalControlCallback( externalControlMessageCallback );
    gEngine->setExternalControlStatusCallback( externalControlStatusCallback );

    /*------------------GUI------------------*/
    sgct::SharedData::instance()->setEncodeFunction(myEncodeFun);
    sgct::SharedData::instance()->setDecodeFunction(myDecodeFun);
    /*-----------------------------------------*/

    /*------------------SPICE-------------------*/
    /*      Kernels needed for calculations     */
    furnsh_c( "kernels/naif0011.tls" ); //Is a generic kernel that you can use to get the positions of Earth and the Sun for various times
    furnsh_c( "kernels/de430.bsp" );    //Is a leapsecond kernel so that you get the accurate times
    furnsh_c( "kernels/pck00010.tpc" ); //Might also be needed
    /*-----------------------------------------*/

    for(int i=0; i<6; i++)
        dirButtons[i] = false;

    //SHADOWMAP
    sgct::SGCTSettings::instance()->setUseDepthTexture(true);
    sgct::SGCTSettings::instance()->setUseFBO(true);
    myBuffer = new sgct_core::OffScreenBuffer;

#if __APPLE__
    if( !gEngine->init(sgct::Engine::OpenGL_3_3_Core_Profile ) ){
        delete gEngine;
        return EXIT_FAILURE;
    }

#elif (_MSC_VER >= 1500)
    if( !gEngine->init(sgct::Engine::OpenGL_3_3_Core_Profile ) ){
        delete gEngine;
        return EXIT_FAILURE;
    }

#elif __WIN32__
    if( !gEngine->init(sgct::Engine::OpenGL_3_3_Core_Profile ) ){
        delete gEngine;
        return EXIT_FAILURE;
    }

#elif __linux__
    if( !gEngine->init( ) ){
        delete gEngine;
        return EXIT_FAILURE;
    }
#endif
    
    resetToCurrentTime();

    // Main loop
    gEngine->render();

    // Clean up
    delete gEngine;

    // Exit program
    exit( EXIT_SUCCESS );

    return( 0 );
}

void myInitOGLFun(){
    sgct::TextureManager::instance()->setWarpingMode(GL_REPEAT, GL_REPEAT);
    sgct::TextureManager::instance()->setAnisotropicFilterSize(8.0f);
    sgct::TextureManager::instance()->setCompression(sgct::TextureManager::S3TC_DXT);

    /*----------------OBJECTS AND TEXTURES--------------*/
    // OBJECTS TO SKY
    sgct::TextureManager::instance()->loadTexure("sun", "texture/sun.jpg", true);
    sun.createSphere(10.0f, 80);

    skyDome.createSphere(5.0f, 100);
    int x, y =0;
    gEngine->getActiveViewportSize(x, y);
    sgct_utils::SGCTDome* newDome = new sgct_utils::SGCTDome(500, x/y, 100, 20, 0.2f);

    // OBJECTS TO SCENE
    //Transformations from origo. ORDER MATTERS!
    landscape.readOBJ("mesh/landscape2.obj", "texture/landscape2.png");
    landscape.translate(0.0f, -20.0f, 0.0f);
    landscape.scale(1.0f, 1.0f, 1.0f);
    objects.push_back(landscape);

    box.readOBJ("mesh/box.obj", "texture/box.png");
    box.translate(0.0f, 0.0f, -5.0f);
    box.scale(2.0f, 2.0f, 2.0f);
    //objects.push_back(box);

    /*----------------------------------------------------------*/

    /*------------------------SHADOWMAP-------------------------*/

	sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
	for(unsigned int i=0; i < thisNode->getNumberOfWindows(); i++)
	{
		class shadow tmpBuffer;
		buffers.push_back( tmpBuffer );
	}
	sgct::MessageHandler::instance()->print("Number of buffers: %d\n", buffers.size());

	for(unsigned int i=0; i < buffers.size(); i++)
	{
        GLint fb_width, fb_height = 0;
        sgct::SGCTWindow * winPtr = gEngine->getWindowPtr(i);
		winPtr->getDrawFBODimensions(fb_width, fb_height);
        buffers[i].createFBOs(gEngine, fb_width, fb_height);

        //myBuffer->createFBO(fb_width, fb_height);
        //myBuffer->attachDepthTexture(buffers[i].shadowTexture);
        //winPtr->getFrameBufferTexture(i); //Använda denna istället?
        buffers[i].initPrintMap();
    }

	//Initialize Shader depthShadowmap
    sgct::ShaderManager::instance()->addShaderProgram( "depthShadowmap", "shaders/depthShadow.vert", "shaders/depthShadow.frag" );
    sgct::ShaderManager::instance()->bindShaderProgram( "depthShadowmap" );

    depthMVP_Loc = sgct::ShaderManager::instance()->getShaderProgram( "depthShadowmap").getUniformLocation( "depthMVP" );
    texID_Loc = sgct::ShaderManager::instance()->getShaderProgram( "depthShadowmap").getUniformLocation( "shadowMap" );
    glUniform1i( texID_Loc, 0 );

    fxNearLoc = sgct::ShaderManager::instance()->getShaderProgram( "depthShadowmap").getUniformLocation( "near" );
    fxFarLoc = sgct::ShaderManager::instance()->getShaderProgram( "depthShadowmap").getUniformLocation( "far" );

    sgct::ShaderManager::instance()->unBindShaderProgram();

//    sgct::ShaderProgram * sp;
//
//	fx.init("depthShadowmap", "shaders/depthShadow.vert", "shaders/depthShadow.frag");
//	fx.setUpdateUniformsFunction( updatePassShadow );
//	sp = fx.getShaderProgram();
//	sp->bind();
//        depthMVP_Loc = sp->getUniformLocation( "depthMVP ");
//        texID_Loc = sp->getUniformLocation( "shadowMap" );
//		fxCTexLoc = sp->getUniformLocation( "cTex" );
//		fxDTexLoc = sp->getUniformLocation( "dTex" );
//		fxNearLoc = sp->getUniformLocation( "near" );
//		fxFarLoc = sp->getUniformLocation( "far" );
//	sp->unbind();
//	gEngine->addPostFX( fx );
//
//	//if( gEngine->getNumberOfWindows() > 1 )
//	//	gEngine->getWindowPtr(1)->setUsePostFX( false );

    /*-----------------------------------------------------------*/

    /*---------------------SHADERS-----------------------*/
    
    //Initialize Shader scene
    sgct::ShaderManager::instance()->addShaderProgram( "scene", "shaders/scene.vert", "shaders/scene.frag" );
    sgct::ShaderManager::instance()->bindShaderProgram( "scene" );

    MVP_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "MVP" );
    NM_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "NM" );
    sColor_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "sunColor" );
    lDir_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "lightDir" );
    Amb_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "fAmbInt" );
    Tex_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "Tex" );
    glUniform1i( Tex_Loc, 0 );
    depthBiasMVP_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "depthBiasMVP" );
    shadowmap_Loc = sgct::ShaderManager::instance()->getShaderProgram( "scene").getUniformLocation( "shadowMap" );
    glUniform1i( shadowmap_Loc, 1 );

    sgct::ShaderManager::instance()->unBindShaderProgram();


    //Initialize Shader sky (sky)
    sgct::ShaderManager::instance()->addShaderProgram( "sky", "shaders/sky.vert", "shaders/sky.frag" );
    sgct::ShaderManager::instance()->bindShaderProgram( "sky" );

    MVP_Loc_S = sgct::ShaderManager::instance()->getShaderProgram( "sky").getUniformLocation( "MVP" );
    NM_Loc_S = sgct::ShaderManager::instance()->getShaderProgram( "sky").getUniformLocation( "NM" );
    lDir_Loc_S = sgct::ShaderManager::instance()->getShaderProgram( "sky").getUniformLocation( "lightDir" );
    Tex_Loc_S = sgct::ShaderManager::instance()->getShaderProgram( "sky").getUniformLocation( "Tex" );
    Glow_Loc_S = sgct::ShaderManager::instance()->getShaderProgram( "sky").getUniformLocation( "glow" );
    SunColor_Loc_S = sgct::ShaderManager::instance()->getShaderProgram( "sky").getUniformLocation( "colorSky" );
    glUniform1i( Tex_Loc_S, 0 );
    glUniform1i( SunColor_Loc_S, 1 );
    glUniform1i( Glow_Loc_S, 2 );

    sgct::ShaderManager::instance()->unBindShaderProgram();
    
    /*---------------------------------------------------------*/
}


void myPreSyncFun(){
    if( gEngine->isMaster() ){
        curr_time.setVal( sgct::Engine::getTime() ); //Används ej för tillfället?

        if( mouseLeftButton ){
            //get the mouse pos from first window
            sgct::Engine::getMousePos( gEngine->getFocusedWindowIndex(), &mouseXPos[0], &mouseYPos[0] );
            mouseDx = mouseXPos[0] - mouseXPos[1];
            mouseDy = mouseYPos[0] - mouseYPos[1];
        }
        else{
            mouseDy = 0.0;
            mouseDx = 0.0;
        }

        //MOUSE AND KEYBOARD INPUT
        static float panRot = 0.0f;
        panRot += (static_cast<float>(mouseDx) * rotationSpeed * static_cast<float>(gEngine->getDt()));

        static float tiltRot = 0.0f;
        tiltRot += (static_cast<float>(mouseDy) * rotationSpeed * static_cast<float>(gEngine->getDt()));

        glm::mat4 ViewRotateX = glm::rotate(
                                            glm::mat4(1.0f),
                                            panRot,
                                            glm::vec3(0.0f, 1.0f, 0.0f)); //rotation around the y-axis


        bView = glm::inverse(glm::mat3(ViewRotateX)) * glm::vec3(0.0f, 0.0f, 1.0f);

        glm::vec3 right = glm::cross(bView, up);

        glm::mat4 ViewRotateY = glm::rotate(
                                            glm::mat4(1.0f),
											tiltRot,
											-right); //rotation around the movavble x-axis

        if( dirButtons[FORWARD] ){
            runningButton ? walkingSpeed = runningSpeed: walkingSpeed = 2.5f;
            pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * bView);
        }
        if( dirButtons[BACKWARD] ){
            runningButton ? walkingSpeed = runningSpeed: walkingSpeed = 2.5f;
            pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * bView);
        }
        if( dirButtons[LEFT] ){
            runningButton ? walkingSpeed = runningSpeed: walkingSpeed = 2.5f;
            pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);
        }
        if( dirButtons[RIGHT] ){
            runningButton ? walkingSpeed = runningSpeed: walkingSpeed = 2.5f;
            pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * right);
        }
        if( dirButtons[UP] ){
            runningButton ? walkingSpeed = runningSpeed: walkingSpeed = 2.5f;
            pos -= (walkingSpeed * static_cast<float>(gEngine->getDt()) * up);
        }
        if( dirButtons[DOWN] ){
            runningButton ? walkingSpeed = runningSpeed: walkingSpeed = 2.5f;
            pos += (walkingSpeed * static_cast<float>(gEngine->getDt()) * up);
        }

        glm::mat4 result;
        //4. transform user back to original position
        result = glm::translate( glm::mat4(1.0f), sgct::Engine::getDefaultUserPtr()->getPos() );

        //3. apply view rotation
        result *= ViewRotateX;
        result *= ViewRotateY;

        //2. apply navigation translation
        result *= glm::translate(glm::mat4(1.0f), pos);

        //1. transform user to coordinate system origin
        result *= glm::translate(glm::mat4(1.0f), -sgct::Engine::getDefaultUserPtr()->getPos());

        //0. Translate to eye height of a person
        result *= glm::translate( glm::mat4(1.0f), glm::vec3( 0.0f, -1.6f, 0.0f ) );

        xform.setVal( result );

        //sgct_core::ClusterManager::instance()->getDefaultUserPtr()->setTransform(result);
    }
}

void myPostSyncPreDrawFun(){
    if( timeIsTicking.getVal() && oneSecondPassed.getVal() ){
        std::cout << "Time is ticking" << std::endl;
    }

    else if( !timeIsTicking.getVal() && oneSecondPassed.getVal() ){
        std::cout << "Time is paused" << std::endl;
    }

    if( reloadShader.getVal() )
    {
        //Call shader-reload senare
        sgct::ShaderProgram sp = sgct::ShaderManager::instance()->getShaderProgram( "scene" );
        sp.reload();

        //reset locations
        sp.bind();

        MVP_Loc = sp.getUniformLocation( "MVP" );
        NM_Loc = sp.getUniformLocation( "NM" );
        depthBiasMVP_Loc = sp.getUniformLocation( "depthBiasMVP" );
        sColor_Loc = sp.getUniformLocation("sunColor");
        lDir_Loc = sp.getUniformLocation("lightDir");
        Amb_Loc = sp.getUniformLocation("fAmbInt");
        Tex_Loc = sp.getUniformLocation( "Tex" );
        glUniform1i( Tex_Loc, 0 );
        shadowmap_Loc = sp.getUniformLocation( "shadowMap" );
        glUniform1i( shadowmap_Loc, 1);

        sp.unbind();

        sgct::ShaderProgram skySp = sgct::ShaderManager::instance()->getShaderProgram( "sky" );
        skySp.reload();

        //reset locations
        skySp.bind();

        MVP_Loc_S = skySp.getUniformLocation( "MVP" );
        NM_Loc_S = skySp.getUniformLocation( "NM" );
        Tex_Loc_S = skySp.getUniformLocation( "Tex" );
        lDir_Loc_S = skySp.getUniformLocation("lightDir");
        Glow_Loc_S = skySp.getUniformLocation( "glow" );
        SunColor_Loc_S = skySp.getUniformLocation( "colorSky" );
        glUniform1i( Glow_Loc_S, 2 );
        glUniform1i( SunColor_Loc_S, 1 );
        glUniform1i( Tex_Loc_S, 0 );

        skySp.unbind();

        //fx.getShaderProgram()->reload();

        reloadShader.setVal(false);
    }


    //Fisheye cubemaps are constant size
	sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
	for(unsigned int i=0; i < thisNode->getNumberOfWindows(); i++) {
		if( gEngine->getWindowPtr(i)->isWindowResized() && !gEngine->getWindowPtr(i)->isUsingFisheyeRendering() ){
			buffers[i].resizeFBOs();

			//GLint fb_width, fb_height = 0;
            //sgct::SGCTWindow * winPtr = gEngine->getWindowPtr(i);
            //winPtr->getDrawFBODimensions(fb_width, fb_height);
			//myBuffer->resizeFBO(fb_width, fb_height);
			break;
		}
    }
}

void myDrawFun(){
    ////fuLhaxX

    oneSecondPassed.setVal(false);      // Assume this is false

    if( timeIsTicking.getVal() )        // If time is ticking, add to timecounter
        timeCount++;

    if( timeCount == 60 ){              // 60Hz => this is true once every second
        oneSecondPassed.setVal(true);
        timeCount = 0;
    }
    if( oneSecondPassed.getVal() ){     // If one second has passed

        std::cout << currentTime[YEAR] << " " << currentTime[MONTH] << " " << currentTime[DAY] << " " << currentTime[HOUR] << ":" << currentTime[MINUTE] << ":" << currentTime[SECOND] << std::endl;

        if( timeIsTicking.getVal() ){
            addSecondToTime();
        }
    }
    ///////////

    //create scene transform (animation)
    glm::mat4 scene_mat = xform.getVal();
    gEngine->setNearAndFarClippingPlanes(0.1f, 2000.0f);

    glm::mat4 MV = gEngine->getActiveModelViewMatrix() * scene_mat;
    glm::mat4 MVP = gEngine->getActiveModelViewProjectionMatrix() * scene_mat;
    glm::mat3 NM = glm::inverseTranspose(glm::mat3( MV ));


    /*------------------SUNPOSITION-----------------------*/

    // Set light properties
    float fSunDis = 800;

    calcSunPosition();

    if( oneSecondPassed.getVal() ){
        std::cout<<"THETA: "<< fSunAngleTheta << std::endl;
        std::cout<<"PHI: " << fSunAnglePhi << std::endl;
    }

    glm::vec3 vSunPos(fSunDis*sin(fSunAngleTheta)*cos(fSunAnglePhi),fSunDis*sin(fSunAngleTheta)*sin(fSunAnglePhi),fSunDis*cos(fSunAngleTheta));

    calcSkyColor(fSunAnglePhi, fAmb, sColor);

    glm::vec3 lDir = glm::normalize(vSunPos);

    /*---------------------------------------------*/

    /*------------------SHADOW MAP------------------*/
    
    //get a pointer to the current window
	sgct::SGCTWindow * winPtr = gEngine->getActiveWindowPtr();
	unsigned int index = winPtr->getId();
	winPtr->getFBOPtr()->unBind();
	//myBuffer->bind();

    // Compute the MVP matrix from the light's point of view
    glm::mat4 depthProjectionMatrix = glm::ortho<float>(-100,100,-100,100,-100,200);
    glm::mat4 depthViewMatrix = glm::lookAt(lDir, glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 depthModelMatrix = glm::mat4(1.0);
    //glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
    glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * scene_mat;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //glDepthFunc(GL_ALWAYS);

    const int * coords = gEngine->getActiveViewportPixelCoords();


    for(unsigned int i=0; i < buffers.size(); i++)
	{
        //Bind current framebuffer
        buffers[i].shadowpass();

        //get viewport data and set the viewport
        glViewport( coords[0], coords[1], coords[2], coords[3] );

        //CLear the screen, only depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);

        sgct::ShaderManager::instance()->bindShaderProgram( "depthShadowmap" );


        glUniform1f( fxNearLoc, gEngine->getNearClippingPlane() );
        glUniform1f( fxFarLoc, gEngine->getFarClippingPlane() );


        std::vector<model>::iterator it;
        for(it = objects.begin(); it != objects.end(); ++it)
        {
            nyDepthMVP = depthMVP * (*it).transformations;

            glUniformMatrix4fv(depthMVP_Loc, 1, GL_FALSE, glm::value_ptr(nyDepthMVP));

            (*it).drawToDepthBuffer();

        }

        sgct::ShaderManager::instance()->unBindShaderProgram();

    }
    //Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //myBuffer->unBind();
    winPtr->getFBOPtr()->bind();
    
    /*------------------------------------------------*/

    glViewport( coords[0], coords[1], coords[2], coords[3] );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    //glDisable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    /*------------------SCENE SHADER------------------*/

    //Bind Shader scene
    sgct::ShaderManager::instance()->bindShaderProgram( "scene" );

    glm::mat4 biasMatrix(0.5, 0.0, 0.0, 0.0,    0.0, 0.5, 0.0, 0.0,    0.0, 0.0, 0.5, 0.0,    0.5, 0.5, 0.5, 1.0);
    glm::mat4 depthBiasMVP = biasMatrix*depthMVP;

    glUniformMatrix4fv(MVP_Loc, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix3fv(NM_Loc, 1, GL_FALSE, &NM[0][0]);
    glUniform4fv(sColor_Loc, 1, &sColor[0]);
    glUniform3fv(lDir_Loc, 1, &lDir[0]);
    glUniform1fv(Amb_Loc, 1, &fAmb);
    glUniformMatrix4fv(depthBiasMVP_Loc, 1, GL_FALSE, &depthBiasMVP[0][0]);


    //Render objects
    std::vector<model>::iterator it;
    for(it = objects.begin(); it != objects.end(); ++it)
    {

        nyMVP = MVP * (*it).transformations;
        glUniformMatrix4fv(MVP_Loc, 1, GL_FALSE, glm::value_ptr(nyMVP));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId((*it).mTextureID));
        glUniform1i(Tex_Loc, 0);

        buffers[index].setShadowTex(shadowmap_Loc);

//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_2D, gEngine->getActiveDepthTexture());
//        glUniform1i(shadowmap_Loc, 1);

        (*it).render();
    }

    sgct::ShaderManager::instance()->unBindShaderProgram();

    //Render shadowMap-texturen
    buffers[index].printMap();

    /*----------------------------------------------*/

    /*------------------SKY SHADER------------------*/
    
    //Bind Shader sky
    sgct::ShaderManager::instance()->bindShaderProgram( "sky" );

    glUniformMatrix4fv(MVP_Loc_S, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix3fv(NM_Loc_S, 1, GL_FALSE, &NM[0][0]);
    glUniform3fv(lDir_Loc_S, 1, &lDir[0]);

    //SUN
    nyMVP = MVP;
        //Transformations from origo. ORDER MATTERS!
        nyMVP = glm::translate(nyMVP, vSunPos);

        //Send the transformations, texture and render
        glUniformMatrix4fv(MVP_Loc_S, 1, GL_FALSE, glm::value_ptr(nyMVP));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::instance()->getTextureId("sun"));
        glUniform1i(Tex_Loc, 0);
        sun.render();

/* SKIPPAR DENNA SÅ LÄNGE
    //SKYDOME
    nyMVP = MVP;
        //Transformations from origo. ORDER MATTERS!

        //Send the transformations, texture and render
        glUniformMatrix4fv(MVP_Loc_S, 1, GL_FALSE, glm::value_ptr(nyMVP));
        //glBindTexture(GL_TEXTURE_2D, 0);
        //glUniform1i(Tex_Loc, 0);
        skyDome.render();
*/
    sgct::ShaderManager::instance()->unBindShaderProgram();
   
    /*----------------------------------------------*/
    
    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
}

void myEncodeFun(){
    sgct::SharedData::instance()->writeObj( &xform );
    sgct::SharedData::instance()->writeDouble( &curr_time );
    sgct::SharedData::instance()->writeBool( &reloadShader );

    //GUI
    sgct::SharedData::instance()->writeBool( &timeIsTicking );
    sgct::SharedData::instance()->writeString( &date );
    sgct::SharedData::instance()->writeInt( &timeSpeed );
    sgct::SharedData::instance()->writeBool( &oneSecondPassed );
}

void myDecodeFun(){
    sgct::SharedData::instance()->readObj( &xform );
    sgct::SharedData::instance()->readDouble( &curr_time );
    sgct::SharedData::instance()->readBool( &reloadShader );

    //GUI
    sgct::SharedData::instance()->readBool( &timeIsTicking );
    sgct::SharedData::instance()->readString( &date );
    sgct::SharedData::instance()->readInt( &timeSpeed );
    sgct::SharedData::instance()->readBool( &oneSecondPassed );
}

/*!
	De-allocate data from GPU
	Textures are deleted automatically when using texture manager
	Shaders are deleted automatically when using shader manager
 */
void myCleanUpFun(){
    for(unsigned int i=0; i < buffers.size(); i++)
	{
        buffers[i].clearBuffers();
    }
    buffers.clear();
    delete myBuffer;
    myBuffer = NULL;
}

void keyCallback(int key, int action){
    if( gEngine->isMaster() ){
        switch( key ){
            case SGCT_KEY_R: if(action == SGCT_PRESS) reloadShader.setVal(true); break;
            case SGCT_KEY_W: case SGCT_KEY_UP: dirButtons[FORWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false); break;
            case SGCT_KEY_S: case SGCT_KEY_DOWN:dirButtons[BACKWARD] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false); break;
            case SGCT_KEY_A: case SGCT_KEY_LEFT: dirButtons[LEFT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false); break;
            case SGCT_KEY_D: case SGCT_KEY_RIGHT:dirButtons[RIGHT] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false); break;
 /*Running*/case SGCT_KEY_LEFT_SHIFT: case SGCT_KEY_RIGHT_SHIFT: runningButton = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false); break;
        	case SGCT_KEY_Q: dirButtons[UP] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false); break;
        	case SGCT_KEY_E: dirButtons[DOWN] = ((action == SGCT_REPEAT || action == SGCT_PRESS) ? true : false); break;
        }
    }
}

void mouseButtonCallback(int button, int action){
    if( gEngine->isMaster() ){
        switch( button ) {
            case SGCT_MOUSE_BUTTON_LEFT:
                mouseLeftButton = (action == SGCT_PRESS ? true : false);
                //set refPos
                sgct::Engine::getMousePos(gEngine->getFocusedWindowIndex(), &mouseXPos[1], &mouseYPos[1]);
                break;
        }
    }
}

void externalControlMessageCallback(const char * receivedChars, int size){
    if( gEngine->isMaster() ){
        //PAUSE TIME
        if( strncmp(receivedChars, "pause", 5) == 0 ){
            if( strncmp(receivedChars, "pause=0", 7) == 0 ){
                timeIsTicking.setVal( true );
                //std::cout << "CONTINUE TIME" << std::endl;
            }
            else if( strncmp(receivedChars, "pause=1", 7) == 0 ){
                timeIsTicking.setVal( false );
                //std::cout << "PAUSE TIME" << std::endl;
            }
        }

        //RESET TO CURRENT TIME
        if( size == 7 && strncmp( receivedChars, "reset", 4 ) == 0 ){
            if( strncmp(receivedChars, "reset=1", 7) == 0 ){
                //std::cout << "RESET TO CURRENT TIME" << std::endl;
                resetToCurrentTime();
            }
        }

        //SET SPEED OF TIME
        if( strncmp( receivedChars, "speed", 5 ) == 0 ){
            // Parse string to int
            int tmpVal = atoi(receivedChars + 6);
            timeSpeed.setVal( static_cast<int>(tmpVal) );
            //std::cout << "Speed of time: " << timeSpeed.getVal() << std::endl;
        }

        //SET DATE MANUALLY
        if( strncmp( receivedChars, "date", 4 ) == 0 ){
            //std::cout << "SET DATE MANUALLY" << std::endl;
            std::string tempTime = ( receivedChars + 5 );

            std::string tempYear = tempTime.substr(0,4);
            std::string tempMonth = tempTime.substr(5,2);
            std::string tempDay = tempTime.substr(8,2);
            std::string tempHour = tempTime.substr(11,2);
            std::string tempMinute = tempTime.substr(14,2);
            std::string tempSeconds = tempTime.substr(17,2);

            currentTime[YEAR] = atoi( tempYear.c_str() );
            currentTime[MONTH] = atoi( tempMonth.c_str() );
            currentTime[DAY] = atoi( tempDay.c_str() );
            currentTime[HOUR] = atoi( tempHour.c_str() );
            currentTime[MINUTE] = atoi( tempMinute.c_str() );
            currentTime[SECOND] = atoi( tempSeconds.c_str() );
        }
        sgct::MessageHandler::instance()->print("Message: '%s', size: %d\n", receivedChars, size);
    }
}

void externalControlStatusCallback( bool connected ){
    if( connected )
        sgct::MessageHandler::instance()->print("External control connected.\n");
    else
        sgct::MessageHandler::instance()->print("External control disconnected.\n");
}


/*
 http://en.cppreference.com/w/cpp/chrono/c/strftime
 Function to calculate the current time, maybe needed to send this out to all the slaves later?
 */
void resetToCurrentTime() {
  /*  time_t now = time(0);
    struct tm tstruct;
    char buffer[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buffer, sizeof(buffer), "%F-%X", &tstruct);
*/

	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-%H-%M-%S");
	std::string tempTime = ss.str();

//    std::string tempTime(&buffer[0]);

    std::string tempYear = tempTime.substr(0,4);
    std::string tempMonth= tempTime.substr(5,2);
    std::string tempDay = tempTime.substr(8,2);
    std::string tempHour= tempTime.substr(11,2);
    std::string tempMinute= tempTime.substr(14,2);
    std::string tempSeconds= tempTime.substr(17,2);

    currentTime[YEAR] = atoi(tempYear.c_str());
    currentTime[MONTH] = atoi(tempMonth.c_str());
    currentTime[DAY] = atoi(tempDay.c_str());
    currentTime[HOUR] = atoi(tempHour.c_str());
    currentTime[MINUTE] = atoi(tempMinute.c_str());
    currentTime[SECOND] = atoi(tempSeconds.c_str());

}

/*Function to calculate the suns illumination angle relative to the earth*/
void calcSunPosition(){
    SpiceDouble r = 6371.0;         // Earth radius [km]
    SpiceDouble ourLon = 16.192421;    // Longitude of Nrkpg
    SpiceDouble ourLat = 58.587745;    // Latitude of Nrkpg

    SpiceChar *abcorr;
    SpiceChar *obsrvr;
    SpiceChar *target;
    SpiceChar *ref;

    SpiceDouble ourPosition[3];
    SpiceDouble sunPointOnEarth[3];
    SpiceDouble sunPosition[3];

    SpiceDouble et, lt;
    SpiceDouble srfvec[3];
    SpiceDouble trgepc;
    SpiceDouble angle;

    SpiceDouble solar;
    SpiceDouble emissn;
    SpiceDouble sslemi;
    SpiceDouble sslphs;
    SpiceDouble sslsol;
    SpiceDouble ssolpt[3];
    SpiceDouble phase;
    SpiceDouble emission;

    //convert planetocentric r/lon/lat to Cartesian 3-vector

    ourLon = ourLon * rpd_c();
    ourLat = ourLat * rpd_c();

    latrec_c( r, ourLon, ourLat, ourPosition );

    std::string tempDate = std::to_string( currentTime[YEAR] ) + " " + std::to_string( currentTime[MONTH] ) + " " + std::to_string( currentTime[DAY] ) + " " + std::to_string( currentTime[HOUR] )  + ":" + std::to_string( currentTime[MINUTE] ) + ":" + std::to_string( currentTime[SECOND] );

    char *cstr = new char[tempDate.length() + 1];
    strcpy(cstr, tempDate.c_str());

    SpiceChar * date = cstr;

    //Used to convert between time as a string into ET, which is in seconds.
    str2et_c ( date, &et );

    delete [] cstr;

    target = "EARTH";
    obsrvr = "SUN";
    abcorr = "LT+S";
    ref = "iau_earth";

    //Calculate Zenit point on earth
    subslr_c ( "Near point: ellipsoid", target, et, ref, abcorr, obsrvr, sunPointOnEarth, &trgepc, srfvec );

    //Calculate suns emission angle
    ilumin_c ( "Ellipsoid", target, et, ref, abcorr, obsrvr, ourPosition, &trgepc, srfvec, &phase, &solar, &emission );

    //fSunAnglePhi = 3.1415/2 - emission;

    SpiceDouble sunPointLon = 0;    // Longitude of zenit
    SpiceDouble sunPointLat = 0;    // Latitude of zenit

    reclat_c(&sunPointOnEarth, &r, &sunPointLon, &sunPointLat);

    fSunAnglePhi = 3.1415/2 - (ourLat-sunPointLat);

    fSunAngleTheta = ourLon - sunPointLon;

}

void addSecondToTime(){
    for(int i = 0; i < timeSpeed.getVal(); i++){
        bool leapYear = false;
        if ( ( (currentTime[YEAR] % 4 == 0) && (currentTime[YEAR] % 100 != 0) ) || (currentTime[YEAR] % 400 == 0) ){
            leapYear = true;
        }

        //Add Second
        currentTime[SECOND] += 1;

        //Add Minute
        if ( currentTime[SECOND] >= 60 ){
            currentTime[MINUTE] += 1;
            currentTime[SECOND] = 0;
        }

        //Add Hour
        if ( currentTime[MINUTE] >= 60 ){
            currentTime[HOUR] += 1;
            currentTime[MINUTE] = 0;
        }

        //Add Day
        if ( currentTime[HOUR] >= 24 ){
            currentTime[DAY] += 1;
            currentTime[HOUR] = 0;
        }

        //Add Month
            //February and leap year
            if ( leapYear && currentTime[MONTH] == 2 && currentTime[DAY] > 29 ){
                currentTime[MONTH] += 1;
                currentTime[DAY] = 1;
            }

            else if ( currentTime[MONTH] == 2 && currentTime[DAY] > 28 )
            {
                currentTime[MONTH] += 1;
                currentTime[DAY] = 1;
            }

            else if( (currentTime[MONTH] == 4 || currentTime[MONTH] == 6 || currentTime[MONTH] == 9 ||
                      currentTime[MONTH] == 11) && currentTime[DAY] > 30 ){
                currentTime[MONTH] += 1;
                currentTime[DAY] = 1;
            }

            else if( currentTime[DAY] > 31 ){
                currentTime[MONTH] += 1;
                currentTime[DAY] = 1;
            }

        //Add Year
        if ( currentTime[MONTH] > 12 ) {
            currentTime[YEAR] += 1;
            currentTime[MONTH] = 1;
        }
    }
}

void calcSkyColor(float fSunAnglePhi,float &fAmb, glm::vec4 &sColor){

    float fSine = sin(fSunAnglePhi);

    // We'll change color of skies depending on sun's position
    gEngine->setClearColor(std::max(0.0f, 0.3f*fSine), std::max(0.0f, 0.9f*fSine), std::max(0.0f, 0.9f*fSine), 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(fSunAnglePhi >= 30.0f*3.1415/180.0 && fSunAnglePhi <= 150.0f*3.1415/180.0) //DAY
    {
        sColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        fAmb = 0.8f;
    }
    else if(fSunAnglePhi <= 0.0f*3.1415/180.0 || fSunAnglePhi >= 180.0f*3.1415/180.0) //NIGHT
    {
        sColor = glm::vec4(110.0f/256.0f, 40.0f/256.0f, 189.0f/256.0f, 1.0f);
        fAmb = 0.3f;
    }
    else // DAWN/DUSK
    {
        sColor = glm::vec4(247.0f/256.0f, 21.0f/256.0f, 21.0f/256.0f, 1.0f);
        fAmb = 0.6f;
    }

}



