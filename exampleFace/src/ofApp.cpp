#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetFrameRate(60);

	face = new ofxKinectFace();
	face->setup();
}

//--------------------------------------------------------------
void ofApp::update(){
	face->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofPushMatrix();
	ofScale((float)ofGetWidth()/face->getWidth(), (float)ofGetHeight()/face->getHeight());
	face->drawColor(0, 0);
	face->draw();
	ofPopMatrix();

	ofSetColor(ofColor::white);
	ofDrawBitmapString(ofToString(ofGetFrameRate()), 20, ofGetHeight()-20);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
