/*
 *  ofxSceneManager.cpp
 *  Cocoa Test
 *
 *  Created by Oriol Ferrer Mesià on 02/11/09.
 *  Copyright 2009 uri.cat. All rights reserved.
 *
 */

#include "ofxSceneManager.h"
#include "ofxScene.h"

//--------------------------------------------------------------
//--------------------------------------------------------------
ofxSceneManager* ofxSceneManager::singleton = NULL;

//--------------------------------------------------------------
//--------------------------------------------------------------
ofxSceneManager::ofxSceneManager()
{

	currentScene = futureScene = NULL;
	curtainDropTime = 0.5f;
	curtainStayTime = 0.0f;
	curtainRiseTime = 0.5f;
	drawDebugInfo = false;
	overlapUpdate = false;
	curtain.setAnimationCurve(EASE_IN_EASE_OUT);
    
    // Register for OF events.
    ofAddListener(ofEvents().mouseDragged, this, &ofxSceneManager::mouseDragged);
    ofAddListener(ofEvents().mouseMoved, this, &ofxSceneManager::mouseMoved);
    ofAddListener(ofEvents().mousePressed, this, &ofxSceneManager::mousePressed);
    ofAddListener(ofEvents().mouseReleased, this, &ofxSceneManager::mouseReleased);
    ofAddListener(ofEvents().keyPressed, this, &ofxSceneManager::keyPressed);
    ofAddListener(ofEvents().keyReleased, this, &ofxSceneManager::keyReleased);
    
    ofAddListener(ofEvents().windowResized, this, &ofxSceneManager::windowResized);
}

//--------------------------------------------------------------
ofxSceneManager* ofxSceneManager::instance()
{
	if (!singleton){   // Only allow one instance of class to be generated.
		singleton = new ofxSceneManager();
	}
	return singleton;
}

//--------------------------------------------------------------
ofxSceneManager::~ofxSceneManager()
{
    // Unregister from OF events.
    ofRemoveListener(ofEvents().mouseDragged, this, &ofxSceneManager::mouseDragged);
    ofRemoveListener(ofEvents().mouseMoved, this, &ofxSceneManager::mouseMoved);
    ofRemoveListener(ofEvents().mousePressed, this, &ofxSceneManager::mousePressed);
    ofRemoveListener(ofEvents().mouseReleased, this, &ofxSceneManager::mouseReleased);
    ofRemoveListener(ofEvents().keyPressed, this, &ofxSceneManager::keyPressed);
    ofRemoveListener(ofEvents().keyReleased, this, &ofxSceneManager::keyReleased);
}

//--------------------------------------------------------------
void ofxSceneManager::addScene( ofxScene* newScene, int sceneID )
{

	int c = scenes.count( sceneID );
	
	if ( c > 0 ){
		ofLogVerbose() << "ofxSceneManager::addScene(" << sceneID << ") >> we already have a scene with that ID";
	}else{
		newScene->setManager( this );
		newScene->setup();
		newScene->setSceneID( sceneID );
		scenes[sceneID] = newScene;
		ofLogVerbose() << "ofxSceneManager::addScene(" << sceneID << ") >> added scene";
		if (scenes.size() == 1){	//first scene, we activate it			
			goToScene( sceneID, true/*regardless*/, false/*transition*/ ); 
			currentScene = getScene( sceneID );
		}
	}
}

//--------------------------------------------------------------
void ofxSceneManager::update( float dt )
{

	if ( !curtain.isReady() ){ //curtain is busy, so we are pre or post transition

		curtain.update(dt);
		if ( curtain.hasReachedBottom() ){			
			currentScene->sceneDidDisappear(futureScene);
			futureScene->sceneWillAppear(currentScene);
			updateHistory( futureScene );
			currentScene = futureScene;
			futureScene = NULL;
		}
		
		if ( curtain.hasReachedTop() ){
			currentScene->sceneDidAppear();
		}
	}else{
		futureScene = NULL;			
	}

	
	if (currentScene != NULL){
						
		vector <ofxScene*> screensToUpdate;
		screensToUpdate.push_back(currentScene);
		if (overlapUpdate){
			if (futureScene != NULL){
				screensToUpdate.push_back(futureScene);
			}
		}
		
		for (int i = 0; i < screensToUpdate.size(); i++){
			ofxScene * s = screensToUpdate[i];
			s->update(dt);
		}
	}
}

//--------------------------------------------------------------
void ofxSceneManager::draw()
{

	if (currentScene != NULL){
		
		vector <ofxScene*> scenesToDraw;
		scenesToDraw.push_back(currentScene);
		
		for (int i = 0; i < scenesToDraw.size(); i++){
			ofxScene * s = scenesToDraw[i];
			s->draw();
			if (drawDebugInfo){
				ofSetColor(255, 0, 0);
				s->drawDebug();
			}
		}
		
		if ( !curtain.isReady() ){
			curtain.draw();
		}
		//curtain.drawDebug();
	}
	
	if (drawDebugInfo){
		ofSetColor(255, 0, 0);
		drawDebug();
	}
}

//--------------------------------------------------------------
void ofxSceneManager::drawDebug()
{

	int y = 20;
	int x = 20;
	int lineHeight = 15;
	if (currentScene){
		ofDrawBitmapString( "Current Scene: " + ofToString(currentScene->getSceneID() ) , x, y);
		y += lineHeight;
	}
	if (futureScene){
		ofDrawBitmapString( "Future Scene: " + ofToString(futureScene->getSceneID()) , x, y);
		y += lineHeight;
	}
	if (!curtain.isReady()){
		ofDrawBitmapString( "Curtain: " + ofToString(curtain.getCurtainAlpha(), 2) , x, y);
	}
}

//--------------------------------------------------------------
bool ofxSceneManager::goToScene(int ID, bool regardless, bool doTransition)
{
	
	if ( curtain.isReady() || regardless){

		ofxScene * next = getScene( ID );
		
		if (doTransition){
			
			if (next != NULL){
				futureScene = next;
				curtain.dropAndRaiseCurtain(curtainDropTime, curtainStayTime, curtainRiseTime, regardless);
				if (currentScene) currentScene->sceneWillDisappear(futureScene);				
				return true;
			}else{
				ofLogVerbose() << "ofxSceneManager::goToScene(" << ID << ") >> scene not found!";
			}			
		}else{	
			
			if (next != NULL){
				//notify
				if (currentScene) currentScene->sceneWillDisappear(next);
				next->sceneWillAppear(currentScene);
				if (currentScene) currentScene->sceneDidDisappear(futureScene);
				next->sceneDidAppear();
				//hot-swap current scenes
				currentScene = next;
				futureScene = NULL;
				return true;
			}
		}

	}else{
		if (futureScene != NULL)
			ofLogVerbose() << "ofxSceneManager::goToScene(" << ID << ") >> CANT DO! we are in the middle of a transition to " <<  futureScene->getSceneID() << "!";
		else
			ofLogVerbose() << "ofxSceneManager::goToScene(" << ID << ") >> CANT DO! we are in the middle of a transition to another scene!";
	}
	return false;
}

//--------------------------------------------------------------
void ofxSceneManager::updateHistory( ofxScene * s )
{
	
	history.push_back(s);
	if (history.size() > MAX_HISTORY){
		history.erase(history.begin());
	}	
}

//--------------------------------------------------------------
int ofxSceneManager::getNumScenes()
{
	return scenes.size(); 
}

//--------------------------------------------------------------
ofxScene * ofxSceneManager::getCurrentScene()
{
	return currentScene;
}

//--------------------------------------------------------------
int ofxSceneManager::getCurrentSceneID()
{
	if (currentScene != NULL )
		return currentScene->getSceneID();
	else
		return NULL_SCENE;
}

//--------------------------------------------------------------
ofxScene * ofxSceneManager::getScene(int sceneID)
{
	if ( scenes.count( sceneID ) > 0 ){
		return scenes[sceneID];
	}else{
		return NULL;
	}
};

//--------------------------------------------------------------
void ofxSceneManager::setCurtainColor(ofColor color)
{
    curtain.setColor(color);
}

//--------------------------------------------------------------
void ofxSceneManager::setCurtainSize(ofRectangle rect)
{
    curtain.setScreenSize(rect);
}

//--------------------------------------------------------------
void ofxSceneManager::setCurtainDropTime(float t)
{
	curtainDropTime = t;
}

//--------------------------------------------------------------
void ofxSceneManager::setCurtainStayTime(float t)
{
	curtainStayTime = t;
}

//--------------------------------------------------------------
void ofxSceneManager::setCurtainRiseTime(float t)
{
	curtainRiseTime = t;
}

//--------------------------------------------------------------
void ofxSceneManager::setCurtainTimes(float drop, float stay, float rise)
{
	curtainDropTime = drop;
	curtainStayTime = stay;
	curtainRiseTime = rise;
}

//--------------------------------------------------------------
void ofxSceneManager::setOverlapUpdate(bool t)
{
	overlapUpdate = t;
}


/// ALL EVENTS HERE /////////////////////////////////////////////////////////// TODO
//--------------------------------------------------------------
void ofxSceneManager::keyPressed(ofKeyEventArgs& args)
{
	if (currentScene != NULL) currentScene->keyPressed( args.key );
}

//--------------------------------------------------------------
void ofxSceneManager::keyReleased(ofKeyEventArgs& args)
{
	if (currentScene != NULL) currentScene->keyReleased( args.key );
}

//--------------------------------------------------------------
void ofxSceneManager::mouseMoved(ofMouseEventArgs& args)
{
	if (currentScene != NULL) currentScene->mouseMoved( args.x, args.y );
}

//--------------------------------------------------------------
void ofxSceneManager::mouseDragged(ofMouseEventArgs& args)
{
	if (currentScene != NULL) currentScene->mouseDragged( args.x, args.y, args.button );
}

//--------------------------------------------------------------
void ofxSceneManager::mousePressed(ofMouseEventArgs& args)
{
	if (currentScene != NULL) currentScene->mousePressed( args.x, args.y, args.button );
}

//--------------------------------------------------------------
void ofxSceneManager::mouseReleased(ofMouseEventArgs& args)
{
	if (currentScene != NULL) currentScene->mouseReleased( args.x, args.y, args.button );
}

//--------------------------------------------------------------
void ofxSceneManager::windowResized (ofResizeEventArgs& args)
{
	curtain.setScreenSize( ofRectangle(0, 0, args.width, args.height) );
	for( map<int,ofxScene*>::iterator ii = scenes.begin(); ii != scenes.end(); ++ii ){
		//string key = (*ii).first;
		ofxScene* t = (*ii).second;
		t->windowResized(args.width, args.height);
	}
}

//--------------------------------------------------------------
#ifdef TARGET_OF_IPHONE
void ofxSceneManager::touchDown(ofTouchEventArgs &touch)
{
	if (currentScene != NULL) currentScene->touchDown( touch );
}

//--------------------------------------------------------------
void ofxSceneManager::touchMoved(ofTouchEventArgs &touch)
{
	if (currentScene != NULL) currentScene->touchMoved( touch );
}

//--------------------------------------------------------------
void ofxSceneManager::touchUp(ofTouchEventArgs &touch)
{
	if (currentScene != NULL) currentScene->touchUp( touch );
}

//--------------------------------------------------------------
void ofxSceneManager::touchDoubleTap(ofTouchEventArgs &touch)
{
	if (currentScene != NULL) currentScene->touchDoubleTap( touch );
}

//--------------------------------------------------------------
void ofxSceneManager::touchCancelled(ofTouchEventArgs &touch)
{
	if (currentScene != NULL) currentScene->touchCancelled( touch );
}
#endif


